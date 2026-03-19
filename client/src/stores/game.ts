import { defineStore } from 'pinia'

import { mergeEvents } from '@/lib/event-feed'
import { pbClient } from '@/lib/pb-client'

const authStorageKey = 'fanren-mud-auth'

interface AuthState {
  account: string
  token: string
}

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
    lastResult: null as Record<string, any> | null,
    loading: false,
    error: '',
    pollError: '',
    pollFailureCount: 0,
    pollIntervalMs: 1500,
    pollTimer: 0 as number | ReturnType<typeof setTimeout>,
    rankings: [] as Record<string, any>[],
    rankingType: 'realm' as 'realm' | 'wealth' | 'combat',
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
        await this.routeLogin()
        if (autoRegister) {
          const registerResponse = await pbClient.registerAccount(account, password)
          if ((registerResponse.code ?? -1) !== 0 && (registerResponse.code ?? -1) !== 40001) {
            throw new Error(registerResponse.message ?? '注册失败')
          }
        }

        const loginResponse = await pbClient.login(account, password)
        if ((loginResponse.code ?? -1) !== 0 || !loginResponse.jwt) {
          throw new Error(loginResponse.message ?? '登录失败')
        }

        this.account = account
        this.token = String(loginResponse.jwt)
        this.persistAuth()

        const enterResponse = await pbClient.enterGame(account, this.token)
        if ((enterResponse.code ?? -1) !== 0) {
          throw new Error(enterResponse.message ?? '进入游戏失败')
        }

        await this.bootstrap()
      } finally {
        this.loading = false
      }
    },
    async bootstrap() {
      if (!this.authenticated) {
        return
      }

      const response = await pbClient.bootstrap(this.account, this.token)
      if ((response.code ?? -1) !== 0) {
        throw new Error(response.message ?? '初始化失败')
      }

      this.error = ''
      this.needCreateCharacter = Boolean(response.needCreateCharacter)
      this.player = response.player ?? null
      this.scene = response.scene ?? null
      this.availableOrigins = (response.availableOrigins as Record<string, any>[]) ?? []
      this.lastResult = null
      this.pollError = ''
      this.pollFailureCount = 0
      this.appendEvents((response.events as Record<string, any>[]) ?? [])
      this.nextEventId = Number(response.nextEventId ?? 0)
      this.pollIntervalMs = Number((response.player as any)?.recommendedPollIntervalMs ?? 1500)
      if (!this.needCreateCharacter && this.player) {
        this.schedulePolling()
      }
    },
    async createCharacter(characterName: string, originId: string) {
      if (!this.authenticated) {
        return
      }

      this.loading = true
      this.error = ''
      try {
        const response = await pbClient.createCharacter(this.account, characterName, originId, this.token)
        if ((response.code ?? -1) !== 0) {
          throw new Error(response.message ?? '创建角色失败')
        }
        this.error = ''
        this.needCreateCharacter = false
        this.player = response.player ?? null
        this.scene = response.scene ?? null
        this.pollError = ''
        this.pollFailureCount = 0
        this.appendEvents((response.events as Record<string, any>[]) ?? [])
        this.nextEventId = Number(response.nextEventId ?? 0)
        this.schedulePolling()
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
    async loadRankings(leaderboard: 'realm' | 'wealth' | 'combat' = 'realm') {
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
