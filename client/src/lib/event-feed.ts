export interface FeedEvent {
  eventId?: number
  title?: string
  content?: string
}

export function mergeEvents<T extends FeedEvent>(current: T[], incoming: T[]) {
  const merged = [...current]
  const seen = new Set<number>()

  for (const item of current) {
    if (typeof item.eventId === 'number') {
      seen.add(item.eventId)
    }
  }

  for (const item of incoming) {
    if (typeof item.eventId === 'number' && seen.has(item.eventId)) {
      continue
    }
    if (typeof item.eventId === 'number') {
      seen.add(item.eventId)
    }
    merged.push(item)
  }

  return merged.sort((left, right) => (left.eventId ?? 0) - (right.eventId ?? 0))
}
