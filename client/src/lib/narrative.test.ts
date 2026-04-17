import { describe, expect, it } from 'vitest'

import {
  narrativeArcOrderForChapter,
  narrativeArcOrderForQuest,
  narrativeArcOrderForScene,
} from '@/lib/narrative'

describe('narrativeArcOrderForScene', () => {
  it('maps the late-game scenes into the new late arcs', () => {
    expect(narrativeArcOrderForScene('outer_sea_mid')).toBe(7)
    expect(narrativeArcOrderForScene('core_flame_vein')).toBe(8)
    expect(narrativeArcOrderForScene('ancient_ruin_ring')).toBe(8)
    expect(narrativeArcOrderForScene('star_abyss')).toBe(9)
  })
})

describe('narrativeArcOrderForQuest', () => {
  it('keeps the late-game quest ladder ordered for tracked quest selection', () => {
    expect(narrativeArcOrderForQuest('outer_sea_trail')).toBe(7)
    expect(narrativeArcOrderForQuest('gold_core_gate')).toBe(7)
    expect(narrativeArcOrderForQuest('core_ruin_heart')).toBe(8)
    expect(narrativeArcOrderForQuest('nascent_soul_gate')).toBe(9)
  })
})

describe('narrativeArcOrderForChapter', () => {
  it('recognizes the late-game chapter labels emitted by the runtime', () => {
    expect(narrativeArcOrderForChapter('结丹之门')).toBe(7)
    expect(narrativeArcOrderForChapter('古修残环')).toBe(8)
    expect(narrativeArcOrderForChapter('凝婴前夜')).toBe(9)
  })
})
