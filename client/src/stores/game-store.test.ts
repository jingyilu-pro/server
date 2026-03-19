import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest'
import { createPinia, setActivePinia } from 'pinia'

const routeLoginMock = vi.fn()
const registerAccountMock = vi.fn()
const loginMock = vi.fn()
const enterGameMock = vi.fn()
const bootstrapMock = vi.fn()
const createCharacterMock = vi.fn()

vi.mock('@/lib/pb-client', () => ({
  pbClient: {
    routeLogin: (...args: unknown[]) => routeLoginMock(...args),
    registerAccount: (...args: unknown[]) => registerAccountMock(...args),
    login: (...args: unknown[]) => loginMock(...args),
    enterGame: (...args: unknown[]) => enterGameMock(...args),
    bootstrap: (...args: unknown[]) => bootstrapMock(...args),
    createCharacter: (...args: unknown[]) => createCharacterMock(...args),
  },
}))

import { useGameStore } from '@/stores/game'

describe('useGameStore role recovery', () => {
  beforeEach(() => {
    vi.useFakeTimers()
    const storage = new Map<string, string>()
    vi.stubGlobal('window', {
      setTimeout: globalThis.setTimeout,
      clearTimeout: globalThis.clearTimeout,
    })
    vi.stubGlobal('localStorage', {
      getItem: (key: string) => storage.get(key) ?? null,
      setItem: (key: string, value: string) => {
        storage.set(key, value)
      },
      removeItem: (key: string) => {
        storage.delete(key)
      },
      clear: () => {
        storage.clear()
      },
    })
    setActivePinia(createPinia())
    routeLoginMock.mockReset()
    registerAccountMock.mockReset()
    loginMock.mockReset()
    enterGameMock.mockReset()
    bootstrapMock.mockReset()
    createCharacterMock.mockReset()
    localStorage.clear()
  })

  afterEach(() => {
    vi.clearAllTimers()
    vi.useRealTimers()
    vi.unstubAllGlobals()
  })

  it('prefers an existing playable snapshot over a stale needCreateCharacter flag', async () => {
    const store = useGameStore()
    store.account = 'recover_acc'
    store.token = 'recover_token'

    bootstrapMock.mockResolvedValue({
      code: 0,
      needCreateCharacter: true,
      player: {
        account: 'recover_acc',
        characterName: '韩立',
      },
      scene: {
        sceneId: 'qixuan_square',
      },
      events: [],
      nextEventId: 7,
    })

    await store.bootstrap()

    expect(store.needCreateCharacter).toBe(false)
    expect(store.readyToPlay).toBe(true)
    expect(store.player?.characterName).toBe('韩立')
    expect(store.scene?.sceneId).toBe('qixuan_square')
  })

  it('rejects auto registration when the new account contains non-alphanumeric characters', async () => {
    const store = useGameStore()

    await expect(store.loginFlow('工人', 'pw123', true)).rejects.toThrow('新注册账号只支持英文和数字')

    expect(routeLoginMock).not.toHaveBeenCalled()
    expect(registerAccountMock).not.toHaveBeenCalled()
  })

  it('stops the auto registration flow when the backend rejects the account format', async () => {
    const store = useGameStore()

    routeLoginMock.mockResolvedValue({ code: 0 })
    registerAccountMock.mockResolvedValue({
      code: 40001,
      message: 'account must contain only English letters and digits',
    })

    await expect(store.loginFlow('worker01', 'pw123', true)).rejects.toThrow('新注册账号只支持英文和数字')

    expect(routeLoginMock).toHaveBeenCalledTimes(1)
    expect(registerAccountMock).toHaveBeenCalledTimes(1)
    expect(loginMock).not.toHaveBeenCalled()
  })

  it('still allows legacy non-alphanumeric accounts to log in without auto registration', async () => {
    const store = useGameStore()

    routeLoginMock.mockResolvedValue({ code: 0 })
    loginMock.mockResolvedValue({ code: 0, jwt: 'legacy_token' })
    enterGameMock.mockResolvedValue({ code: 0 })
    bootstrapMock.mockResolvedValue({
      code: 0,
      needCreateCharacter: true,
      player: {
        account: '工人',
        characterName: '韩立',
      },
      scene: {
        sceneId: 'qixuan_square',
      },
      events: [],
      nextEventId: 3,
    })

    await store.loginFlow('工人', 'pw123', false)

    expect(routeLoginMock).toHaveBeenCalledTimes(1)
    expect(loginMock).toHaveBeenCalledWith('工人', 'pw123')
    expect(enterGameMock).toHaveBeenCalledWith('工人', 'legacy_token')
    expect(store.account).toBe('工人')
    expect(store.readyToPlay).toBe(true)
    expect(store.needCreateCharacter).toBe(false)
  })

  it('reboots into the existing role when createCharacter reports that the character already exists', async () => {
    const store = useGameStore()
    store.account = 'recover_acc'
    store.token = 'recover_token'
    store.needCreateCharacter = true

    createCharacterMock.mockResolvedValue({
      code: 40002,
      message: 'character already exists',
    })
    bootstrapMock.mockResolvedValue({
      code: 0,
      needCreateCharacter: false,
      player: {
        account: 'recover_acc',
        characterName: '韩立',
      },
      scene: {
        sceneId: 'qixuan_square',
      },
      events: [{ eventId: 1, channel: 'system', title: '恢复成功', text: '已恢复已有角色。' }],
      nextEventId: 1,
    })

    await store.createCharacter('韩立', 'tiannan_human')

    expect(createCharacterMock).toHaveBeenCalledTimes(1)
    expect(bootstrapMock).toHaveBeenCalledTimes(1)
    expect(store.needCreateCharacter).toBe(false)
    expect(store.readyToPlay).toBe(true)
    expect(store.player?.characterName).toBe('韩立')
  })
})
