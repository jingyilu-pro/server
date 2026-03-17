import { parse } from 'protobufjs'

import gatewayProto from '@/proto/gateway.proto?raw'
import mudProto from '@/proto/mud.proto?raw'

const gatewayRoot = parse(gatewayProto).root
const mudRoot = parse(mudProto).root

type RootKind = 'gateway' | 'mud'

function getRoot(kind: RootKind) {
  return kind === 'gateway' ? gatewayRoot : mudRoot
}

export function encodeMessage(kind: RootKind, typeName: string, payload: Record<string, unknown>) {
  const type = getRoot(kind).lookupType(typeName)
  const message = type.fromObject(payload)
  return type.encode(message).finish()
}

export function decodeMessage<T>(kind: RootKind, typeName: string, payload: Uint8Array) {
  const type = getRoot(kind).lookupType(typeName)
  const decoded = type.decode(payload)
  return type.toObject(decoded, {
    longs: Number,
    defaults: true,
    arrays: true,
    objects: true,
  }) as T
}
