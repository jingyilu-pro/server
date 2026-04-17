export function formatProgressStageLabel(rawValue: string) {
  const normalized = String(rawValue ?? '').trim()
  if (!normalized) {
    return '凡躯启程'
  }
  if (normalized.includes('元婴')) {
    return '元婴初成'
  }
  if (normalized.includes('结丹后期') || normalized.includes('结丹圆满')) {
    return '结丹圆满'
  }
  if (normalized.includes('结丹') || normalized.includes('金丹')) {
    return '金丹凝练'
  }
  if (normalized.includes('筑基中期') || normalized.includes('筑基后期') || normalized.includes('筑基深修')) {
    return '筑基深修'
  }
  return normalized
}

export function formatHelpTopicTitle(topicId: string, fallbackTitle = '') {
  const normalizedTopicId = String(topicId ?? '').trim()
  const normalizedFallback = String(fallbackTitle ?? '').trim()
  if (normalizedTopicId === 'core_dan') {
    return '结丹'
  }
  if (normalizedTopicId === 'nascent_soul') {
    return '凝婴'
  }
  return normalizedFallback
}
