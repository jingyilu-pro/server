<script setup lang="ts">
import { computed, nextTick, onMounted, ref, watch } from 'vue'

import { useGameStore } from '@/stores/game'
import { directionLabelMap, worldMapEdges, worldMapNodes } from '@/lib/world-map'

type SideTab = 'player' | 'quests' | 'inventory' | 'map' | 'team' | 'rank'
type ComposerMode = 'chat' | 'command'
type CommandCategoryId = 'social' | 'explore' | 'tasks' | 'combat' | 'cultivation' | 'trade' | 'group'
type OverlayPanel = 'none' | 'commands' | 'scene' | SideTab
type SceneInteractableKind = 'player' | 'npc' | 'shop' | 'monster'

interface CommandAction {
  key: string
  label: string
  detail: string
  command?: string
  execute?: boolean
  composer?: ComposerMode
  chatChannel?: 'world' | 'team'
  prefillText?: string
}

interface DenseLine {
  key: string
  tag: string
  text: string
  tone: 'system' | 'chat' | 'quest' | 'combat' | 'hint'
}

interface DockEntry {
  key: string
  label: string
  overlay: 'commands' | SideTab
}

interface SceneInteractable {
  key: string
  kind: SceneInteractableKind
  railLabel: string
  railCaption: string
  title: string
  subtitle: string
  description: string
  meta: string[]
  actions: CommandAction[]
}

interface OrientationTile {
  key: string
  slot: 'north' | 'west' | 'center' | 'east' | 'south'
  label: string
  caption: string
  command?: string
  disabled?: boolean
}

const store = useGameStore()

const account = ref(store.account)
const password = ref('')
const characterName = ref('')
const composerText = ref('')
const activeTab = ref<SideTab>('player')
const activeCommandCategory = ref<CommandCategoryId>('social')
const composerMode = ref<ComposerMode>('chat')
const chatChannel = ref<'world' | 'team'>('world')
const activeOverlay = ref<OverlayPanel>('none')
const selectedSceneInteractableKey = ref('')
const eventViewport = ref<HTMLElement | null>(null)
const storyViewport = ref<HTMLElement | null>(null)

const sceneQuestOffers: Record<string, Array<{ id: string; title: string; summary: string }>> = {
  jiayuan_market: [{ id: 'qixuan_herb', title: '墨府采药', summary: '嘉元城总管正急需黄精草，你若接下此事，便能借此熟悉坊市与采集路线。' }],
  tainan_fair: [{ id: 'tainan_snake', title: '太南异胆', summary: '太南小会有人高价收购异胆，这条线索会带你熟悉散修交易与小规模战斗。' }],
  huangfeng_outpost: [{ id: 'huangfeng_letter', title: '黄枫谷羽信', summary: '外营弟子正等人送一封羽信入谷，这是接触黄枫谷主线的入口。' }],
  blood_gate: [{ id: 'blood_forbidden_token', title: '血禁采兰', summary: '血禁石门附近传出灵兰踪迹，接下任务后可进一步熟悉禁地探索规则。' }],
  tiannan_harbor: [{ id: 'chaos_sea_chart', title: '乱星海海图', summary: '天南港口有人在找可托付的修士，拿到海图便能继续深入海域。' }],
  chaos_sea_isle: [{ id: 'xutian_key', title: '虚天残钥', summary: '孤岛残碑下埋有虚天殿残钥的线索，适合作为后续秘境主线入口。' }],
}

const sceneSectOffers: Record<string, Array<{ command: string; name: string; summary: string }>> = {
  qixuan_hall: [{ command: 'join qixuan_gate', name: '七玄门', summary: '七玄门适合凡人启程，在这里能打好最初的修行和江湖根基。' }],
  huangfeng_hall: [{ command: 'join huangfeng_valley', name: '黄枫谷', summary: '黄枫谷重视基础与心性，是越国七派里较稳的一条成长路线。' }],
  yanyue_peak: [{ command: 'join yanyue_sect', name: '掩月宗', summary: '掩月宗重视身法与法门，入门后更适合往灵动轻灵路线修行。' }],
}

const categoryLabels: Record<CommandCategoryId, string> = {
  social: '交流',
  explore: '探索',
  tasks: '任务',
  combat: '战斗',
  cultivation: '修炼',
  trade: '交易',
  group: '宗门队伍',
}

const sideTabLabels: Array<{ id: SideTab; label: string }> = [
  { id: 'player', label: '人物' },
  { id: 'quests', label: '任务' },
  { id: 'inventory', label: '背包' },
  { id: 'map', label: '地图' },
  { id: 'team', label: '队伍' },
  { id: 'rank', label: '排行' },
]

const dockEntries: DockEntry[] = [
  { key: 'commands', label: '功能', overlay: 'commands' },
  ...sideTabLabels.map((item) => ({
    key: item.id,
    label: item.label,
    overlay: item.id,
  })),
]

const rankingOptions: Array<{ id: 'realm' | 'wealth' | 'combat'; label: string }> = [
  { id: 'realm', label: '境界榜' },
  { id: 'wealth', label: '财富榜' },
  { id: 'combat', label: '战力榜' },
]

const scene = computed(() => store.scene ?? {})
const player = computed(() => store.player ?? {})
const currentSceneId = computed(() => String(scene.value.sceneId ?? ''))
const channelEvents = computed(() => store.events.slice(-4))
const latestEventId = computed(() => store.events[store.events.length - 1]?.eventId)
const exits = computed(() => (scene.value.exits as Record<string, any>[] | undefined) ?? [])
const inventory = computed(() => (player.value.inventory as Record<string, any>[] | undefined) ?? [])
const quests = computed(() => (player.value.quests as Record<string, any>[] | undefined) ?? [])
const npcs = computed(() => (scene.value.npcs as Record<string, any>[] | undefined) ?? [])
const monsters = computed(() => (scene.value.monsters as string[] | undefined) ?? [])
const shops = computed(() => (scene.value.shops as string[] | undefined) ?? [])
const scenePlayers = computed(() => (scene.value.players as Record<string, any>[] | undefined) ?? [])
const sceneItems = computed(() => {
  const items = (scene.value.items as Record<string, any>[] | undefined) ?? []
  if (items.length > 0) {
    return items
  }
  return shops.value.map((name) => ({
    itemId: String(name),
    name: String(name),
    itemType: '',
    description: '',
    source: 'shop',
    price: 0,
  }))
})
const teamMembers = computed(() => (player.value.team?.members as Record<string, any>[] | undefined) ?? [])

const worldMapNodeLookup = new Map(worldMapNodes.map((node) => [node.id, node]))

const mapNodes = computed(() => {
  const linked = new Set(exits.value.map((item) => String(item.targetSceneId ?? '')))
  return worldMapNodes.map((node) => ({
    ...node,
    active: node.id === currentSceneId.value,
    linked: linked.has(node.id),
  }))
})

const mapEdges = computed(() =>
  worldMapEdges
    .map((edge) => {
      const from = worldMapNodeLookup.get(edge.from)
      const to = worldMapNodeLookup.get(edge.to)
      if (!from || !to) {
        return null
      }
      const active = edge.from === currentSceneId.value || edge.to === currentSceneId.value
      return { ...edge, from, to, active }
    })
    .filter((edge): edge is NonNullable<typeof edge> => Boolean(edge)),
)

const commandCategories = computed(() => {
  const currentQuestIds = new Set(quests.value.map((quest) => String(quest.questId ?? '')))

  const social: CommandAction[] = [
    {
      key: 'chat-world',
      label: '世界聊天',
      detail: '默认输入模式，直接发往世界频道。',
      composer: 'chat',
      chatChannel: 'world',
      execute: false,
    },
    {
      key: 'chat-team',
      label: '队伍聊天',
      detail: teamMembers.value.length > 0 ? '发给队伍成员。' : '先创建或加入队伍后使用。',
      composer: 'chat',
      chatChannel: 'team',
      execute: false,
    },
    {
      key: 'event',
      label: '天地异象',
      detail: '查看最近的世界事件。',
      command: 'event',
    },
  ]

  const explore: CommandAction[] = [
    { key: 'look', label: '查看场景', detail: '重读当前场景描述。', command: 'look' },
    { key: 'map', label: '查看地图', detail: '查看整张人界地图。', command: 'map' },
    ...exits.value.map((exit) => ({
      key: `go-${String(exit.direction)}`,
      label: `前往${directionLabel(String(exit.direction))}`,
      detail: `移动到${String(exit.targetSceneName ?? exit.targetSceneId ?? '未知地点')}。`,
      command: `go ${String(exit.direction)}`,
    })),
  ]

  const taskActions: CommandAction[] = [
    ...npcs.value.map((npc) => ({
      key: `talk-${String(npc.npcId)}`,
      label: `交谈·${String(npc.name)}`,
      detail: String(npc.hint ?? '与人物对话，推进剧情或获取任务。'),
      command: `talk ${String(npc.name)}`,
    })),
    ...(sceneQuestOffers[currentSceneId.value] ?? [])
      .filter((quest) => !currentQuestIds.has(quest.id))
      .map((quest) => ({
        key: `accept-${quest.id}`,
        label: `接取·${quest.title}`,
        detail: '接下当前场景的线索任务。',
        command: `accept ${quest.id}`,
      })),
    ...quests.value
      .filter((quest) => String(quest.status ?? '') === 'active')
      .map((quest) => ({
        key: `submit-${String(quest.questId)}`,
        label: `提交·${String(quest.title)}`,
        detail: '条件达成后可提交任务。',
        command: `submit ${String(quest.questId)}`,
      })),
  ]

  const consumables = inventory.value.filter((item) => ['consumable'].includes(String(item.itemType ?? '')))
  const combat: CommandAction[] = [
    ...monsters.value.map((monster) => ({
      key: `fight-${monster}`,
      label: `挑战·${monster}`,
      detail: '与当前场景的妖兽交战。',
      command: `fight ${monster}`,
    })),
    ...consumables.map((item) => ({
      key: `use-${String(item.itemId)}`,
      label: `使用·${String(item.name)}`,
      detail: '恢复或增益状态。',
      command: `use ${String(item.itemId)}`,
    })),
    {
      key: 'flee',
      label: '暂避锋芒',
      detail: '退出战斗节奏，稳住气血。',
      command: 'flee',
    },
  ]

  const cultivation: CommandAction[] = [
    {
      key: 'practice',
      label: `修炼·${String(player.value.cultivation?.primarySkill ?? '长春功')}`,
      detail: '运转本命功法提升修为。',
      command: `practice ${String(player.value.cultivation?.primarySkill ?? '长春功')}`,
    },
    {
      key: 'breakthrough',
      label: '尝试突破',
      detail: '修为充足时冲击下一层境界。',
      command: 'breakthrough',
    },
  ]

  const trade: CommandAction[] = [
    ...shops.value.map((shop) => ({
      key: `buy-${shop}`,
      label: `购买·${shop}`,
      detail: '从当前坊市购入物品。',
      command: `buy ${shop}`,
    })),
    ...inventory.value.map((item) => ({
      key: `sell-${String(item.itemId)}-${String(item.equipped)}`,
      label: `出售·${String(item.name)}`,
      detail: '把背包中的物品卖给坊市。',
      command: `sell ${String(item.itemId)}`,
    })),
  ]

  const group: CommandAction[] = [
    ...(sceneSectOffers[currentSceneId.value] ?? [])
      .filter(() => !player.value.sect?.joined)
      .map((sect) => ({
        key: `join-${sect.command}`,
        label: `加入·${sect.name}`,
        detail: '完成宗门条件后可在此入门。',
        command: sect.command,
      })),
    {
      key: 'team-create',
      label: '创建队伍',
      detail: '自己担任队长，发起组队。',
      command: 'team create',
    },
    {
      key: 'team-info',
      label: '查看队伍',
      detail: '查看当前队伍信息。',
      command: 'team info',
    },
    {
      key: 'team-leave',
      label: '离开队伍',
      detail: '退出当前队伍。',
      command: 'team leave',
    },
    {
      key: 'team-join',
      label: '加入队伍',
      detail: '切到指令输入，填写目标队长账号。',
      command: 'team join ',
      execute: false,
      composer: 'command',
    },
  ]

  return [
    { id: 'social' as const, label: categoryLabels.social, actions: social },
    { id: 'explore' as const, label: categoryLabels.explore, actions: explore },
    { id: 'tasks' as const, label: categoryLabels.tasks, actions: taskActions },
    { id: 'combat' as const, label: categoryLabels.combat, actions: combat },
    { id: 'cultivation' as const, label: categoryLabels.cultivation, actions: cultivation },
    { id: 'trade' as const, label: categoryLabels.trade, actions: trade },
    { id: 'group' as const, label: categoryLabels.group, actions: group },
  ]
})

const activeCommandActions = computed(
  () => commandCategories.value.find((category) => category.id === activeCommandCategory.value)?.actions ?? [],
)

const activeCommandCategoryLabel = computed(
  () => commandCategories.value.find((category) => category.id === activeCommandCategory.value)?.label ?? '交流',
)

const activeInfoTab = computed<SideTab>(() =>
  activeOverlay.value !== 'none' && activeOverlay.value !== 'commands' && activeOverlay.value !== 'scene'
    ? activeOverlay.value
    : activeTab.value,
)

const sceneInteractables = computed<SceneInteractable[]>(() => {
  const entries: SceneInteractable[] = []
  const activeQuestList = quests.value.filter((quest) => String(quest.status ?? '') === 'active')
  const currentQuestIds = new Set(quests.value.map((quest) => String(quest.questId ?? '')))
  const availableQuestOffers = (sceneQuestOffers[currentSceneId.value] ?? []).filter((quest) => !currentQuestIds.has(quest.id))
  const sectOffers = (sceneSectOffers[currentSceneId.value] ?? []).filter(() => !player.value.sect?.joined)
  const primarySkill = String(player.value.cultivation?.primarySkill ?? '长春功')
  const activeSceneQuest = activeQuestList.find((quest) =>
    (sceneQuestOffers[currentSceneId.value] ?? []).some((offer) => offer.id === String(quest.questId ?? '')),
  )
  const teamAccountSet = new Set(teamMembers.value.map((member) => String(member.account ?? '')))

  scenePlayers.value.forEach((scenePlayer) => {
    const playerAccount = String(scenePlayer.account ?? '')
    const displayName = String(scenePlayer.characterName ?? scenePlayer.account ?? '无名修士')
    const sameTeam = teamAccountSet.has(playerAccount)
    const playerActions: CommandAction[] = [
      {
        key: `interactable-chat-world-${playerAccount}`,
        label: '频道招呼',
        detail: '切到世界聊天，并预填对方称呼。',
        composer: 'chat',
        chatChannel: 'world',
        prefillText: `@${displayName} `,
      },
    ]

    if (sameTeam) {
      playerActions.push({
        key: `interactable-chat-team-${playerAccount}`,
        label: '队伍交流',
        detail: '切到队伍频道，方便和队友同步。',
        composer: 'chat',
        chatChannel: 'team',
        prefillText: `@${displayName} `,
      })
    }

    entries.push({
      key: `interactable-player-${playerAccount}`,
      kind: 'player',
      railLabel: '玩家',
      railCaption: shortenText(displayName, 5),
      title: displayName,
      subtitle: sameTeam ? '同场景队友' : '同场景玩家',
      description: `${displayName}也在此地行动，与你共享这一片场景视野。`,
      meta: [
        `账号：${playerAccount || '未知'}`,
        `境界：${String(scenePlayer.realmName ?? '凡躯')}`,
        `宗门：${String(scenePlayer.sectName ?? '散修')}`,
        String(scenePlayer.title ?? '') ? `称号：${String(scenePlayer.title)}` : '',
      ].filter(Boolean) as string[],
      actions: playerActions,
    })
  })

  npcs.value.forEach((npc, index) => {
    const npcName = String(npc.name ?? '无名修士')
    const npcActions: CommandAction[] = [
      {
        key: `interactable-talk-${String(npc.npcId ?? npcName)}`,
        label: `交谈·${npcName}`,
        detail: '先与此人交谈，打探消息和后续线索。',
        command: `talk ${npcName}`,
      },
    ]
    const questOffer = Boolean(npc.hasQuest) ? availableQuestOffers[0] : undefined

    if (questOffer) {
      npcActions.push({
        key: `interactable-npc-accept-${String(npc.npcId ?? npcName)}-${questOffer.id}`,
        label: `接取·${questOffer.title}`,
        detail: '顺着这位人物给出的线索继续推进。',
        command: `accept ${questOffer.id}`,
      })
    }

    if (Boolean(npc.hasQuest) && activeSceneQuest) {
      npcActions.push({
        key: `interactable-npc-submit-${String(npc.npcId ?? npcName)}-${String(activeSceneQuest.questId ?? '')}`,
        label: `提交·${String(activeSceneQuest.title ?? '任务')}`,
        detail: '若材料已齐，可直接向此人交付。',
        command: `submit ${String(activeSceneQuest.questId ?? '')}`,
      })
    }

    if (index === 0 && sectOffers.length > 0) {
      const sect = sectOffers[0]
      npcActions.push({
        key: `interactable-join-${sect.command}`,
        label: `拜入·${sect.name}`,
        detail: '若条件足够，可当场拜入宗门。',
        command: sect.command,
      })
    }

    entries.push({
      key: `interactable-npc-${String(npc.npcId ?? npcName)}`,
      kind: 'npc',
      railLabel: '人物',
      railCaption: shortenText(npcName, 5),
      title: npcName,
      subtitle: '视野可见人物',
      description: String(npc.hint ?? `${npcName}正驻留在这里，也许愿意和你多说几句。`),
      meta: [
        questOffer
          ? `可接任务：${questOffer.title}`
          : activeSceneQuest && Boolean(npc.hasQuest)
            ? `可提交任务：${String(activeSceneQuest.title ?? '当前任务')}`
            : Boolean(npc.hasQuest)
              ? '身上似有任务线索'
              : '可继续交谈打探消息',
        `所在场景：${String(scene.value.sceneName ?? '此地')}`,
      ],
      actions: npcActions,
    })
  })
  
  sceneItems.value.forEach((sceneItem) => {
    const itemId = String(sceneItem.itemId ?? sceneItem.name ?? '')
    const itemName = String(sceneItem.name ?? itemId ?? '无名物件') || '无名物件'
    const itemSource = String(sceneItem.source ?? 'shop')
    const itemType = String(sceneItem.itemType ?? '杂物') || '杂物'
    const itemActions: CommandAction[] = []
    if (itemSource === 'shop') {
      itemActions.push({
        key: `interactable-buy-${itemId}`,
        label: `购买·${itemName}`,
        detail: '从眼前摊位直接买下这件物件。',
        command: `buy ${itemId || itemName}`,
      })
    }

    entries.push({
      key: `interactable-shop-${itemId}-${itemSource}`,
      kind: 'shop',
      railLabel: '物件',
      railCaption: shortenText(itemName, 5),
      title: itemName,
      subtitle: sceneItemSourceLabel(itemSource),
      description:
        String(sceneItem.description ?? '') || `这件物品正处在你的视野范围内，可以先查看，再决定是否出手。`,
      meta: [
        `类别：${itemType}`,
        `来源：${sceneItemSourceLabel(itemSource)}`,
        Number(sceneItem.price ?? 0) > 0 ? `价格：${Number(sceneItem.price)} 灵石` : '',
      ].filter(Boolean) as string[],
      actions: itemActions,
    })
  })

  monsters.value.forEach((monster) => {
    entries.push({
      key: `interactable-monster-${monster}`,
      kind: 'monster',
      railLabel: '妖兽',
      railCaption: shortenText(monster, 5),
      title: monster,
      subtitle: '视野可见妖兽',
      description: `${monster}正在附近徘徊，若你想练手或搜集战利品，可以主动上前挑战。`,
      meta: ['可发起战斗', '战后可能获得掉落或修为'],
      actions: [
        {
          key: `interactable-fight-${monster}`,
          label: `挑战·${monster}`,
          detail: '立即进入战斗，检验当前战力。',
          command: `fight ${monster}`,
        },
        {
          key: `interactable-practice-${monster}`,
          label: '先行调息',
          detail: '先运转功法稳住状态，再考虑出手。',
          command: `practice ${primarySkill}`,
        },
      ],
    })
  })

  return entries
})

const selectedSceneInteractable = computed(
  () =>
    sceneInteractables.value.find((item) => item.key === selectedSceneInteractableKey.value) ??
    sceneInteractables.value[0] ??
    null,
)

const activeOverlayTitle = computed(() => {
  if (activeOverlay.value === 'commands') {
    return activeCommandCategoryLabel.value
  }
  if (activeOverlay.value === 'scene') {
    return selectedSceneInteractable.value?.title ?? '场景交互'
  }
  if (activeOverlay.value === 'none') {
    return ''
  }
  return sideTabLabels.find((item) => item.id === activeInfoTab.value)?.label ?? ''
})

const activeOverlayCorner = computed(() => {
  if (activeOverlay.value === 'commands') {
    return '功能盘'
  }
  if (activeOverlay.value === 'scene') {
    return sceneInteractableKindLabel(selectedSceneInteractable.value?.kind ?? 'npc')
  }
  return scene.value.sceneName || '修行界'
})

const composerTitle = computed(() =>
  composerMode.value === 'chat'
    ? chatChannel.value === 'world'
      ? '世界聊天'
      : '队伍聊天'
    : '指令输入',
)

const composerPlaceholder = computed(() => {
  if (composerMode.value === 'chat') {
    return chatChannel.value === 'world' ? '直接输入想说的话，默认发往世界频道' : '输入要发送给队友的话'
  }
  return '这里输入具体指令，或点击上方页签按钮自动填入'
})

const sceneMissionText = computed(() => {
  const activeQuest = quests.value.find((quest) => String(quest.status ?? '') === 'active')
  if (activeQuest) {
    return `当前主线：${String(activeQuest.title)}，进度 ${String(activeQuest.progress ?? 0)} / ${String(activeQuest.target ?? 0)}。`
  }

  const questOffer = (sceneQuestOffers[currentSceneId.value] ?? []).find(
    (quest) => !quests.value.some((item) => String(item.questId ?? '') === quest.id),
  )
  if (questOffer) {
    return `此地有新线索可接：${questOffer.title}。`
  }

  return '多与场景人物交谈，寻找下一段机缘。'
})

function shortenText(value: string, limit = 4) {
  const normalized = value.trim()
  if (normalized.length <= limit) {
    return normalized
  }
  return `${normalized.slice(0, limit)}…`
}

function splitDenseText(text: string, limit = 4) {
  const normalized = text.replace(/\s+/g, ' ').trim()
  if (!normalized) {
    return []
  }

  const parts = normalized
    .split(/(?<=[。！？；])/u)
    .map((part) => part.trim())
    .filter(Boolean)

  return (parts.length > 0 ? parts : [normalized]).slice(0, limit)
}

const sceneTranscript = computed<DenseLine[]>(() => {
  const lines: DenseLine[] = []
  const regionName = String(scene.value.regionName ?? '')
  const sceneName = String(scene.value.sceneName ?? '')
  const sceneDescription = String(scene.value.description ?? '')

  if (regionName || sceneName) {
    lines.push({
      key: `scene-place-${currentSceneId.value}`,
      tag: '场景',
      text: [regionName, sceneName].filter(Boolean).join(' · '),
      tone: 'system',
    })
  }

  splitDenseText(sceneDescription).forEach((text, index) => {
    lines.push({
      key: `scene-description-${currentSceneId.value}-${index}`,
      tag: '场景',
      text,
      tone: 'system',
    })
  })

  lines.push({
    key: `scene-mission-${currentSceneId.value}`,
    tag: '任务',
    text: sceneMissionText.value,
    tone: 'quest',
  })

  if (npcs.value.length > 0) {
    lines.push({
      key: `scene-npcs-${currentSceneId.value}`,
      tag: '人物',
      text: `你看见 ${npcs.value.map((npc) => String(npc.name)).join('、')}。`,
      tone: 'hint',
    })
  }

  if (scenePlayers.value.length > 0) {
    lines.push({
      key: `scene-players-${currentSceneId.value}`,
      tag: '玩家',
      text: `同场景还有 ${scenePlayers.value.map((entry) => String(entry.characterName ?? entry.account)).join('、')}。`,
      tone: 'chat',
    })
  }

  if (monsters.value.length > 0) {
    lines.push({
      key: `scene-monsters-${currentSceneId.value}`,
      tag: '战报',
      text: `附近徘徊着 ${monsters.value.join('、')}。`,
      tone: 'combat',
    })
  }

  if (sceneItems.value.length > 0) {
    lines.push({
      key: `scene-items-${currentSceneId.value}`,
      tag: '系统',
      text: `视野里可见 ${sceneItems.value.map((entry) => String(entry.name ?? entry.itemId)).join('、')}。`,
      tone: 'system',
    })
  }

  if (store.lastResult?.title || store.lastResult?.summary) {
    const resultText = [String(store.lastResult?.title ?? ''), String(store.lastResult?.summary ?? '')]
      .filter(Boolean)
      .join('：')
    lines.push({
      key: `result-${String(store.nextEventId)}`,
      tag: '结果',
      text: resultText,
      tone: 'hint',
    })
  }

  ;((store.lastResult?.hints as string[] | undefined) ?? []).slice(0, 2).forEach((hint, index) => {
    lines.push({
      key: `result-hint-${String(store.nextEventId)}-${index}`,
      tag: '提示',
      text: hint,
      tone: 'hint',
    })
  })

  return lines.slice(-30)
})

const quickStats = computed(() => [
  {
    key: 'hp',
    label: '气血',
    value: `${String(player.value.hp ?? 0)}/${String(player.value.maxHp ?? 0)}`,
  },
  {
    key: 'attack',
    label: '攻击',
    value: String(player.value.attackPower ?? 0),
  },
  {
    key: 'stone',
    label: '灵石',
    value: String(player.value.spiritStone ?? 0),
  },
  {
    key: 'exp',
    label: '修为',
    value: String(player.value.cultivation?.exp ?? 0),
  },
])

const infoOverview = computed(() => [
  {
    key: 'name',
    label: '角色',
    value: String(player.value.characterName ?? '未命名'),
  },
  {
    key: 'sect',
    label: '宗门',
    value: String(player.value.sect?.sectName ?? '散修'),
  },
  {
    key: 'quests',
    label: '任务',
    value: `${quests.value.filter((quest) => String(quest.status ?? '') === 'active').length} 条`,
  },
  {
    key: 'bag',
    label: '背包',
    value: `${inventory.value.length} 格`,
  },
])

const orientationTiles = computed<OrientationTile[]>(() => {
  const slotDirections: Array<{
    slot: OrientationTile['slot']
    directions: string[]
    emptyLabel: string
  }> = [
    { slot: 'north', directions: ['north', 'up'], emptyLabel: '前路未显' },
    { slot: 'west', directions: ['west'], emptyLabel: '左侧无路' },
    { slot: 'east', directions: ['east'], emptyLabel: '右侧无路' },
    { slot: 'south', directions: ['south', 'down'], emptyLabel: '后路未开' },
  ]

  const tiles: OrientationTile[] = [
    {
      key: `orientation-center-${currentSceneId.value}`,
      slot: 'center',
      label: String(scene.value.sceneName ?? '当前位置'),
      caption: String(scene.value.regionName ?? '当前场景'),
      command: 'look',
    },
  ]

  slotDirections.forEach((slotConfig) => {
    const exit = exits.value.find((item) => slotConfig.directions.includes(String(item.direction ?? '')))
    if (exit) {
      const direction = String(exit.direction ?? '')
      tiles.push({
        key: `orientation-${slotConfig.slot}-${direction}`,
        slot: slotConfig.slot,
        label: String(exit.targetSceneName ?? exit.targetSceneId ?? directionLabel(direction)),
        caption: `${directionLabel(direction)}方`,
        command: `go ${direction}`,
      })
      return
    }

    tiles.push({
      key: `orientation-${slotConfig.slot}-empty`,
      slot: slotConfig.slot,
      label: slotConfig.emptyLabel,
      caption: '暂无去路',
      disabled: true,
    })
  })

  return tiles
})

const orientationExtraExits = computed(() =>
  exits.value.filter((item) => !['north', 'south', 'east', 'west', 'up', 'down'].includes(String(item.direction ?? ''))),
)

function setError(error: unknown) {
  store.error = error instanceof Error ? error.message : '发生未知错误'
}

function directionLabel(value: string) {
  return directionLabelMap[value] ?? value
}

function eventTone(event: Record<string, any>) {
  const text = `${String(event.title ?? '')} ${String(event.content ?? '')}`
  if (/战|斗|妖兽|击|伤|胜|败/.test(text)) {
    return 'combat'
  }
  if (/任务|采药|提交|接取|线索|主线/.test(text)) {
    return 'quest'
  }
  if (/聊|说|频道|world|team|队伍/.test(text)) {
    return 'chat'
  }
  return 'system'
}

function denseToneClass(tone: DenseLine['tone']) {
  return `tone-${tone}`
}

function eventToneClass(event: Record<string, any>) {
  return denseToneClass(eventTone(event) as DenseLine['tone'])
}

function eventChannelLabel(event: Record<string, any>) {
  const tone = eventTone(event)
  if (tone === 'combat') {
    return '战报'
  }
  if (tone === 'quest') {
    return '任务'
  }
  if (tone === 'chat') {
    return '闲聊'
  }
  return '系统'
}

function questStatusLabel(value: string) {
  const labels: Record<string, string> = {
    active: '进行中',
    completed: '已达成',
    submitted: '已提交',
    failed: '已失败',
  }
  return labels[value] ?? value
}

function sceneInteractableKindLabel(kind: SceneInteractableKind) {
  const labels: Record<SceneInteractableKind, string> = {
    player: '玩家',
    npc: '人物',
    shop: '物件',
    monster: '妖兽',
  }
  return labels[kind] ?? '交互'
}

function sceneItemSourceLabel(source: string) {
  const labels: Record<string, string> = {
    shop: '摊位物件',
    ground: '地面物件',
  }
  return labels[source] ?? '可见物件'
}

function openSceneInteractable(item: SceneInteractable) {
  selectedSceneInteractableKey.value = item.key
  activeOverlay.value = 'scene'
}

function commandCategoryClass(id: CommandCategoryId) {
  return `command-tone-${id}`
}

function sideTabClass(id: SideTab) {
  return `dock-tone-${id}`
}

function dockEntryClass(entry: DockEntry) {
  return entry.overlay === 'commands' ? 'dock-tone-commands' : sideTabClass(entry.overlay)
}

function sceneInteractableClass(item: SceneInteractable) {
  return [
    `rail-button--${item.kind}`,
    {
      active: activeOverlay.value === 'scene' && selectedSceneInteractable.value?.key === item.key,
    },
  ]
}

function orientationTileClass(tile: OrientationTile) {
  return [`orientation-node--${tile.slot}`, { 'orientation-node--disabled': tile.disabled, 'orientation-node--center': tile.slot === 'center' }]
}

function sceneInteractableToneClass(kind: SceneInteractableKind) {
  return `scene-entity-tone-${kind}`
}

function openOverlay(overlay: Exclude<OverlayPanel, 'none'>) {
  if (overlay !== 'commands' && overlay !== 'scene') {
    activeTab.value = overlay
  }
  activeOverlay.value = overlay
}

function closeOverlay() {
  activeOverlay.value = 'none'
}

async function login(autoRegister = false) {
  try {
    await store.loginFlow(account.value, password.value, autoRegister)
  } catch (error) {
    setError(error)
  }
}

async function createCharacter() {
  try {
    await store.createCharacter(characterName.value)
    characterName.value = ''
  } catch (error) {
    setError(error)
  }
}

async function submitComposer(commandOverride?: string) {
  const raw = commandOverride ?? composerText.value.trim()
  if (!raw) {
    return
  }

  const nextCommand =
    commandOverride ?? (composerMode.value === 'chat' ? `chat ${chatChannel.value} ${raw}` : raw)

  try {
    await store.executeCommand(nextCommand)
    closeOverlay()
    if (!commandOverride) {
      composerText.value = ''
    }
  } catch (error) {
    setError(error)
  }
}

function applyAction(action: CommandAction) {
  if (action.composer === 'chat') {
    composerMode.value = 'chat'
    chatChannel.value = action.chatChannel ?? 'world'
    composerText.value = action.prefillText ?? ''
    closeOverlay()
    return
  }

  if (!action.command) {
    return
  }

  if (action.execute === false || action.command.endsWith(' ')) {
    composerMode.value = 'command'
    composerText.value = action.prefillText ?? action.command
    closeOverlay()
    return
  }

  closeOverlay()
  void submitComposer(action.command)
}

async function loadRanking(kind: 'realm' | 'wealth' | 'combat') {
  try {
    await store.loadRankings(kind)
  } catch (error) {
    setError(error)
  }
}

async function scrollTextPanelsToBottom() {
  await nextTick()
  for (const viewport of [eventViewport.value, storyViewport.value]) {
    if (!viewport) {
      continue
    }
    viewport.scrollTop = viewport.scrollHeight
  }
}

onMounted(async () => {
  if (!store.authenticated) {
    return
  }

  try {
    await store.bootstrap()
    await scrollTextPanelsToBottom()
  } catch (error) {
    setError(error)
  }
})

watch(activeOverlay, async (value) => {
  if (value !== 'rank' || !store.authenticated) {
    return
  }

  try {
    await store.loadRankings(store.rankingType)
  } catch (error) {
    setError(error)
  }
})

watch(
  [
    () => currentSceneId.value,
    () => latestEventId.value,
    () => store.lastResult?.summary,
  ],
  () => {
    void scrollTextPanelsToBottom()
  },
  { flush: 'post' },
)

watch(
  () => sceneInteractables.value.map((item) => item.key).join('|'),
  () => {
    if (sceneInteractables.value.some((item) => item.key === selectedSceneInteractableKey.value)) {
      return
    }
    selectedSceneInteractableKey.value = sceneInteractables.value[0]?.key ?? ''
  },
  { immediate: true },
)
</script>

<template>
  <div class="shell" :class="{ 'shell--game': store.authenticated && !store.needCreateCharacter }">
    <header v-if="store.authenticated && !store.needCreateCharacter" class="top-banner">
      <div class="brand-copy">
        <p class="eyebrow">人界修行中</p>
        <h1>凡人修仙录</h1>
      </div>
      <div class="top-banner-actions">
        <div class="status-chip">
          <span>账号</span>
          <strong>{{ store.account }}</strong>
        </div>
        <button type="button" class="ghost-button compact-button" @click="store.logout()">退出</button>
      </div>
    </header>

    <header v-else class="hero">
      <div class="brand-copy">
        <p class="eyebrow">网页客户端 · 文字修仙</p>
        <h1>凡人修仙传 · 人界修行录</h1>
        <p class="subtitle">从七玄门入世，沿着嘉元城、黄枫谷、乱星海一路修行，完整体验人界主线。</p>
      </div>
    </header>

    <p v-if="store.error" class="error-banner">{{ store.error }}</p>

    <section v-if="!store.authenticated" class="auth-card">
      <div class="card-heading">
        <h2>进入凡人世界</h2>
        <p>沿用现有服务端协议，登录后直接进入移动端主界面。</p>
      </div>
      <div class="form-grid">
        <label>
          <span>账号名</span>
          <input v-model="account" autocomplete="username" placeholder="例如 韩立001" />
        </label>
        <label>
          <span>登录密码</span>
          <input v-model="password" type="password" autocomplete="current-password" placeholder="请输入密码" />
        </label>
      </div>
      <div class="action-row">
        <button type="button" class="primary-button" :disabled="store.loading" @click="login(false)">登录</button>
        <button type="button" class="secondary-button" :disabled="store.loading" @click="login(true)">注册并登录</button>
      </div>
    </section>

    <section v-else-if="store.needCreateCharacter" class="auth-card">
      <div class="card-heading">
        <h2>塑造新角色</h2>
        <p>创建后自动带上新手任务“墨府采药”，可直接开始第一段剧情。</p>
      </div>
      <div class="form-grid single-column">
        <label>
          <span>角色名</span>
          <input v-model="characterName" maxlength="24" placeholder="例如 韩立" @keyup.enter="createCharacter()" />
        </label>
      </div>
      <div class="action-row">
        <button type="button" class="primary-button" :disabled="store.loading" @click="createCharacter()">踏入修仙路</button>
      </div>
    </section>

    <main v-else class="mobile-layout">
      <section class="surface-panel event-panel">
        <div class="channel-header">
          <span class="channel-badge">频道</span>
          <div class="channel-meta">
            <span class="channel-chip">{{ scene.regionName || '未知地域' }}</span>
            <span class="channel-chip">{{ scene.sceneName || '未知场景' }}</span>
            <span class="channel-chip">{{ player.cultivation?.realmName || '凡躯' }}</span>
          </div>
        </div>
        <div ref="eventViewport" class="event-stream dense-log">
          <article v-for="event in channelEvents" :key="event.eventId" class="event-item dense-item">
            <span class="event-tag" :class="eventToneClass(event)">{{ eventChannelLabel(event) }}</span>
            <p class="event-line">
              {{ [event.title, event.content].filter(Boolean).join('：') }}
            </p>
          </article>
          <p v-if="channelEvents.length === 0" class="empty-text">暂时没有新的消息，先去和周围人物说说话吧。</p>
        </div>
      </section>

      <section class="surface-panel scene-panel">
        <div class="section-row scene-section-header">
          <div>
            <p class="section-kicker">主界面</p>
            <h2>{{ scene.sceneName || '修行记录' }}</h2>
          </div>
          <div class="scene-header-actions">
            <button type="button" class="ghost-button tiny-button" @click="openOverlay('player')">人物</button>
            <button type="button" class="ghost-button tiny-button" @click="openOverlay('map')">地图</button>
            <button type="button" class="ghost-button tiny-button" @click="submitComposer('look')">重看</button>
          </div>
        </div>
        <div class="scene-frame">
          <aside class="scene-side-rail">
            <p v-if="sceneInteractables.length === 0" class="scene-side-empty">当前视野里暂无明显目标</p>
            <button
              v-for="item in sceneInteractables"
              :key="item.key"
              type="button"
              class="rail-button"
              :class="sceneInteractableClass(item)"
              @click="openSceneInteractable(item)"
            >
              <span class="rail-label">{{ item.railLabel }}</span>
              <small class="rail-caption">{{ item.railCaption }}</small>
            </button>
          </aside>

          <div class="scene-main-board">
            <div class="scene-board-header">
              <div>
                <p class="scene-board-title">{{ scene.regionName || '修行界' }}</p>
                <p class="scene-board-subtitle">{{ sceneMissionText }}</p>
              </div>
              <div class="scene-board-tools">
                <button type="button" class="mini-tool-button" @click="activeCommandCategory = 'social'; openOverlay('commands')">闲聊</button>
                <button type="button" class="mini-tool-button" @click="activeCommandCategory = 'tasks'; openOverlay('commands')">任务</button>
                <button type="button" class="mini-tool-button" @click="activeCommandCategory = 'explore'; openOverlay('commands')">附近</button>
              </div>
            </div>

            <div class="story-console">
              <div ref="storyViewport" class="story-log">
                <article
                  v-for="line in sceneTranscript"
                  :key="line.key"
                  class="story-line"
                  :class="denseToneClass(line.tone)"
                >
                  <span class="story-tag">[{{ line.tag }}]</span>
                  <p class="story-text">{{ line.text }}</p>
                </article>
              </div>
            </div>

            <div class="scene-presence-strip">
              <span class="meta-tag" v-for="entry in scenePlayers" :key="entry.account">玩家 · {{ entry.characterName }}</span>
              <span class="meta-tag" v-for="npc in npcs" :key="npc.npcId">人物 · {{ npc.name }}</span>
              <span class="meta-tag" v-for="sceneItem in sceneItems" :key="`${sceneItem.itemId}-${sceneItem.source}`">物件 · {{ sceneItem.name }}</span>
              <span class="meta-tag" v-for="monster in monsters" :key="monster">妖兽 · {{ monster }}</span>
            </div>

            <div class="scene-orientation-card">
              <div class="scene-orientation-frame">
                <button
                  v-for="tile in orientationTiles"
                  :key="tile.key"
                  type="button"
                  class="orientation-node"
                  :class="orientationTileClass(tile)"
                  :disabled="tile.disabled"
                  @click="tile.command ? submitComposer(tile.command) : undefined"
                >
                  <span class="orientation-label">{{ tile.label }}</span>
                  <small class="orientation-caption">{{ tile.caption }}</small>
                </button>
              </div>
              <div v-if="orientationExtraExits.length > 0" class="orientation-extra-row">
                <button
                  v-for="exit in orientationExtraExits"
                  :key="`extra-${exit.direction}`"
                  type="button"
                  class="orientation-extra-button"
                  @click="submitComposer(`go ${String(exit.direction)}`)"
                >
                  <span>{{ directionLabel(String(exit.direction)) }}方</span>
                  <strong>{{ exit.targetSceneName }}</strong>
                </button>
              </div>
            </div>
          </div>
        </div>
      </section>

      <footer class="bottom-dock">
        <div class="vitals-row">
          <article v-for="stat in quickStats" :key="stat.key" class="vital-card" :class="stat.key">
            <span>{{ stat.label }}</span>
            <strong>{{ stat.value }}</strong>
          </article>
        </div>

        <div class="dock-nav-grid">
          <button
            v-for="entry in dockEntries"
            :key="entry.key"
            type="button"
            class="dock-nav-button"
            :class="[dockEntryClass(entry), { active: activeOverlay === entry.overlay }]"
            @click="openOverlay(entry.overlay)"
          >
            {{ entry.label }}
          </button>
        </div>

        <div class="mode-row">
          <button
            type="button"
            class="mode-button"
            :class="{ active: composerMode === 'chat' && chatChannel === 'world' }"
            @click="applyAction({ key: 'chat-world', label: '', detail: '', composer: 'chat', chatChannel: 'world' })"
          >
            世界聊天
          </button>
          <button
            type="button"
            class="mode-button"
            :class="{ active: composerMode === 'chat' && chatChannel === 'team' }"
            @click="applyAction({ key: 'chat-team', label: '', detail: '', composer: 'chat', chatChannel: 'team' })"
          >
            队伍聊天
          </button>
          <button
            type="button"
            class="mode-button"
            :class="{ active: composerMode === 'command' }"
            @click="composerMode = 'command'"
          >
            原始指令
          </button>
        </div>

        <div class="composer-caption">{{ composerTitle }}</div>
        <form class="command-form" @submit.prevent="submitComposer()">
          <input v-model="composerText" :placeholder="composerPlaceholder" :disabled="store.loading" />
          <button class="primary-button" :disabled="store.loading">
            {{ composerMode === 'chat' ? '发送' : '执行' }}
          </button>
        </form>
      </footer>
    </main>

    <div v-if="store.authenticated && activeOverlay !== 'none'" class="overlay-backdrop" @click.self="closeOverlay()">
      <section class="overlay-sheet" :class="{ 'overlay-sheet--wide': activeOverlay === 'map' || activeOverlay === 'commands' }">
        <div class="overlay-grabber" aria-hidden="true"></div>
        <header class="overlay-header">
          <div>
            <p class="section-kicker">弹出界面</p>
            <h2>{{ activeOverlayTitle }}</h2>
          </div>
          <div class="overlay-header-actions">
            <span class="panel-corner-label">{{ activeOverlayCorner }}</span>
            <button type="button" class="ghost-button tiny-button" @click="closeOverlay()">关闭</button>
          </div>
        </header>

        <div v-if="activeOverlay === 'commands'" class="overlay-body overlay-body--commands">
          <div class="tray-summary-row overlay-summary-row">
            <article v-for="item in infoOverview" :key="item.key" class="tray-summary-card">
              <span>{{ item.label }}</span>
              <strong>{{ item.value }}</strong>
            </article>
          </div>
          <div class="tab-strip command-tab-strip">
            <button
              v-for="category in commandCategories"
              :key="category.id"
              type="button"
              class="tab-button command-tab-button"
              :class="[commandCategoryClass(category.id), { active: activeCommandCategory === category.id }]"
              @click="activeCommandCategory = category.id"
            >
              {{ category.label }}
            </button>
          </div>
          <div class="command-grid overlay-command-grid">
            <button
              v-for="action in activeCommandActions"
              :key="action.key"
              type="button"
              class="command-card"
              :class="commandCategoryClass(activeCommandCategory)"
              @click="applyAction(action)"
            >
              <span class="command-card-title">{{ action.label }}</span>
              <span class="command-card-detail">{{ action.detail }}</span>
            </button>
            <p v-if="activeCommandActions.length === 0" class="empty-text">这一栏暂时没有可用动作。</p>
          </div>
        </div>

        <div v-else-if="activeOverlay === 'scene'" class="overlay-body overlay-body--scene">
          <div class="tab-strip scene-entity-tabs">
            <button
              v-for="item in sceneInteractables"
              :key="item.key"
              type="button"
              class="tab-button command-tab-button scene-entity-tab-button"
              :class="[sceneInteractableToneClass(item.kind), { active: selectedSceneInteractable?.key === item.key }]"
              @click="openSceneInteractable(item)"
            >
              {{ item.railCaption }}
            </button>
          </div>

          <div v-if="selectedSceneInteractable" class="detail-stack">
            <article class="detail-card scene-entity-card">
              <div class="scene-entity-header">
                <span class="scene-entity-kind" :class="sceneInteractableToneClass(selectedSceneInteractable.kind)">
                  {{ sceneInteractableKindLabel(selectedSceneInteractable.kind) }}
                </span>
                <div class="scene-entity-copy">
                  <p class="detail-title">{{ selectedSceneInteractable.title }}</p>
                  <p class="scene-entity-subtitle">{{ selectedSceneInteractable.subtitle }}</p>
                </div>
              </div>
              <p class="scene-entity-description">{{ selectedSceneInteractable.description }}</p>
              <div class="scene-entity-meta">
                <span v-for="meta in selectedSceneInteractable.meta" :key="meta" class="meta-tag subtle-tag">{{ meta }}</span>
              </div>
            </article>

            <div class="command-grid scene-entity-action-grid">
              <button
                v-for="action in selectedSceneInteractable.actions"
                :key="action.key"
                type="button"
                class="command-card scene-entity-action-card"
                :class="sceneInteractableToneClass(selectedSceneInteractable.kind)"
                @click="applyAction(action)"
              >
                <span class="command-card-title">{{ action.label }}</span>
                <span class="command-card-detail">{{ action.detail }}</span>
              </button>
              <p v-if="selectedSceneInteractable.actions.length === 0" class="empty-text">当前没有可直接执行的操作。</p>
            </div>
          </div>

          <p v-else class="empty-text">当前场景暂无明显可见目标。</p>
        </div>

        <div v-else class="overlay-body">
          <div class="tab-strip overlay-info-tabs">
            <button
              v-for="item in sideTabLabels"
              :key="item.id"
              type="button"
              class="tab-button command-tab-button"
              :class="{ active: activeInfoTab === item.id }"
              @click="openOverlay(item.id)"
            >
              {{ item.label }}
            </button>
          </div>

          <div v-if="activeInfoTab === 'player'" class="detail-stack">
            <div class="stat-grid">
              <article class="stat-card">
                <span>角色</span>
                <strong>{{ player.characterName }}</strong>
              </article>
              <article class="stat-card">
                <span>气血</span>
                <strong>{{ player.hp }} / {{ player.maxHp }}</strong>
              </article>
              <article class="stat-card">
                <span>攻击</span>
                <strong>{{ player.attackPower }}</strong>
              </article>
              <article class="stat-card">
                <span>防御</span>
                <strong>{{ player.defensePower }}</strong>
              </article>
              <article class="stat-card">
                <span>灵石</span>
                <strong>{{ player.spiritStone }}</strong>
              </article>
              <article class="stat-card">
                <span>宗门</span>
                <strong>{{ player.sect?.sectName || '散修' }}</strong>
              </article>
            </div>
            <div class="info-block">
              <p>本命功法：{{ player.cultivation?.primarySkill || '长春功' }}</p>
              <p>功法等级：{{ player.cultivation?.skillLevel || 1 }}</p>
              <p>突破需求：{{ player.cultivation?.exp || 0 }} / {{ player.cultivation?.nextBreakthroughExp || 0 }}</p>
              <p>章节进度：{{ player.progressionChapter || '七玄门启程' }}</p>
              <p>宗门贡献：{{ player.sectContribution || 0 }}</p>
              <p>已解锁区域：{{ (player.unlockedRegions || []).join('、') || '七玄门' }}</p>
            </div>
          </div>

          <div v-else-if="activeInfoTab === 'quests'" class="detail-stack">
            <article v-for="quest in quests" :key="quest.questId" class="detail-card">
              <p class="detail-title">{{ quest.title }}</p>
              <p>{{ quest.description }}</p>
              <p>状态：{{ questStatusLabel(String(quest.status ?? '')) }} · 进度：{{ quest.progress }} / {{ quest.target }}</p>
            </article>
            <p v-if="quests.length === 0" class="empty-text">当前暂无任务，试着去和场景人物交谈。</p>
          </div>

          <div v-else-if="activeInfoTab === 'inventory'" class="detail-stack">
            <article v-for="item in inventory" :key="`${item.itemId}-${item.equipped}`" class="detail-card">
              <p class="detail-title">{{ item.name }}</p>
              <p>{{ item.description }}</p>
              <p>数量：{{ item.quantity }} <span v-if="item.equipped">· 已装备</span></p>
            </article>
            <p v-if="inventory.length === 0" class="empty-text">背包空空如也。</p>
          </div>

          <div v-else-if="activeInfoTab === 'team'" class="detail-stack">
            <article class="detail-card">
              <p class="detail-title">{{ player.team?.teamName || '暂无队伍' }}</p>
              <p v-if="teamMembers.length === 0">去“功能”里的宗门队伍页签创建队伍，或输入 `team join 账号` 加入他人队伍。</p>
              <p v-else>队伍编号：{{ player.team?.teamId }}</p>
            </article>
            <article v-for="member in teamMembers" :key="member.account" class="detail-card">
              <p class="detail-title">{{ member.displayName }}</p>
              <p>{{ member.account }} <span v-if="member.leader">· 队长</span></p>
            </article>
          </div>

          <div v-else-if="activeInfoTab === 'rank'" class="detail-stack">
            <div class="ranking-row">
              <button
                v-for="option in rankingOptions"
                :key="option.id"
                type="button"
                class="ghost-button small-button"
                :class="{ active: store.rankingType === option.id }"
                @click="loadRanking(option.id)"
              >
                {{ option.label }}
              </button>
            </div>
            <article v-for="entry in store.rankings" :key="`${store.rankingType}-${entry.rank}`" class="detail-card">
              <p class="detail-title">第 {{ entry.rank }} 名 · {{ entry.characterName }}</p>
              <p>{{ entry.account }} · {{ entry.realmName }} · {{ entry.sectName || '散修' }}</p>
              <p>等级 {{ entry.level }} · 修为 {{ entry.exp }} · 灵石 {{ entry.spiritStone }}</p>
            </article>
            <p v-if="store.rankings.length === 0" class="empty-text">当前暂无排行数据。</p>
          </div>

          <div v-else class="detail-stack">
            <article class="detail-card">
              <p class="detail-title">当前方位</p>
              <p>{{ scene.regionName }} / {{ scene.sceneName }}</p>
            </article>
            <article class="detail-card">
              <p class="detail-title">人界连通图</p>
              <div class="map-board">
                <svg class="map-svg" viewBox="0 0 100 120" preserveAspectRatio="none" aria-hidden="true">
                  <line
                    v-for="edge in mapEdges"
                    :key="`${edge.from.id}-${edge.to.id}`"
                    :x1="edge.from.x"
                    :y1="edge.from.y"
                    :x2="edge.to.x"
                    :y2="edge.to.y"
                    class="map-edge"
                    :class="{ active: edge.active }"
                  />
                </svg>
                <button
                  v-for="node in mapNodes"
                  :key="node.id"
                  type="button"
                  class="map-node"
                  :class="{ active: node.active, linked: node.linked }"
                  :style="{ left: `${node.x}%`, top: `${node.y}%` }"
                >
                  <span>{{ node.name }}</span>
                  <small>{{ node.region }}</small>
                </button>
              </div>
            </article>
            <article v-for="exit in exits" :key="exit.direction" class="detail-card">
              <p class="detail-title">向{{ directionLabel(String(exit.direction)) }}</p>
              <p>{{ exit.targetSceneName }}</p>
            </article>
          </div>
        </div>
      </section>
    </div>
  </div>
</template>
