import { describe, expect, it } from 'vitest'

import { mergeEvents } from '@/lib/event-feed'

describe('mergeEvents', () => {
  it('deduplicates by eventId and keeps ascending order', () => {
    const merged = mergeEvents(
      [
        { eventId: 2, title: 'b' },
        { eventId: 4, title: 'd' },
      ],
      [
        { eventId: 3, title: 'c' },
        { eventId: 4, title: 'd-dup' },
      ],
    )

    expect(merged.map((item) => item.eventId)).toEqual([2, 3, 4])
  })
})
