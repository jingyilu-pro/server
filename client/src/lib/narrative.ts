export const narrativeArcLabels: Record<number, string> = {
  1: '七玄门启程',
  2: '太南小会',
  3: '黄枫谷试炼',
  4: '血色禁地试炼',
  5: '乱星海漂流',
  6: '虚天殿初启',
  7: '结丹之门',
  8: '古修残环',
  9: '凝婴前夜',
}

export const narrativeQuestOrder: Record<string, number> = {
  backslope_wolf_skin: 1,
  qixuan_stream_note: 1,
  qixuan_herb: 1,
  mofu_guest_token: 1,
  ruins_old_map: 4,
  fair_rumor_packet: 2,
  tainan_snake: 2,
  tainan_array_flag: 2,
  huangfeng_letter: 3,
  medicine_moss: 3,
  huangfeng_manual: 3,
  blood_forbidden_token: 4,
  blood_swamp_rescue: 4,
  harbor_signal: 5,
  chaos_sea_chart: 5,
  captain_supply: 5,
  demon_fish_core: 5,
  chaos_relic: 5,
  xutian_key: 6,
  xutian_star_map: 6,
  outer_sea_trail: 7,
  gold_core_gate: 7,
  core_ruin_heart: 8,
  nascent_soul_gate: 9,
}

export const sceneFollowupHints: Record<string, string> = {
  chaos_sea_ship:
    '当前章节：乱星海漂流。甲板术士更像是在替你梳理后续海路与虚天线索，可继续交谈后再向深海推进。',
  chaos_sea_isle: '残碑孤岛的碑文与钥片都和虚天殿有关，别忘了查看遗迹、人物与地面线索。',
  storm_route: '当前章节：乱星海漂流。先稳住法力与气力，再沿风暴航道继续向东探路。',
  outer_sea_mid: '当前章节：结丹之门。外海会先逼你认清自身火候，再决定是否把真正的稳丹主材交到你手里。',
  core_flame_vein:
    '当前章节：古修残环。这里真正考的是你能不能在古修余火里稳住心神与经脉，而不只是扛住伤害。',
  ancient_ruin_ring:
    '当前章节：古修残环。残环主殿里的东西不会白给，先把旧令牌、残环与守门残灵的话连起来看。',
  star_abyss:
    '当前章节：凝婴前夜。星渊不是单纯的采集点，更像在确认你有没有资格把凝婴这一步真正扛到身上。',
}

export function narrativeArcOrderForChapter(value: string) {
  const normalized = value.trim()
  if (!normalized) {
    return 0
  }

  const entry = Object.entries(narrativeArcLabels).find(([, label]) => label === normalized)
  return entry ? Number(entry[0]) : 0
}

export function narrativeArcOrderForScene(sceneId: string) {
  const normalized = sceneId.trim()
  if (!normalized) {
    return 0
  }
  if (normalized === 'star_abyss') {
    return 9
  }
  if (normalized === 'core_flame_vein' || normalized === 'ancient_ruin_ring') {
    return 8
  }
  if (normalized === 'outer_sea_mid') {
    return 7
  }
  if (normalized.startsWith('xutian_')) {
    return 6
  }
  if (
    normalized.startsWith('chaos_sea_') ||
    normalized.startsWith('tiannan_') ||
    normalized === 'reef_shore' ||
    normalized === 'demon_fish_nest' ||
    normalized === 'storm_route' ||
    normalized === 'sea_wind_tower'
  ) {
    return 5
  }
  if (normalized.startsWith('blood_')) {
    return 4
  }
  if (normalized.startsWith('huangfeng_')) {
    return 3
  }
  if (normalized.startsWith('tainan_') || normalized === 'xin_house' || normalized === 'array_lane') {
    return 2
  }
  return 1
}

export function narrativeArcOrderForQuest(questId: string) {
  return narrativeQuestOrder[questId] ?? 0
}
