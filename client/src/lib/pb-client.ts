import { decodeMessage, encodeMessage } from '@/lib/protobuf'

const managerBase = '/api/manager'
const loginBase = '/api/login'
const gameBase = '/api/game'

export interface ProtoResponse {
  code?: number
  message?: string
  [key: string]: unknown
}

function ensureHeaderSafeToken(token: string) {
  for (const char of token) {
    if ((char.codePointAt(0) ?? 0) > 0xff) {
      throw new Error('登录态包含非法字符，请重新登录')
    }
  }
}

async function protobufRequest<T extends ProtoResponse>(options: {
  kind: 'gateway' | 'mud'
  requestType: string
  responseType: string
  path: string
  payload: Record<string, unknown>
  token?: string
}) {
  const body = encodeMessage(options.kind, options.requestType, options.payload)
  const headers: Record<string, string> = {
    'Content-Type': 'application/x-protobuf',
  }

  if (options.token) {
    ensureHeaderSafeToken(options.token)
    headers.Authorization = `Bearer ${options.token}`
  }

  const response = await fetch(options.path, {
    method: 'POST',
    headers,
    body: body as unknown as BodyInit,
  })

  if (!response.ok) {
    throw new Error(`HTTP ${response.status} ${response.statusText}`)
  }

  const binary = new Uint8Array(await response.arrayBuffer())
  return decodeMessage<T>(options.kind, options.responseType, binary)
}

export const pbClient = {
  routeLogin() {
    return protobufRequest({
      kind: 'gateway',
      requestType: 'gateway.RouteLoginRequest',
      responseType: 'gateway.RouteLoginResponse',
      path: `${managerBase}/v1/route/login`,
      payload: { clientVersion: 'h5_mud_v1' },
    })
  },
  registerAccount(account: string, password: string) {
    return protobufRequest({
      kind: 'gateway',
      requestType: 'gateway.AuthRegisterRequest',
      responseType: 'gateway.AuthRegisterResponse',
      path: `${loginBase}/v1/auth/register`,
      payload: { account, password },
    })
  },
  login(account: string, password: string) {
    return protobufRequest({
      kind: 'gateway',
      requestType: 'gateway.AuthLoginRequest',
      responseType: 'gateway.AuthLoginResponse',
      path: `${loginBase}/v1/auth/login`,
      payload: { account, password },
    })
  },
  enterGame(account: string, token: string) {
    return protobufRequest({
      kind: 'gateway',
      requestType: 'gateway.GameEnterRequest',
      responseType: 'gateway.GameEnterResponse',
      path: `${gameBase}/v1/game/enter`,
      payload: { account },
      token,
    })
  },
  bootstrap(account: string, token: string) {
    return protobufRequest({
      kind: 'mud',
      requestType: 'mud.BootstrapRequest',
      responseType: 'mud.BootstrapResponse',
      path: `${gameBase}/v1/game/bootstrap`,
      payload: { account },
      token,
    })
  },
  createCharacter(account: string, characterName: string, originId: string, token: string) {
    return protobufRequest({
      kind: 'mud',
      requestType: 'mud.CharacterCreateRequest',
      responseType: 'mud.CharacterCreateResponse',
      path: `${gameBase}/v1/game/character/create`,
      payload: {
        account,
        characterName,
        originId,
      },
      token,
    })
  },
  executeCommand(account: string, command: string, token: string) {
    return protobufRequest({
      kind: 'mud',
      requestType: 'mud.CommandExecuteRequest',
      responseType: 'mud.CommandExecuteResponse',
      path: `${gameBase}/v1/game/command/execute`,
      payload: { account, command },
      token,
    })
  },
  pullFeed(account: string, afterEventId: number, limit: number, token: string) {
    return protobufRequest({
      kind: 'mud',
      requestType: 'mud.FeedPullRequest',
      responseType: 'mud.FeedPullResponse',
      path: `${gameBase}/v1/game/feed/pull`,
      payload: {
        account,
        afterEventId,
        limit,
      },
      token,
    })
  },
  loadCodexList(account: string, category: string, token: string) {
    return protobufRequest({
      kind: 'mud',
      requestType: 'mud.CodexListRequest',
      responseType: 'mud.CodexListResponse',
      path: `${gameBase}/v1/game/codex/list`,
      payload: {
        account,
        category,
      },
      token,
    })
  },
  loadCodexDetail(account: string, entryId: string, token: string) {
    return protobufRequest({
      kind: 'mud',
      requestType: 'mud.CodexDetailRequest',
      responseType: 'mud.CodexDetailResponse',
      path: `${gameBase}/v1/game/codex/detail`,
      payload: {
        account,
        entryId,
      },
      token,
    })
  },
  loadRankings(account: string, leaderboard: string, limit: number, token: string) {
    return protobufRequest({
      kind: 'mud',
      requestType: 'mud.RankListRequest',
      responseType: 'mud.RankListResponse',
      path: `${gameBase}/v1/game/rank/list`,
      payload: {
        account,
        leaderboard,
        limit,
      },
      token,
    })
  },
}
