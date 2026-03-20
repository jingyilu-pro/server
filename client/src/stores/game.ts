import { defineStore } from 'pinia'

import { mergeEvents } from '@/lib/event-feed'
import { pbClient } from '@/lib/pb-client'

const authStorageKey = 'fanren-mud-auth'

interface AuthState {
  account: string
  token: string
}

type RankingType = 'realm' | 'wealth' | 'combat' | 'alchemy' | 'travel' | 'bounty' | 'chief'

const gatewayErrorCode = {
  characterAlreadyExistsOrInvalidInput: 40002,
  invalidOrExpiredJwt: 40102,
} as const

function loadAuthState(): AuthState {
  if (typeof localStorage === 'undefined') {
    return { account: '', token: '' }
  }

  const raw = localStorage.getItem(authStorageKey)
  if (!raw) {
    return { account: '', token: '' }
  }

  try {
    const parsed = JSON.parse(raw) as Partial<AuthState>
    return {
      account: parsed.account ?? '',
      token: parsed.token ?? '',
    }
  } catch {
    return { account: '', token: '' }
  }
}

function isInvalidTokenHeaderError(error: unknown) {
  return error instanceof Error && /登录态包含非法字符/.test(error.message)
}

function isRegisterableAccount(account: string) {
  return account.length > 0 && /^[A-Za-z0-9]+$/.test(account)
}

function hasPlayableSnapshot(player: Record<string, any> | null | undefined, scene: Record<string, any> | null | undefined) {
  const characterName = String(player?.characterName ?? '').trim()
  const playerAccount = String(player?.account ?? '').trim()
  const sceneId = String(scene?.sceneId ?? '').trim()
  return Boolean(sceneId && (characterName || playerAccount))
}

function isCharacterAlreadyExistsResponse(response: Record<string, any> | null | undefined) {
  const code = Number(response?.code ?? -1)
  const message = String(response?.message ?? '')
  return code === gatewayErrorCode.characterAlreadyExistsOrInvalidInput && /character already exists/i.test(message)
}

function isInvalidRegisterAccountResponse(response: Record<string, any> | null | undefined) {
  const code = Number(response?.code ?? -1)
  const message = String(response?.message ?? '')
  return code === 40001 && /english letters and digits/i.test(message)
}

function isInvalidOrExpiredJwtResponse(response: Record<string, any> | null | undefined) {
  return Number(response?.code ?? -1) === gatewayErrorCode.invalidOrExpiredJwt
}

export const useGameStore = defineStore('game', {
  state: () => ({
    account: loadAuthState().account,
    token: loadAuthState().token,
    player: null as Record<string, any> | null,
    scene: null as Record<string, any> | null,
    events: [] as Record<string, any>[],
    commandHistory: [] as string[],
    nextEventId: 0,
    needCreateCharacter: false,
    availableOrigins: [] as Record<string, any>[],
    availableBackgrounds: [] as Record<string, any>[],
    lastResult: null as Record<string, any> | null,
    loading: false,
    error: '',
    pollError: '',
    pollFailureCount: 0,
    pollIntervalMs: 1500,
    pollTimer: 0 as number | ReturnType<typeof setTimeout>,
    rankings: [] as Record<string, any>[],
    rankingType: 'realm' as RankingType,
    codexEntries: [] as Record<string, any>[],
    codexDetail: null as Record<string, any> | null,
  }),
  getters: {
    authenticated: (state) => Boolean(state.account && state.token),
    readyToPlay: (state) => Boolean(state.player && state.scene),
  },
  actions: {
    persistAuth() {
      if (typeof localStorage === 'undefined') {
        return
      }
      localStorage.setItem(
        authStorageKey,
        JSON.stringify({
          account: this.account,
          token: this.token,
        }),
      )
    },
    clearAuth() {
      if (typeof localStorage === 'undefined') {
        return
      }
      localStorage.removeItem(authStorageKey)
    },
    appendEvents(incoming: Record<string, any>[] = []) {
      this.events = mergeEvents(this.events, incoming)
    },
    stopPolling() {
      if (this.pollTimer) {
        clearTimeout(this.pollTimer)
        this.pollTimer = 0
      }
    },
    schedulePolling() {
      this.stopPolling()
      if (!this.authenticated || !this.player) {
        return
      }

      this.pollTimer = window.setTimeout(async () => {
        try {
          await this.pullFeed()
        } catch (error) {
          this.pollFailureCount += 1
          if (this.pollFailureCount >= 3) {
            this.pollError = '与游戏世界暂时失去联系，正在重连…'
          }
        } finally {
          this.schedulePolling()
        }
      }, this.pollIntervalMs)
    },
    async routeLogin() {
      return pbClient.routeLogin()
    },
    async loginFlow(account: string, password: string, autoRegister = false) {
      this.loading = true
      this.error = ''
      try {
        if (autoRegister && !isRegisterableAccount(account)) {
          throw new Error('新注册账号只支持英文和数字，请改成例如 hanli001 这样的账号名')
        }

        await this.routeLogin()
        if (autoRegister) {
          const registerResponse = await pbClient.registerAccount(account, password)
          if (isInvalidRegisterAccountResponse(registerResponse)) {
            throw new Error('新注册账号只支持英文和数字，请改成例如 hanli001 这样的账号名')
          }
          if ((registerResponse.code ?? -1) !== 0 && (registerResponse.code ?? -1) !== 40001) {
            throw new Error(registerResponse.message ?? '注册失败')
          }
        }

        const loginResponse = await pbClient.login(account, password)
        if ((loginResponse.code ?? -1) !== 0 || !loginResponse.jwt) {
          throw new Error(loginResponse.message ?? '登录失败')
        }

        const issuedToken = String(loginResponse.jwt)

        const enterResponse = await pbClient.enterGame(account, issuedToken)
        if ((enterResponse.code ?? -1) !== 0) {
          throw new Error(enterResponse.message ?? '进入游戏失败')
        }

        this.account = account
        this.token = issuedToken
        this.persistAuth()

        await this.bootstrap()
      } finally {
        this.loading = false
      }
    },
    async bootstrap() {
      if (!this.authenticated) {
        return
      }

      try {
        const response = await pbClient.bootstrap(this.account, this.token)
        if ((response.code ?? -1) !== 0) {
          if (isInvalidOrExpiredJwtResponse(response)) {
            this.logout()
            throw new Error('登录已失效，请重新登录')
          }
          throw new Error(response.message ?? '初始化失败')
        }

        const player = (response.player as Record<string, any> | undefined) ?? null
        const scene = (response.scene as Record<string, any> | undefined) ?? null
        const playable = hasPlayableSnapshot(player, scene)

        this.error = ''
        this.needCreateCharacter = Boolean(response.needCreateCharacter) && !playable
        this.player = player
        this.scene = scene
        this.availableOrigins = (response.availableOrigins as Record<string, any>[]) ?? []
        this.availableBackgrounds = (response.availableBackgrounds as Record<string, any>[]) ?? []
        this.lastResult = null
        this.pollError = ''
        this.pollFailureCount = 0
        this.appendEvents((response.events as Record<string, any>[]) ?? [])
        this.nextEventId = Number(response.nextEventId ?? 0)
        this.pollIntervalMs = Number((player as any)?.recommendedPollIntervalMs ?? 1500)
        if (playable) {
          this.schedulePolling()
        } else {
          this.stopPolling()
        }
      } catch (error) {
        if (isInvalidTokenHeaderError(error)) {
          this.logout()
        }
        throw error
      }
    },
    async createCharacter(characterName: string, originId: string, backgroundId = '') {
      if (!this.authenticated) {
        return
      }

      this.loading = true
      this.error = ''
      try {
        const response = await pbClient.createCharacter(this.account, characterName, originId, backgroundId, this.token)
        if (isInvalidOrExpiredJwtResponse(response)) {
          this.logout()
          throw new Error('登录已失效，请重新登录')
        }
        if (isCharacterAlreadyExistsResponse(response)) {
          try {
            await this.bootstrap()
          } catch {
            // Keep the original recovery message below if bootstrap still cannot restore the role.
          }

          if (this.readyToPlay) {
            this.error = ''
            this.needCreateCharacter = false
            return
          }

          throw new Error('这个账号已经有角色了，已尝试恢复存档。请点“重新检查角色”，或先退出后重新登录。')
        }
        if ((response.code ?? -1) !== 0) {
          throw new Error(response.message ?? '创建角色失败')
        }

        const player = (response.player as Record<string, any> | undefined) ?? null
        const scene = (response.scene as Record<string, any> | undefined) ?? null
        const playable = hasPlayableSnapshot(player, scene)

        this.error = ''
        this.needCreateCharacter = !playable
        this.player = player
        this.scene = scene
        this.pollError = ''
        this.pollFailureCount = 0
        this.appendEvents((response.events as Record<string, any>[]) ?? [])
        this.nextEventId = Number(response.nextEventId ?? 0)
        if (playable) {
          this.schedulePolling()
        } else {
          this.stopPolling()
        }
      } finally {
        this.loading = false
      }
    },
    async executeCommand(command: string) {
      if (!this.authenticated) {
        return
      }
      const trimmed = command.trim()
      if (!trimmed) {
        return
      }

      this.loading = true
      this.error = ''
      try {
        const response = await pbClient.executeCommand(this.account, trimmed, this.token)
        this.commandHistory.unshift(trimmed)
        this.commandHistory = this.commandHistory.slice(0, 20)
        this.lastResult = response.result ?? null
        this.player = response.player ?? this.player
        this.scene = response.scene ?? this.scene
        this.appendEvents((response.events as Record<string, any>[]) ?? [])
        this.nextEventId = Number(response.nextEventId ?? this.nextEventId)
        this.pollIntervalMs = Number((response.result as any)?.recommendedPollIntervalMs ?? 1500)
        if ((response.code ?? -1) !== 0) {
          throw new Error(response.message ?? '命令执行失败')
        }
        this.error = ''
      } finally {
        this.loading = false
      }
    },
    async pullFeed() {
      if (!this.authenticated) {
        return
      }

      const response = await pbClient.pullFeed(this.account, this.nextEventId, 100, this.token)
      if ((response.code ?? -1) !== 0) {
        throw new Error(response.message ?? '拉取事件失败')
      }
      this.pollFailureCount = 0
      this.pollError = ''
      this.appendEvents((response.events as Record<string, any>[]) ?? [])
      this.scene = response.scene ?? this.scene
      this.nextEventId = Number(response.nextEventId ?? this.nextEventId)
      this.pollIntervalMs = Number(response.recommendedPollIntervalMs ?? this.pollIntervalMs)
    },
    async loadRankings(leaderboard: RankingType = 'realm') {
      if (!this.authenticated) {
        return
      }

      const response = await pbClient.loadRankings(this.account, leaderboard, 10, this.token)
      if ((response.code ?? -1) !== 0) {
        throw new Error(response.message ?? '加载排行失败')
      }
      this.rankingType = leaderboard
      this.rankings = (response.entries as Record<string, any>[]) ?? []
    },
    async loadCodexList(category = '') {
      if (!this.authenticated) {
        return
      }

      const response = await pbClient.loadCodexList(this.account, category, this.token)
      if ((response.code ?? -1) !== 0) {
        throw new Error(response.message ?? '加载手册失败')
      }
      this.codexEntries = (response.entries as Record<string, any>[]) ?? []
    },
    async loadCodexDetail(entryId: string) {
      if (!this.authenticated) {
        return
      }

      const response = await pbClient.loadCodexDetail(this.account, entryId, this.token)
      if ((response.code ?? -1) !== 0) {
        throw new Error(response.message ?? '加载手册详情失败')
      }
      this.codexDetail = (response.entry as Record<string, any> | undefined) ?? null
    },
    logout() {
      this.stopPolling()
      this.account = ''
      this.token = ''
      this.player = null
      this.scene = null
      this.events = []
      this.commandHistory = []
      this.nextEventId = 0
      this.needCreateCharacter = false
      this.availableOrigins = []
      this.availableBackgrounds = []
      this.lastResult = null
      this.error = ''
      this.pollError = ''
      this.pollFailureCount = 0
      this.rankings = []
      this.codexEntries = []
      this.codexDetail = null
      this.clearAuth()
    },
  },
})
