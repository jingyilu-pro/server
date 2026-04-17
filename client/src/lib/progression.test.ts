import { describe, expect, it } from 'vitest'

import { formatHelpTopicTitle, formatProgressStageLabel } from '@/lib/progression'

describe('formatProgressStageLabel', () => {
  it('compresses later-stage realm names into the terminal-facing labels', () => {
    expect(formatProgressStageLabel('元婴初期')).toBe('元婴初成')
    expect(formatProgressStageLabel('结丹中期')).toBe('金丹凝练')
    expect(formatProgressStageLabel('结丹后期')).toBe('结丹圆满')
    expect(formatProgressStageLabel('筑基后期')).toBe('筑基深修')
  })

  it('falls back to the original stage text for earlier realms', () => {
    expect(formatProgressStageLabel('炼气后期')).toBe('炼气后期')
    expect(formatProgressStageLabel('')).toBe('凡躯启程')
  })
})

describe('formatHelpTopicTitle', () => {
  it('maps the late-stage help topics to stable terminal labels', () => {
    expect(formatHelpTopicTitle('core_dan', '')).toBe('结丹')
    expect(formatHelpTopicTitle('nascent_soul', '')).toBe('凝婴')
  })

  it('keeps existing titles for unrelated topics', () => {
    expect(formatHelpTopicTitle('work', '工作总览')).toBe('工作总览')
  })
})
