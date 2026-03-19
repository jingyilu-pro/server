<script setup lang="ts">
import { computed, nextTick, onMounted, reactive, ref, watch } from 'vue'

import { useGameStore } from '@/stores/game'
import { directionLabelMap, worldMapEdges, worldMapNodes } from '@/lib/world-map'

type SideTab = 'player' | 'quests' | 'inventory' | 'map' | 'team' | 'rank' | 'codex'
type ComposerMode = 'chat' | 'command'
type CommandCategoryId =
  | 'social'
  | 'explore'
  | 'tasks'
  | 'combat'
  | 'spell'
  | 'cultivation'
  | 'gather'
  | 'alchemy'
  | 'trade'
  | 'group'
  | 'manual'
type OverlayPanel = 'none' | 'commands' | 'scene' | 'messages' | SideTab
type SceneInteractableKind = 'player' | 'npc' | 'shop' | 'monster' | 'resource' | 'loot' | 'hazard'
type ScrollPanelKey = 'topChat' | 'mainStory' | 'chatOverlay'

interface CommandAction {
  key: string
  label: string
  detail: string
  command?: string
  execute?: boolean
  composer?: ComposerMode
  chatChannel?: 'world' | 'team'
  prefillText?: string
  codexEntryId?: string
  codexCategory?: string
}

interface DenseLine {
  key: string
  tag: string
  text: string
  tone: 'system' | 'chat' | 'quest' | 'combat' | 'hint'
}

interface TimelineLine extends DenseLine {
  sequence: number
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
const selectedOriginId = ref('')
const composerText = ref('')
const activeTab = ref<SideTab>('player')
const activeCommandCategory = ref<CommandCategoryId>('social')
const composerMode = ref<ComposerMode>('chat')
const chatChannel = ref<'world' | 'team'>('world')
const activeOverlay = ref<OverlayPanel>('none')
const selectedSceneInteractableKey = ref('')
const selectedCodexCategory = ref('人物志')
const selectedCodexEntryId = ref('')
const eventViewport = ref<HTMLElement | null>(null)
const storyViewport = ref<HTMLElement | null>(null)
const chatOverlayViewport = ref<HTMLElement | null>(null)
const overlayChatText = ref('')
const mainTimeline = ref<TimelineLine[]>([])
const scrollPanels = reactive<Record<ScrollPanelKey, { autoFollow: boolean }>>({
  topChat: { autoFollow: true },
  mainStory: { autoFollow: true },
  chatOverlay: { autoFollow: true },
})
let mainTimelineSequence = 0
let trackedTimelineAccount = ''
const processedMainEventIds = new Set<number>()

const codexCategories = ['人物志', '宗门志', '妖兽志', '奇虫志', '地理志', '灵草丹药志', '功法技能志', '法术志', '宝物阵法志', '韩立年历'] as const

const sceneQuestOffers: Record<string, Array<{ id: string; title: string; summary: string }>> = {
  qixuan_square: [{ id: 'backslope_wolf_skin', title: '后山狼皮', summary: '厉飞雨想先看看你敢不敢见血，这条支线会带你熟悉七玄门后山与基础战斗。' }],
  qixuan_dormitory: [{ id: 'qixuan_stream_note', title: '溪边药方', summary: '孙二正急着找回被风吹走的药方纸，顺着这条线能把你带到洗剑溪与医师处。' }],
  qixuan_stream: [{ id: 'qixuan_stream_note', title: '溪边药方', summary: '药方找到后，可在洗剑溪交回医师完成这段跑腿。' }],
  jiayuan_market: [{ id: 'qixuan_herb', title: '墨府采药', summary: '嘉元城总管正急需黄精草，你若接下此事，便能借此熟悉坊市与采集路线。' }],
  mofu_front_hall: [{ id: 'mofu_guest_token', title: '墨府来客令', summary: '墨府前厅散落着来客令牌，适合继续熟悉搜寻与拾取。' }],
  tainan_gate: [{ id: 'ruins_old_map', title: '残垣旧图', summary: '流动牙人正在替海港一带打听旧图下落，这会把你一路引向血禁残垣与天南港。' }],
  tainan_fair: [{ id: 'fair_rumor_packet', title: '小会密闻', summary: '旧书摊想买虚天传闻，适合把太南谷里的人物与摊位继续串起来。' }],
  xin_house: [{ id: 'tainan_snake', title: '太南异胆', summary: '辛如音需要妖蛇异胆试阵，这条线会带你熟悉太南谷附近的战斗与材料循环。' }],
  array_lane: [{ id: 'tainan_array_flag', title: '阵旗回收', summary: '阵旗巷学徒正收残片，这条线更偏向阵法与拾取。' }],
  huangfeng_outpost: [{ id: 'huangfeng_letter', title: '黄枫谷羽信', summary: '外营弟子正等人送一封羽信入谷，这是接触黄枫谷主线的入口。' }],
  huangfeng_medicine_terrace: [{ id: 'medicine_moss', title: '药台苔引', summary: '药梯台正在收灵苔药引，适合继续熟悉采药与炼制材料。' }],
  huangfeng_scripture: [{ id: 'huangfeng_manual', title: '藏经抄卷', summary: '藏经石廊的执事在等一卷误落抄卷，这条线会把你带往黄枫谷典籍区。' }],
  blood_gate: [
    { id: 'blood_forbidden_token', title: '血禁采兰', summary: '血禁执事正在收血兰验阵，这是血色禁地的正式入口任务。' },
    { id: 'blood_swamp_rescue', title: '沼泽驱邪', summary: '血雾沼泽的虫群样本也有人悬赏，适合继续深入禁地。' },
  ],
  tiannan_harbor: [
    { id: 'ruins_old_map', title: '残垣旧图', summary: '旧图真正的买家在海港，交到海商牙人手里才能换到远航门路。' },
    { id: 'chaos_sea_chart', title: '乱星海海图', summary: '海商牙人正在找修补海图禁制的材料，这会继续把你推向乱星海。' },
  ],
  smuggler_alley: [{ id: 'harbor_signal', title: '暗巷接头', summary: '暗潮小巷有人在收接头暗号，适合补足港口支线。' }],
  chaos_sea_port: [{ id: 'captain_supply', title: '远航补给', summary: '曲船主还缺远航物资，这条线会把你送上真正的海路。' }],
  chaos_sea_ship: [{ id: 'demon_fish_core', title: '妖鱼内丹', summary: '船上术士正在收妖鱼内丹，可顺手把海战与法术线串起来。' }],
  chaos_sea_isle: [{ id: 'chaos_relic', title: '孤岛残碑', summary: '残碑孤岛的隐士需要残钥碎片，这会引出虚天殿前置线。' }],
  xutian_hall: [{ id: 'xutian_key', title: '虚天残钥', summary: '守门残灵在等残钥，交齐后才能真正逼近虚天殿内层。' }],
  xutian_star_platform: [{ id: 'xutian_star_map', title: '星纹演算', summary: '祭台残灵正在收星纹拓片，这是逼近内殿玄门的最后一步。' }],
}

const sceneSectOffers: Record<string, Array<{ command: string; name: string; summary: string }>> = {
  qixuan_hall: [{ command: 'join qixuan_gate', name: '七玄门', summary: '七玄门适合凡人启程，在这里能打好最初的修行和江湖根基。' }],
  huangfeng_hall: [{ command: 'join huangfeng_valley', name: '黄枫谷', summary: '黄枫谷重视基础与心性，是越国七派里较稳的一条成长路线。' }],
  yanyue_peak: [{ command: 'join yanyue_sect', name: '掩月宗', summary: '掩月宗重视身法与法门，入门后更适合往灵动轻灵路线修行。' }],
}

const narrativeArcLabels: Record<number, string> = {
  1: '七玄门启程',
  2: '太南小会',
  3: '黄枫谷试炼',
  4: '血色禁地试炼',
  5: '乱星海漂流',
  6: '虚天殿初启',
}

const narrativeQuestOrder: Record<string, number> = {
  backslope_wolf_skin: 1,
  qixuan_stream_note: 1,
  qixuan_herb: 1,
  mofu_guest_token: 1,
  ruins_old_map: 2,
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
}

const sceneFollowupHints: Record<string, string> = {
  chaos_sea_ship: '当前章节：乱星海漂流。甲板术士更像是在替你梳理后续海路与虚天线索，可继续交谈后再向深海推进。',
  chaos_sea_isle: '残碑孤岛的碑文与钥片都和虚天殿有关，别忘了查看遗迹、人物与地面线索。',
  storm_route: '当前章节：乱星海漂流。先稳住法力与气力，再沿风暴航道继续向东探路。',
}

const categoryLabels: Record<CommandCategoryId, string> = {
  social: '交流',
  explore: '探索',
  tasks: '任务',
  combat: '战斗',
  spell: '法术',
  cultivation: '修炼',
  gather: '采集',
  alchemy: '炼制',
  trade: '交易',
  group: '宗门队伍',
  manual: '手册',
}

const sideTabLabels: Array<{ id: SideTab; label: string }> = [
  { id: 'player', label: '人物' },
  { id: 'quests', label: '任务' },
  { id: 'inventory', label: '背包' },
  { id: 'codex', label: '手册' },
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
const availableOrigins = computed(() => (store.availableOrigins as Record<string, any>[] | undefined) ?? [])
const currentSceneId = computed(() => String(scene.value.sceneId ?? ''))
const chatEvents = computed(() => store.events.filter((event) => isChatEvent(event)))
const latestChatEventId = computed(() => chatEvents.value[chatEvents.value.length - 1]?.eventId)
const exits = computed(() => (scene.value.exits as Record<string, any>[] | undefined) ?? [])
const inventory = computed(() => (player.value.inventory as Record<string, any>[] | undefined) ?? [])
const quests = computed(() => (player.value.quests as Record<string, any>[] | undefined) ?? [])
const npcs = computed(() => (scene.value.npcs as Record<string, any>[] | undefined) ?? [])
const monsters = computed(() => (scene.value.monsters as string[] | undefined) ?? [])
const shops = computed(() => (scene.value.shops as string[] | undefined) ?? [])
const scenePlayers = computed(() => (scene.value.players as Record<string, any>[] | undefined) ?? [])
const sceneResourceNodes = computed(() => (scene.value.resourceNodes as Record<string, any>[] | undefined) ?? [])
const sceneGroundLoots = computed(() => (scene.value.groundLoots as Record<string, any>[] | undefined) ?? [])
const sceneHazards = computed(() => (scene.value.hazards as Record<string, any>[] | undefined) ?? [])
const codexSummaries = computed(() => (player.value.codexSummaries as Record<string, any>[] | undefined) ?? [])
const codexEntries = computed(() => (store.codexEntries as Record<string, any>[] | undefined) ?? [])
const codexDetail = computed(() => store.codexDetail as Record<string, any> | null)
const displayError = computed(() => store.error || store.pollError)
const currentQuestIds = computed(() => new Set(quests.value.map((quest) => String(quest.questId ?? ''))))
const latestNonChatEvent = computed(
  () => [...store.events].reverse().find((event) => !isChatEvent(event)) ?? null,
)
const showGameView = computed(() => store.authenticated && store.readyToPlay)
const showCreateCharacterView = computed(() => store.authenticated && store.needCreateCharacter && !store.readyToPlay)
const currentStatusAttributes = computed(
  () => (player.value.currentStatusAttributes as Record<string, any> | undefined) ?? player.value.statusAttributes ?? {},
)
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
const selectedOrigin = computed(
  () =>
    availableOrigins.value.find((origin) => String(origin.originId ?? '') === selectedOriginId.value) ??
    availableOrigins.value[0] ??
    null,
)
const selectedCodexSummary = computed(
  () =>
    codexSummaries.value.find((summary) => String(summary.category ?? summary.entryId ?? '') === selectedCodexCategory.value) ??
    null,
)

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
  const availableSceneQuestOffers = (sceneQuestOffers[currentSceneId.value] ?? []).filter(
    (quest) => !currentQuestIds.value.has(quest.id),
  )
  const readySceneQuests = quests.value.filter(
    (quest) =>
      isQuestReadyToSubmit(quest) &&
      (sceneQuestOffers[currentSceneId.value] ?? []).some((offer) => offer.id === String(quest.questId ?? '')),
  )
  const firstMonster = monsters.value[0]

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
    ...availableSceneQuestOffers.map((quest) => ({
        key: `accept-${quest.id}`,
        label: `接取·${quest.title}`,
        detail: '接下当前场景的线索任务。',
        command: `accept ${quest.id}`,
      })),
    ...readySceneQuests.map((quest) => ({
        key: `submit-${String(quest.questId)}`,
        label: `提交·${String(quest.title)}`,
        detail: '材料已齐，可以当场提交。',
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

  const spell: CommandAction[] = [
    ...(((player.value.spells as Record<string, any>[] | undefined) ?? []).filter((item) => Boolean(item.unlocked))).map((item) => ({
      key: `cast-${String(item.spellId)}`,
      label: `施放·${String(item.name)}`,
      detail: firstMonster ? `默认对 ${firstMonster} 施法，也可切到原始指令改目标。` : '先选定目标，再通过原始指令施放。',
      command: firstMonster ? `cast ${String(item.spellId)} ${firstMonster}` : `cast ${String(item.spellId)} `,
      execute: Boolean(firstMonster),
      composer: firstMonster ? undefined : ('command' as const),
    })),
    {
      key: 'meditate',
      label: '吐纳调息',
      detail: '恢复法力、神念与气力，准备下一轮施法。',
      command: 'meditate',
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

  const gather: CommandAction[] = [
    ...sceneResourceNodes.value.map((node) => ({
      key: `harvest-${String(node.nodeId)}`,
      label: `采集·${String(node.name)}`,
      detail: `采集 ${String(node.dropItemName ?? node.dropItemId ?? '材料')}。`,
      command: `harvest ${String(node.nodeId)}`,
    })),
    ...sceneGroundLoots.value.map((loot) => ({
      key: `loot-${String(loot.lootId)}`,
      label: `拾取·${String(loot.itemName)}`,
      detail: `拾起地面上的 ${String(loot.itemName ?? loot.itemId)}。`,
      command: `loot ${String(loot.lootId ?? loot.itemId)}`,
    })),
  ]

  const alchemy: CommandAction[] = [
    ...(((player.value.recipes as Record<string, any>[] | undefined) ?? []).filter((item) => Boolean(item.unlocked))).map((item) => ({
      key: `brew-${String(item.recipeId)}`,
      label: `炼制·${String(item.name)}`,
      detail: String(item.description ?? '按配方炼制丹药与辅助物。'),
      command: `brew ${String(item.recipeId)}`,
    })),
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

  const manual: CommandAction[] = codexCategories.map((category) => {
    const summary =
      codexSummaries.value.find((item) => String(item.category ?? item.entryId ?? '') === category) ?? null
    return {
      key: `manual-${category}`,
      label: category,
      detail: String(summary?.summary ?? '打开分类手册，查看当前已解锁条目。'),
      codexCategory: category,
    }
  })

  return [
    { id: 'social' as const, label: categoryLabels.social, actions: social },
    { id: 'explore' as const, label: categoryLabels.explore, actions: explore },
    { id: 'tasks' as const, label: categoryLabels.tasks, actions: taskActions },
    { id: 'combat' as const, label: categoryLabels.combat, actions: combat },
    { id: 'spell' as const, label: categoryLabels.spell, actions: spell },
    { id: 'cultivation' as const, label: categoryLabels.cultivation, actions: cultivation },
    { id: 'gather' as const, label: categoryLabels.gather, actions: gather },
    { id: 'alchemy' as const, label: categoryLabels.alchemy, actions: alchemy },
    { id: 'trade' as const, label: categoryLabels.trade, actions: trade },
    { id: 'group' as const, label: categoryLabels.group, actions: group },
    { id: 'manual' as const, label: categoryLabels.manual, actions: manual },
  ]
})

const activeCommandActions = computed(
  () => commandCategories.value.find((category) => category.id === activeCommandCategory.value)?.actions ?? [],
)

const activeCommandCategoryLabel = computed(
  () => commandCategories.value.find((category) => category.id === activeCommandCategory.value)?.label ?? '交流',
)

const activeInfoTab = computed<SideTab>(() =>
  activeOverlay.value !== 'none' &&
  activeOverlay.value !== 'commands' &&
  activeOverlay.value !== 'scene' &&
  activeOverlay.value !== 'messages'
    ? activeOverlay.value
    : activeTab.value,
)

const sceneInteractables = computed<SceneInteractable[]>(() => {
  const entries: SceneInteractable[] = []
  const activeQuestList = quests.value.filter((quest) => String(quest.status ?? '') === 'active')
  const sceneQuestEntries = sceneQuestOffers[currentSceneId.value] ?? []
  const availableQuestOffers = sceneQuestEntries.filter((quest) => !currentQuestIds.value.has(quest.id))
  const sectOffers = (sceneSectOffers[currentSceneId.value] ?? []).filter(() => !player.value.sect?.joined)
  const primarySkill = String(player.value.cultivation?.primarySkill ?? '长春功')
  const readySceneQuests = activeQuestList.filter((quest) =>
    sceneQuestEntries.some((offer) => offer.id === String(quest.questId ?? '')) && isQuestReadyToSubmit(quest),
  )
  const trackingSceneQuest = activeQuestList.find((quest) =>
    sceneQuestEntries.some((offer) => offer.id === String(quest.questId ?? '')),
  )
  const completedSceneQuest = sceneQuestEntries.some((offer) => currentQuestIds.value.has(offer.id))
  const sceneFollowupHint = sceneFollowupHints[currentSceneId.value]
  const teamAccountSet = new Set(teamMembers.value.map((member) => String(member.account ?? '')))

  scenePlayers.value.forEach((scenePlayer) => {
    const playerAccount = String(scenePlayer.account ?? '')
    const displayName = String(scenePlayer.characterName ?? scenePlayer.account ?? '无名修士')
    const sameTeam = teamAccountSet.has(playerAccount)
    const playerActions: CommandAction[] = [
      {
        key: `interactable-inspect-player-${playerAccount}`,
        label: '观察对方',
        detail: '先从外观和称号上判断对方来历。',
        composer: 'chat',
        chatChannel: 'world',
        prefillText: `看向 ${displayName} `,
      },
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
        key: `interactable-inspect-npc-${String(npc.npcId ?? npcName)}`,
        label: `端详·${npcName}`,
        detail: '先仔细观察此人的来历与气度。',
        command: `inspect ${npcName}`,
      },
      {
        key: `interactable-talk-${String(npc.npcId ?? npcName)}`,
        label: `交谈·${npcName}`,
        detail: '先与此人交谈，打探消息和后续线索。',
        command: `talk ${npcName}`,
      },
    ]
    const npcQuestOffers = Boolean(npc.hasQuest) ? availableQuestOffers : []
    const npcReadyQuests = Boolean(npc.hasQuest) ? readySceneQuests : []

    npcQuestOffers.forEach((questOffer) => {
      npcActions.push({
        key: `interactable-npc-accept-${String(npc.npcId ?? npcName)}-${questOffer.id}`,
        label: `接取·${questOffer.title}`,
        detail: '顺着这位人物给出的线索继续推进。',
        command: `accept ${questOffer.id}`,
      })
    })

    npcReadyQuests.forEach((questState) => {
      npcActions.push({
        key: `interactable-npc-submit-${String(npc.npcId ?? npcName)}-${String(questState.questId ?? '')}`,
        label: `提交·${String(questState.title ?? '任务')}`,
        detail: '材料已经齐备，可直接向此人交付。',
        command: `submit ${String(questState.questId ?? '')}`,
      })
    })

    if (index === 0 && sectOffers.length > 0) {
      const sect = sectOffers[0]
      npcActions.push({
        key: `interactable-join-${sect.command}`,
        label: `拜入·${sect.name}`,
        detail: '若条件足够，可当场拜入宗门。',
        command: sect.command,
      })
    }

    if (String(npc.codexEntryId ?? '')) {
      npcActions.push({
        key: `interactable-codex-npc-${String(npc.npcId ?? npcName)}`,
        label: '查看资料',
        detail: '打开手册查看此人物的资料条目。',
        codexEntryId: String(npc.codexEntryId),
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
        npcQuestOffers.length > 0
          ? `可接任务：${npcQuestOffers.map((quest) => quest.title).join('、')}`
          : npcReadyQuests.length > 0
            ? `可提交任务：${npcReadyQuests.map((quest) => String(quest.title ?? '当前任务')).join('、')}`
            : Boolean(npc.hasQuest) && trackingSceneQuest
              ? `相关任务进行中：${String(trackingSceneQuest.title ?? '当前任务')} ${String(trackingSceneQuest.progress ?? 0)} / ${String(trackingSceneQuest.target ?? 0)}`
              : Boolean(npc.hasQuest) && completedSceneQuest && sceneFollowupHint
                ? '可继续交谈梳理后续线索'
                : Boolean(npc.hasQuest) && sceneQuestEntries.length > 0
                  ? '似有新委托可谈'
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

    itemActions.push({
      key: `interactable-inspect-item-${itemId}`,
      label: `查看·${itemName}`,
      detail: '先看看这件物件的来历与用途。',
      command: `inspect ${itemId || itemName}`,
    })

    if (String(sceneItem.codexEntryId ?? '')) {
      itemActions.push({
        key: `interactable-codex-item-${itemId}`,
        label: '查看资料',
        detail: '打开手册查看这件物件相关的设定。',
        codexEntryId: String(sceneItem.codexEntryId),
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
        `类别：${itemTypeLabel(itemType)}`,
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
          key: `interactable-inspect-${monster}`,
          label: `查看·${monster}`,
          detail: '先探明敌手，再决定出手方式。',
          command: `inspect ${monster}`,
        },
        {
          key: `interactable-fight-${monster}`,
          label: `挑战·${monster}`,
          detail: '立即进入战斗，检验当前战力。',
          command: `fight ${monster}`,
        },
        ...(((player.value.spells as Record<string, any>[] | undefined) ?? []).filter((item) => Boolean(item.unlocked)).slice(0, 1).map((spell) => ({
          key: `interactable-cast-${monster}-${String(spell.spellId)}`,
          label: `法术·${String(spell.name)}`,
          detail: '以当前掌握的法术先手试探。',
          command: `cast ${String(spell.spellId)} ${monster}`,
        })) as CommandAction[]),
        {
          key: `interactable-practice-${monster}`,
          label: '先行调息',
          detail: '先运转功法稳住状态，再考虑出手。',
          command: `practice ${primarySkill}`,
        },
      ],
    })
  })

  sceneResourceNodes.value.forEach((node) => {
    const actions: CommandAction[] = [
      {
        key: `interactable-inspect-resource-${String(node.nodeId)}`,
        label: `查看·${String(node.name)}`,
        detail: '观察采集点的环境和可得材料。',
        command: `inspect ${String(node.nodeId ?? node.name)}`,
      },
      {
        key: `interactable-harvest-${String(node.nodeId)}`,
        label: `采集·${String(node.name)}`,
        detail: `尝试获得 ${String(node.dropItemName ?? node.dropItemId ?? '材料')}。`,
        command: `harvest ${String(node.nodeId ?? node.name)}`,
      },
    ]
    if (String(node.codexEntryId ?? '')) {
      actions.push({
        key: `interactable-codex-resource-${String(node.nodeId)}`,
        label: '查看资料',
        detail: '打开手册查看这处资源点相关资料。',
        codexEntryId: String(node.codexEntryId),
      })
    }

    entries.push({
      key: `interactable-resource-${String(node.nodeId)}`,
      kind: 'resource',
      railLabel: '采点',
      railCaption: shortenText(String(node.name ?? '资源点'), 5),
      title: String(node.name ?? '资源点'),
      subtitle: '视野可见采集点',
      description: String(node.description ?? '这里有可采集的灵材与原料。'),
      meta: [
        `产出：${String(node.dropItemName ?? node.dropItemId ?? '未知材料')}`,
        Number(node.dropItemCount ?? 0) > 0 ? `数量：${String(node.dropItemCount)}` : '',
      ].filter(Boolean) as string[],
      actions,
    })
  })

  sceneGroundLoots.value.forEach((loot) => {
    const actions: CommandAction[] = [
      {
        key: `interactable-inspect-loot-${String(loot.lootId)}`,
        label: `查看·${String(loot.itemName)}`,
        detail: '看看地面遗落物的来历和价值。',
        command: `inspect ${String(loot.lootId ?? loot.itemId ?? loot.itemName)}`,
      },
      {
        key: `interactable-loot-${String(loot.lootId)}`,
        label: `拾取·${String(loot.itemName)}`,
        detail: '把这件遗落物收入囊中。',
        command: `loot ${String(loot.lootId ?? loot.itemId ?? loot.itemName)}`,
      },
    ]
    if (String(loot.codexEntryId ?? '')) {
      actions.push({
        key: `interactable-codex-loot-${String(loot.lootId)}`,
        label: '查看资料',
        detail: '打开手册查看相关物件条目。',
        codexEntryId: String(loot.codexEntryId),
      })
    }

    entries.push({
      key: `interactable-loot-${String(loot.lootId)}`,
      kind: 'loot',
      railLabel: '遗落',
      railCaption: shortenText(String(loot.itemName ?? '遗落物'), 5),
      title: String(loot.itemName ?? loot.itemId ?? '遗落物'),
      subtitle: '地面可拾取物件',
      description: String(loot.description ?? '一件散落在地上的物件。'),
      meta: [`数量：${String(loot.quantity ?? 1)}`],
      actions,
    })
  })

  sceneHazards.value.forEach((hazard) => {
    const actions: CommandAction[] = []
    if (String(hazard.codexEntryId ?? '')) {
      actions.push({
        key: `interactable-codex-hazard-${String(hazard.hazardId)}`,
        label: '查看资料',
        detail: '打开手册查看这处禁制或险地说明。',
        codexEntryId: String(hazard.codexEntryId),
      })
    }
    if (!actions.length) {
      actions.push({
        key: `interactable-chat-hazard-${String(hazard.hazardId)}`,
        label: '记录异象',
        detail: '切到聊天输入，记下此地异常。',
        composer: 'chat',
        chatChannel: 'world',
        prefillText: `此地遇到${String(hazard.name ?? '禁制')}，`,
      })
    }

    entries.push({
      key: `interactable-hazard-${String(hazard.hazardId)}`,
      kind: 'hazard',
      railLabel: '禁制',
      railCaption: shortenText(String(hazard.name ?? '禁制'), 5),
      title: String(hazard.name ?? '禁制'),
      subtitle: '场景中的机关与风险',
      description: String(hazard.description ?? '这处区域存在明显的灵压与禁制反应。'),
      meta: [
        Number(hazard.hpCost ?? 0) > 0 ? `气血消耗：${String(hazard.hpCost)}` : '',
        Number(hazard.manaCost ?? 0) > 0 ? `法力消耗：${String(hazard.manaCost)}` : '',
        Number(hazard.staCost ?? 0) > 0 ? `气力消耗：${String(hazard.staCost)}` : '',
      ].filter(Boolean) as string[],
      actions,
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
  if (activeOverlay.value === 'messages') {
    return '聊天消息'
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
  if (activeOverlay.value === 'messages') {
    return chatChannel.value === 'team' ? '队伍频道' : '世界频道'
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

function narrativeArcOrderForChapter(value: string) {
  const normalized = value.trim()
  if (!normalized) {
    return 0
  }

  const entry = Object.entries(narrativeArcLabels).find(([, label]) => label === normalized)
  return entry ? Number(entry[0]) : 0
}

function narrativeArcOrderForScene(sceneId: string) {
  const normalized = sceneId.trim()
  if (!normalized) {
    return 0
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

function narrativeArcOrderForQuest(questId: string) {
  return narrativeQuestOrder[questId] ?? 0
}

const displayProgressionChapter = computed(() => {
  const playerChapter = String(player.value.progressionChapter ?? '')
  const playerArc = narrativeArcOrderForChapter(playerChapter)
  const sceneArc = narrativeArcOrderForScene(currentSceneId.value)
  if (sceneArc > playerArc) {
    return narrativeArcLabels[sceneArc] || playerChapter || '七玄门启程'
  }
  return playerChapter || narrativeArcLabels[sceneArc] || '七玄门启程'
})

const currentNarrativeArc = computed(() =>
  Math.max(
    narrativeArcOrderForScene(currentSceneId.value),
    narrativeArcOrderForChapter(String(player.value.progressionChapter ?? '')),
  ),
)

const trackedQuest = computed(() => {
  const activeQuestList = quests.value.filter((quest) => String(quest.status ?? '') === 'active')
  if (activeQuestList.length === 0) {
    return null
  }

  const sceneQuestIds = new Set((sceneQuestOffers[currentSceneId.value] ?? []).map((quest) => quest.id))
  for (let index = activeQuestList.length - 1; index >= 0; index -= 1) {
    const quest = activeQuestList[index]
    if (sceneQuestIds.has(String(quest.questId ?? ''))) {
      return quest
    }
  }

  const narrativeFloor = Math.max(1, currentNarrativeArc.value - 1)
  const chapterQuest = activeQuestList
    .map((quest, index) => ({
      quest,
      index,
      order: narrativeArcOrderForQuest(String(quest.questId ?? '')),
    }))
    .filter((entry) => entry.order >= narrativeFloor)
    .sort((left, right) => right.order - left.order || right.index - left.index)[0]

  if (chapterQuest) {
    return chapterQuest.quest
  }

  if (currentNarrativeArc.value <= 1) {
    return activeQuestList[activeQuestList.length - 1] ?? null
  }

  return null
})

const sceneMissionText = computed(() => {
  const activeQuest = trackedQuest.value
  if (activeQuest) {
    return `当前主线：${String(activeQuest.title)}，进度 ${String(activeQuest.progress ?? 0)} / ${String(activeQuest.target ?? 0)}。`
  }

  const questOffer = (sceneQuestOffers[currentSceneId.value] ?? []).find(
    (quest) => !quests.value.some((item) => String(item.questId ?? '') === quest.id),
  )
  if (questOffer) {
    return `此地有新线索可接：${questOffer.title}。`
  }

  const followupHint = sceneFollowupHints[currentSceneId.value]
  if (followupHint) {
    return followupHint
  }

  if (currentNarrativeArc.value > 1) {
    return `当前章节：${displayProgressionChapter.value}。先在此地观察局势，再与场景人物交谈梳理后续线索。`
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

const quickStats = computed(() => [
  {
    key: 'hp',
    label: '气血',
    value: `${String(player.value.hp ?? 0)}/${String(player.value.maxHp ?? 0)}`,
  },
  {
    key: 'mana',
    label: '法力',
    value: `${String(currentStatusAttributes.value.mana ?? player.value.statusAttributes?.mana ?? 0)}/${String(player.value.statusAttributes?.mana ?? 0)}`,
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

function itemTypeLabel(value: string) {
  const normalized = value.trim()
  if (!normalized) {
    return '杂物'
  }
  if (/[\u4e00-\u9fff]/u.test(normalized)) {
    return normalized
  }

  const labels: Record<string, string> = {
    consumable: '丹药',
    weapon: '兵刃',
    armor: '护具',
    accessory: '饰物',
    material: '材料',
    treasure: '宝物',
    book: '典籍',
    recipe: '配方',
    quest: '任务物',
    tool: '器具',
  }

  return labels[normalized.toLowerCase()] ?? '杂物'
}

function normalizeEventType(value: string) {
  return value.trim().toLowerCase()
}

function isChatEvent(event: Record<string, any>) {
  const type = normalizeEventType(String(event.type ?? ''))
  if (type === 'chat') {
    return true
  }
  return /^\[(world|public|team)\]\s*/iu.test(String(event.title ?? ''))
}

function chatChannelLabelFromEvent(event: Record<string, any>) {
  const title = String(event.title ?? '')
  if (/^\[team\]\s*/iu.test(title)) {
    return '队伍'
  }
  if (/^\[(world|public)\]\s*/iu.test(title)) {
    return '世界'
  }
  return '聊天'
}

function formatChatSpeaker(title: string) {
  const normalized = title.trim()
  if (!normalized) {
    return '无名修士'
  }
  return normalized
    .replace(/^\[(world|public)\]\s*/iu, '世界 · ')
    .replace(/^\[team\]\s*/iu, '队伍 · ')
}

function formatEventText(event: Record<string, any>) {
  const title = String(event.title ?? '').trim()
  const content = String(event.content ?? '').trim()
  if (isChatEvent(event)) {
    return [formatChatSpeaker(title), content].filter(Boolean).join('：')
  }
  return [title, content].filter(Boolean).join('：')
}

function eventTone(event: Record<string, any>) {
  const type = normalizeEventType(String(event.type ?? ''))
  if (type === 'chat') {
    return 'chat'
  }
  if (['fight', 'spell'].includes(type)) {
    return 'combat'
  }
  if (['quest', 'join'].includes(type)) {
    return 'quest'
  }
  if (['harvest', 'loot'].includes(type)) {
    return 'hint'
  }

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

function eventChannelLabel(event: Record<string, any>) {
  const type = normalizeEventType(String(event.type ?? ''))
  if (isChatEvent(event)) {
    return chatChannelLabelFromEvent(event)
  }
  if (type === 'fight' || type === 'spell') {
    return '战报'
  }
  if (type === 'quest' || type === 'join') {
    return '任务'
  }
  if (type === 'loot') {
    return '拾取'
  }
  if (type === 'harvest') {
    return '采集'
  }
  if (type === 'cultivation') {
    return '修炼'
  }
  if (type === 'brew') {
    return '炼制'
  }
  if (type === 'team') {
    return '队伍'
  }

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

function isQuestReadyToSubmit(quest: Record<string, any>) {
  return (
    String(quest.status ?? '') === 'active' &&
    Number(quest.target ?? 0) > 0 &&
    Number(quest.progress ?? 0) >= Number(quest.target ?? 0)
  )
}

function parseQuestHint(value: string) {
  const acceptMatch = value.match(/^可接任务：accept\s+([^\s（(]+)[（(](.+)[）)]$/u)
  if (acceptMatch) {
    return {
      type: 'accept' as const,
      questId: acceptMatch[1],
      title: acceptMatch[2],
    }
  }

  const submitMatch = value.match(/^可提交任务：submit\s+([^\s（(]+)[（(](.+)[）)]$/u)
  if (submitMatch) {
    return {
      type: 'submit' as const,
      questId: submitMatch[1],
      title: submitMatch[2],
    }
  }

  return null
}

function shouldDisplayResultHint(value: string) {
  const questHint = parseQuestHint(value)
  if (!questHint) {
    return true
  }

  const questState = quests.value.find((quest) => String(quest.questId ?? '') === questHint.questId)
  if (questHint.type === 'accept') {
    return !questState
  }

  if (!questState) {
    return false
  }

  return isQuestReadyToSubmit(questState)
}

function displayResultHint(value: string) {
  const questHint = parseQuestHint(value)
  if (questHint) {
    return `${questHint.type === 'accept' ? '可接任务' : '可提交任务'}：${questHint.title}`
  }

  const joinMatch = value.match(/^若想拜入(.+)，可使用：join\s+.+$/u)
  if (joinMatch) {
    return `若想拜入${joinMatch[1]}，可在下方功能盘中选择“加入·${joinMatch[1]}”。`
  }

  const replacements: Array<[RegExp, string]> = [
    [/^先用 look 查看场景人物$/u, '可先点击“重看”查看当前场景人物。'],
    [/^先用 look 查看场景妖兽$/u, '可先点击“重看”查看当前场景妖兽。'],
    [/^先用 map 查看当前出口$/u, '可先打开地图或点击方位盘查看去路。'],
    [/^继续 practice .+ 或 fight <target>$/u, '继续修炼本命功法，或挑战附近妖兽积累突破火候。'],
    [/^可使用 use .+ 恢复气血$/u, '可在背包或战斗功能里使用回气药物恢复气血。'],
  ]

  for (const [pattern, replacement] of replacements) {
    if (pattern.test(value)) {
      return replacement
    }
  }

  return value
}

function formatResultSummary(result: Record<string, any>) {
  return [String(result.title ?? '').trim(), String(result.summary ?? '').trim()].filter(Boolean).join('：')
}

function latestNonChatEventMatches(text: string) {
  const normalized = text.trim()
  if (!normalized) {
    return false
  }

  const event = latestNonChatEvent.value
  if (!event) {
    return false
  }

  return [formatEventText(event), String(event.content ?? '').trim(), String(event.title ?? '').trim()].includes(normalized)
}

function appendMainTimeline(lines: DenseLine[]) {
  const normalizedLines = lines.filter((line) => line.text.trim())
  if (!normalizedLines.length) {
    return
  }

  normalizedLines.forEach((line) => {
    mainTimelineSequence += 1
    mainTimeline.value.push({
      ...line,
      sequence: mainTimelineSequence,
    })
  })

  if (mainTimeline.value.length > 400) {
    mainTimeline.value.splice(0, mainTimeline.value.length - 400)
  }
}

function resetMessageTimelines() {
  mainTimeline.value = []
  mainTimelineSequence = 0
  processedMainEventIds.clear()
  scrollPanels.topChat.autoFollow = true
  scrollPanels.mainStory.autoFollow = true
  scrollPanels.chatOverlay.autoFollow = true
}

function buildSceneSnapshotLines() {
  const lines: DenseLine[] = []
  const regionName = String(scene.value.regionName ?? '').trim()
  const sceneName = String(scene.value.sceneName ?? '').trim()
  const sceneDescription = String(scene.value.description ?? '').trim()

  if (regionName || sceneName) {
    lines.push({
      key: `scene-place-${currentSceneId.value}-${mainTimelineSequence + 1}`,
      tag: '场景',
      text: [regionName, sceneName].filter(Boolean).join(' · '),
      tone: 'system',
    })
  }

  splitDenseText(sceneDescription, 6).forEach((text, index) => {
    lines.push({
      key: `scene-description-${currentSceneId.value}-${mainTimelineSequence + index + 1}`,
      tag: '场景',
      text,
      tone: 'system',
    })
  })

  if (sceneMissionText.value) {
    lines.push({
      key: `scene-mission-${currentSceneId.value}-${mainTimelineSequence + 1}`,
      tag: '任务',
      text: sceneMissionText.value,
      tone: 'quest',
    })
  }

  if (npcs.value.length > 0) {
    lines.push({
      key: `scene-npcs-${currentSceneId.value}-${mainTimelineSequence + 1}`,
      tag: '人物',
      text: `你看见 ${npcs.value.map((npc) => String(npc.name ?? '无名人物')).join('、')}。`,
      tone: 'hint',
    })
  }

  if (scenePlayers.value.length > 0) {
    lines.push({
      key: `scene-players-${currentSceneId.value}-${mainTimelineSequence + 1}`,
      tag: '玩家',
      text: `同场景还有 ${scenePlayers.value.map((entry) => String(entry.characterName ?? entry.account ?? '无名修士')).join('、')}。`,
      tone: 'hint',
    })
  }

  if (monsters.value.length > 0) {
    lines.push({
      key: `scene-monsters-${currentSceneId.value}-${mainTimelineSequence + 1}`,
      tag: '战报',
      text: `附近徘徊着 ${monsters.value.join('、')}。`,
      tone: 'combat',
    })
  }

  if (sceneItems.value.length > 0) {
    lines.push({
      key: `scene-items-${currentSceneId.value}-${mainTimelineSequence + 1}`,
      tag: '物件',
      text: `视野里可见 ${sceneItems.value.map((entry) => String(entry.name ?? '无名物件')).join('、')}。`,
      tone: 'system',
    })
  }

  if (sceneResourceNodes.value.length > 0) {
    lines.push({
      key: `scene-resource-${currentSceneId.value}-${mainTimelineSequence + 1}`,
      tag: '采集',
      text: `附近可采集 ${sceneResourceNodes.value.map((entry) => String(entry.name ?? '资源点')).join('、')}。`,
      tone: 'hint',
    })
  }

  if (sceneGroundLoots.value.length > 0) {
    lines.push({
      key: `scene-ground-loot-${currentSceneId.value}-${mainTimelineSequence + 1}`,
      tag: '遗落',
      text: `地面遗落着 ${sceneGroundLoots.value.map((entry) => String(entry.itemName ?? '遗落物')).join('、')}。`,
      tone: 'system',
    })
  }

  if (sceneHazards.value.length > 0) {
    lines.push({
      key: `scene-hazard-${currentSceneId.value}-${mainTimelineSequence + 1}`,
      tag: '禁制',
      text: `此地存在 ${sceneHazards.value.map((entry) => String(entry.name ?? '未知禁制')).join('、')}。`,
      tone: 'quest',
    })
  }

  return lines
}

function buildResultTimelineLines(result: Record<string, any>) {
  const lines: DenseLine[] = []
  const resultSummary = formatResultSummary(result)
  if (resultSummary && !latestNonChatEventMatches(resultSummary)) {
    lines.push({
      key: `result-summary-${mainTimelineSequence + 1}`,
      tag: '结果',
      text: resultSummary,
      tone: result.success === false ? 'quest' : 'hint',
    })
  }

  ;((result.hints as string[] | undefined) ?? [])
    .filter((hint) => shouldDisplayResultHint(hint))
    .slice(0, 4)
    .forEach((hint, index) => {
      const text = displayResultHint(hint)
      if (!text || latestNonChatEventMatches(text)) {
        return
      }
      lines.push({
        key: `result-hint-${mainTimelineSequence + index + 1}`,
        tag: '提示',
        text,
        tone: 'hint',
      })
    })

  const spellSummary = String(result.spellSummary ?? '').trim()
  if (spellSummary && !latestNonChatEventMatches(spellSummary)) {
    lines.push({
      key: `result-spell-${mainTimelineSequence + 1}`,
      tag: '法术',
      text: spellSummary,
      tone: 'combat',
    })
  }

  const brewSummary = String(result.brewSummary ?? '').trim()
  if (brewSummary && !latestNonChatEventMatches(brewSummary)) {
    lines.push({
      key: `result-brew-${mainTimelineSequence + 1}`,
      tag: '炼制',
      text: brewSummary,
      tone: 'system',
    })
  }

  const hazardFeedback = String(result.hazardFeedback ?? '').trim()
  if (hazardFeedback) {
    lines.push({
      key: `result-hazard-${mainTimelineSequence + 1}`,
      tag: '禁制',
      text: hazardFeedback,
      tone: 'hint',
    })
  }

  ;((result.unlockedCodexEntries as Record<string, any>[] | undefined) ?? []).slice(0, 4).forEach((entry, index) => {
    lines.push({
      key: `result-codex-${mainTimelineSequence + index + 1}`,
      tag: '手册',
      text: `解锁资料：${String(entry.title ?? entry.entryId ?? '未知条目')}。`,
      tone: 'hint',
    })
  })

  return lines
}

function viewportFor(panel: ScrollPanelKey) {
  if (panel === 'topChat') {
    return eventViewport.value
  }
  if (panel === 'mainStory') {
    return storyViewport.value
  }
  return chatOverlayViewport.value
}

function distanceFromViewportBottom(viewport: HTMLElement) {
  return Math.max(0, viewport.scrollHeight - viewport.clientHeight - viewport.scrollTop)
}

function refreshPanelAutoFollow(panel: ScrollPanelKey) {
  const viewport = viewportFor(panel)
  if (!viewport) {
    return
  }
  scrollPanels[panel].autoFollow = distanceFromViewportBottom(viewport) <= viewport.clientHeight / 3
}

function handleViewportScroll(panel: ScrollPanelKey) {
  refreshPanelAutoFollow(panel)
}

async function maybeFollowPanel(panel: ScrollPanelKey) {
  await nextTick()
  const viewport = viewportFor(panel)
  if (!viewport) {
    return
  }

  if (!scrollPanels[panel].autoFollow && distanceFromViewportBottom(viewport) > viewport.clientHeight / 3) {
    return
  }

  viewport.scrollTop = viewport.scrollHeight
  refreshPanelAutoFollow(panel)
}

function sceneInteractableKindLabel(kind: SceneInteractableKind) {
  const labels: Record<SceneInteractableKind, string> = {
    player: '玩家',
    npc: '人物',
    shop: '物件',
    monster: '妖兽',
    resource: '采集点',
    loot: '遗落物',
    hazard: '禁制',
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

async function ensureCodexLoaded(category = selectedCodexCategory.value) {
  const normalizedCategory = category || selectedCodexCategory.value
  selectedCodexCategory.value = normalizedCategory
  await store.loadCodexList(normalizedCategory)

  const entries = (store.codexEntries as Record<string, any>[] | undefined) ?? []
  const nextEntryId =
    selectedCodexEntryId.value && entries.some((entry) => String(entry.entryId ?? '') === selectedCodexEntryId.value)
      ? selectedCodexEntryId.value
      : String(entries[0]?.entryId ?? '')

  selectedCodexEntryId.value = nextEntryId
  if (!nextEntryId) {
    store.codexDetail = null
    return
  }

  await store.loadCodexDetail(nextEntryId)
}

async function openCodexOverlay(category = selectedCodexCategory.value) {
  activeTab.value = 'codex'
  activeOverlay.value = 'codex'
  try {
    await ensureCodexLoaded(category)
  } catch (error) {
    setError(error)
  }
}

async function openCodexEntry(entryId: string, categoryHint = '') {
  activeTab.value = 'codex'
  activeOverlay.value = 'codex'

  try {
    if (categoryHint) {
      selectedCodexCategory.value = categoryHint
      await store.loadCodexList(categoryHint)
    } else if (!store.codexEntries.length) {
      await store.loadCodexList(selectedCodexCategory.value)
    }

    await store.loadCodexDetail(entryId)
    const resolvedCategory = String(store.codexDetail?.category ?? categoryHint ?? '')
    if (resolvedCategory && resolvedCategory !== selectedCodexCategory.value) {
      selectedCodexCategory.value = resolvedCategory
      await store.loadCodexList(resolvedCategory)
    }
    selectedCodexEntryId.value = entryId
  } catch (error) {
    setError(error)
  }
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
  if (overlay !== 'commands' && overlay !== 'scene' && overlay !== 'messages') {
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
    if (!selectedOriginId.value) {
      throw new Error('请先选择一个出身。')
    }

    await store.createCharacter(characterName.value, selectedOriginId.value)
    characterName.value = ''
  } catch (error) {
    setError(error)
  }
}

async function retryBootstrap() {
  try {
    await store.bootstrap()
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

async function submitOverlayChat() {
  const raw = overlayChatText.value.trim()
  if (!raw) {
    return
  }

  try {
    await store.executeCommand(`chat ${chatChannel.value} ${raw}`)
    overlayChatText.value = ''
  } catch (error) {
    setError(error)
  }
}

function applyAction(action: CommandAction) {
  if (action.codexEntryId) {
    void openCodexEntry(action.codexEntryId, action.codexCategory ?? '')
    return
  }

  if (action.codexCategory) {
    void openCodexOverlay(action.codexCategory)
    return
  }

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

onMounted(async () => {
  if (!store.authenticated) {
    return
  }

  try {
    await store.bootstrap()
    await maybeFollowPanel('topChat')
    await maybeFollowPanel('mainStory')
  } catch (error) {
    setError(error)
  }
})

watch(activeOverlay, async (value) => {
  try {
    if (value === 'rank' && store.authenticated) {
      await store.loadRankings(store.rankingType)
    }
    if (value === 'codex' && store.authenticated) {
      await ensureCodexLoaded(selectedCodexCategory.value)
    }
  } catch (error) {
    setError(error)
  }
})

watch(
  () => availableOrigins.value.map((origin) => String(origin.originId ?? '')).join('|'),
  () => {
    if (!availableOrigins.value.length) {
      selectedOriginId.value = ''
      return
    }

    if (availableOrigins.value.some((origin) => String(origin.originId ?? '') === selectedOriginId.value)) {
      return
    }

    selectedOriginId.value = String(availableOrigins.value[0]?.originId ?? '')
  },
  { immediate: true },
)

watch(
  [() => store.account, () => store.authenticated],
  ([account, authenticated]) => {
    if (account !== trackedTimelineAccount) {
      trackedTimelineAccount = account
      resetMessageTimelines()
      return
    }

    if (!authenticated) {
      trackedTimelineAccount = ''
      resetMessageTimelines()
    }
  },
  { immediate: true },
)

watch(
  () => store.events.map((event) => String(event.eventId ?? '')).join('|'),
  () => {
    const lines: DenseLine[] = []
    store.events.forEach((event) => {
      const eventId = Number(event.eventId ?? 0)
      if (eventId > 0 && processedMainEventIds.has(eventId)) {
        return
      }
      if (eventId > 0) {
        processedMainEventIds.add(eventId)
      }
      if (isChatEvent(event)) {
        return
      }

      lines.push({
        key: `event-line-${eventId || mainTimelineSequence + lines.length + 1}`,
        tag: eventChannelLabel(event),
        text: formatEventText(event),
        tone: eventTone(event) as DenseLine['tone'],
      })
    })

    appendMainTimeline(lines)
  },
  { immediate: true },
)

watch(
  () => currentSceneId.value,
  (sceneId, previousSceneId) => {
    if (!sceneId || sceneId === previousSceneId) {
      return
    }
    appendMainTimeline(buildSceneSnapshotLines())
  },
  { immediate: true, flush: 'post' },
)

watch(
  () => store.lastResult,
  (result) => {
    if (!result) {
      return
    }
    appendMainTimeline(buildResultTimelineLines(result))
  },
  { flush: 'post' },
)

watch(
  () => latestChatEventId.value,
  () => {
    void maybeFollowPanel('topChat')
    if (activeOverlay.value === 'messages') {
      void maybeFollowPanel('chatOverlay')
    }
  },
  { flush: 'post' },
)

watch(
  () => mainTimeline.value.length,
  () => {
    void maybeFollowPanel('mainStory')
  },
  { flush: 'post' },
)

watch(
  () => activeOverlay.value,
  (value) => {
    if (value === 'messages') {
      void maybeFollowPanel('chatOverlay')
    }
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
  <div class="shell" :class="{ 'shell--game': showGameView }">
    <header v-if="showGameView" class="top-banner">
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

    <p v-if="displayError" class="error-banner">{{ displayError }}</p>

    <section v-if="!store.authenticated" class="auth-card">
      <div class="card-heading">
        <h2>进入凡人世界</h2>
        <p>沿用现有服务端协议，登录后直接进入移动端主界面。新注册账号仅支持英文字母和数字，老账号不受影响。</p>
      </div>
      <div class="form-grid">
        <label>
          <span>账号名</span>
          <input v-model="account" autocomplete="username" placeholder="例如 hanli001" />
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

    <section v-else-if="showCreateCharacterView" class="auth-card">
      <div class="card-heading">
        <h2>塑造新角色</h2>
        <p>当前账号：{{ store.account }}。先定姓名，再选一处人界出身；如果这个账号本来就有角色，也可以先重新检查存档。</p>
      </div>
      <div class="form-grid single-column">
        <label>
          <span>角色名</span>
          <input v-model="characterName" maxlength="24" placeholder="例如 韩立" @keyup.enter="createCharacter()" />
        </label>
        <div>
          <span>人界出身</span>
          <div class="origin-grid">
            <button
              v-for="origin in availableOrigins"
              :key="origin.originId"
              type="button"
              class="origin-card"
              :class="{ active: selectedOriginId === String(origin.originId) }"
              @click="selectedOriginId = String(origin.originId)"
            >
              <strong>{{ origin.originName }}</strong>
              <small>{{ origin.homeland }}</small>
              <p>{{ origin.description }}</p>
            </button>
          </div>
          <article v-if="selectedOrigin" class="detail-card origin-preview-card">
            <p class="detail-title">{{ selectedOrigin.originName }} · {{ selectedOrigin.raceName }}</p>
            <p>{{ selectedOrigin.description }}</p>
            <p>故土：{{ selectedOrigin.homeland }}</p>
          </article>
        </div>
      </div>
      <div class="action-row">
        <button type="button" class="primary-button" :disabled="store.loading || !selectedOriginId" @click="createCharacter()">
          踏入修仙路
        </button>
        <button type="button" class="secondary-button" :disabled="store.loading" @click="retryBootstrap()">重新检查角色</button>
        <button type="button" class="ghost-button" :disabled="store.loading" @click="store.logout()">返回登录</button>
      </div>
    </section>

    <main v-else-if="showGameView" class="mobile-layout">
      <section
        class="surface-panel event-panel event-panel--clickable"
        role="button"
        tabindex="0"
        @click="openOverlay('messages')"
        @keydown.enter.prevent="openOverlay('messages')"
        @keydown.space.prevent="openOverlay('messages')"
      >
        <div class="channel-header">
          <span class="channel-badge">频道</span>
          <div class="channel-meta">
            <span class="channel-chip">{{ scene.regionName || '未知地域' }}</span>
            <span class="channel-chip">{{ scene.sceneName || '未知场景' }}</span>
            <span class="channel-chip">{{ player.cultivation?.realmName || '凡躯' }}</span>
            <span class="channel-chip">点此展开聊天</span>
          </div>
        </div>
        <div ref="eventViewport" class="event-stream dense-log" @scroll.stop="handleViewportScroll('topChat')">
          <article v-for="event in chatEvents" :key="event.eventId" class="event-item dense-item">
            <span class="event-tag tone-chat">{{ eventChannelLabel(event) }}</span>
            <p class="event-line">{{ formatEventText(event) }}</p>
          </article>
          <p v-if="chatEvents.length === 0" class="empty-text">当前还没有聊天消息，点这里展开后就能直接发言。</p>
        </div>
      </section>

      <section class="surface-panel scene-panel">
        <div class="section-row scene-section-header">
          <div class="scene-heading">
            <p class="section-kicker">主界面</p>
            <h2>{{ scene.sceneName || '修行记录' }}</h2>
            <p class="scene-header-summary">
              章节：{{ displayProgressionChapter }} · 当前可见 {{ sceneInteractables.length }} 项 · 去路 {{ exits.length }} 条
            </p>
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
              <div class="scene-board-copy">
                <p class="scene-board-title">{{ scene.regionName || '修行界' }}</p>
                <p class="scene-board-subtitle">{{ sceneMissionText }}</p>
              </div>
              <div class="scene-board-badges">
                <span class="scene-board-badge">可见 · {{ sceneInteractables.length }}</span>
                <span class="scene-board-badge">去路 · {{ exits.length }}</span>
              </div>
            </div>

            <div class="story-console">
              <div ref="storyViewport" class="story-log" @scroll="handleViewportScroll('mainStory')">
                <article
                  v-for="line in mainTimeline"
                  :key="line.key"
                  class="story-line"
                  :class="denseToneClass(line.tone)"
                >
                  <span class="story-tag">[{{ line.tag }}]</span>
                  <p class="story-text">{{ line.text }}</p>
                </article>
                <p v-if="mainTimeline.length === 0" class="empty-text">当前还没有新的场景记录，先试着观察、交谈或移动吧。</p>
              </div>
            </div>

            <div class="scene-presence-strip">
              <span class="meta-tag" v-for="entry in scenePlayers" :key="entry.account">玩家 · {{ entry.characterName }}</span>
              <span class="meta-tag" v-for="npc in npcs" :key="npc.npcId">人物 · {{ npc.name }}</span>
              <span class="meta-tag" v-for="sceneItem in sceneItems" :key="`${sceneItem.itemId}-${sceneItem.source}`">物件 · {{ sceneItem.name }}</span>
              <span class="meta-tag" v-for="node in sceneResourceNodes" :key="node.nodeId">采点 · {{ node.name }}</span>
              <span class="meta-tag" v-for="loot in sceneGroundLoots" :key="loot.lootId">遗落 · {{ loot.itemName }}</span>
              <span class="meta-tag" v-for="hazard in sceneHazards" :key="hazard.hazardId">禁制 · {{ hazard.name }}</span>
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

    <section v-else-if="store.authenticated" class="auth-card">
      <div class="card-heading">
        <h2>正在恢复角色</h2>
        <p>当前账号：{{ store.account }}。登录态还在，但角色数据还没有恢复成可玩的状态；可以重新检查存档，或者退出后重新登录。</p>
      </div>
      <div class="action-row">
        <button type="button" class="primary-button" :disabled="store.loading" @click="retryBootstrap()">重新检查角色</button>
        <button type="button" class="secondary-button" :disabled="store.loading" @click="store.logout()">返回登录</button>
      </div>
    </section>

    <div v-if="store.authenticated && activeOverlay !== 'none'" class="overlay-backdrop" @click.self="closeOverlay()">
      <section class="overlay-sheet" :class="{ 'overlay-sheet--wide': activeOverlay === 'map' || activeOverlay === 'commands' || activeOverlay === 'codex' }">
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

        <div v-else-if="activeOverlay === 'messages'" class="overlay-body overlay-body--messages">
          <div ref="chatOverlayViewport" class="messages-overlay-log" @scroll="handleViewportScroll('chatOverlay')">
            <article v-for="event in chatEvents" :key="`overlay-${event.eventId}`" class="event-item dense-item">
              <span class="event-tag tone-chat">{{ eventChannelLabel(event) }}</span>
              <p class="event-line">{{ formatEventText(event) }}</p>
            </article>
            <p v-if="chatEvents.length === 0" class="empty-text">当前还没有聊天消息，发一句话试试看吧。</p>
          </div>

          <div class="messages-overlay-composer">
            <div class="mode-row messages-mode-row">
              <button
                type="button"
                class="mode-button"
                :class="{ active: chatChannel === 'world' }"
                @click="chatChannel = 'world'"
              >
                世界聊天
              </button>
              <button
                type="button"
                class="mode-button"
                :class="{ active: chatChannel === 'team' }"
                @click="chatChannel = 'team'"
              >
                队伍聊天
              </button>
              <button type="button" class="mode-button" @click="overlayChatText = ''">清空输入</button>
            </div>
            <form class="command-form messages-command-form" @submit.prevent="submitOverlayChat()">
              <input
                v-model="overlayChatText"
                :placeholder="chatChannel === 'world' ? '输入要发送到世界频道的话' : '输入要发送给队伍成员的话'"
                :disabled="store.loading"
              />
              <button class="primary-button" :disabled="store.loading">发送</button>
            </form>
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
              <p>出身：{{ player.race?.originName || '未定' }} · {{ player.race?.homeland || '人界' }}</p>
              <p>本命功法：{{ player.cultivation?.primarySkill || '长春功' }}</p>
              <p>功法等级：{{ player.cultivation?.skillLevel || 1 }}</p>
              <p>突破需求：{{ player.cultivation?.exp || 0 }} / {{ player.cultivation?.nextBreakthroughExp || 0 }}</p>
              <p>章节进度：{{ displayProgressionChapter }}</p>
              <p>宗门贡献：{{ player.sectContribution || 0 }}</p>
              <p>已解锁区域：{{ (player.unlockedRegions || []).join('、') || '七玄门' }}</p>
            </div>
            <article class="detail-card">
              <p class="detail-title">基础属性</p>
              <p>神识 {{ player.baseAttributes?.spi || 0 }} · 经脉 {{ player.baseAttributes?.gin || 0 }} · 炼体 {{ player.baseAttributes?.str || 0 }}</p>
              <p>灵觉 {{ player.baseAttributes?.per || 0 }} · 悟性 {{ player.baseAttributes?.int || 0 }} · 魅力 {{ player.baseAttributes?.cha || 0 }} · 机缘 {{ player.baseAttributes?.luc || 0 }}</p>
              <p>法力 {{ currentStatusAttributes.mana || 0 }} / {{ player.statusAttributes?.mana || 0 }} · 神念 {{ currentStatusAttributes.sen || 0 }} / {{ player.statusAttributes?.sen || 0 }} · 气力 {{ currentStatusAttributes.sta || 0 }} / {{ player.statusAttributes?.sta || 0 }}</p>
            </article>
            <article class="detail-card">
              <p class="detail-title">技艺概览</p>
              <p>技能：{{ ((player.skills || []) as Record<string, any>[]).map((item) => item.name).join('、') || '尚未习得' }}</p>
              <p>法术：{{ ((player.spells || []) as Record<string, any>[]).filter((item) => item.unlocked).map((item) => item.name).join('、') || '尚未习得' }}</p>
              <p>配方：{{ ((player.recipes || []) as Record<string, any>[]).filter((item) => item.unlocked).map((item) => item.name).join('、') || '尚未掌握' }}</p>
            </article>
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

          <div v-else-if="activeInfoTab === 'codex'" class="detail-stack">
            <div class="tab-strip codex-category-strip">
              <button
                v-for="category in codexCategories"
                :key="category"
                type="button"
                class="tab-button command-tab-button"
                :class="{ active: selectedCodexCategory === category }"
                @click="void openCodexOverlay(category)"
              >
                {{ category }}
              </button>
            </div>

            <article v-if="selectedCodexSummary" class="detail-card">
              <p class="detail-title">{{ selectedCodexCategory }}</p>
              <p>{{ selectedCodexSummary.summary }}</p>
              <p>未读状态：{{ selectedCodexSummary.unread ? '有新资料' : '已阅' }}</p>
            </article>

            <div class="codex-entry-list">
              <button
                v-for="entry in codexEntries"
                :key="entry.entryId"
                type="button"
                class="detail-card codex-entry-card"
                :class="{ active: selectedCodexEntryId === String(entry.entryId) }"
                @click="void openCodexEntry(String(entry.entryId), selectedCodexCategory)"
              >
                <p class="detail-title">{{ entry.title }}</p>
                <p>{{ entry.summary }}</p>
                <p>{{ entry.unlocked ? '已解锁' : '资料未明' }}<span v-if="entry.unread"> · 新</span></p>
              </button>
            </div>

            <article v-if="codexDetail" class="detail-card codex-detail-card">
              <p class="detail-title">{{ codexDetail.title }}</p>
              <p>{{ codexDetail.summary }}</p>
              <p>{{ codexDetail.content }}</p>
              <p v-if="(codexDetail.relatedSceneIds || []).length > 0">关联场景：{{ codexDetail.relatedSceneIds.join('、') }}</p>
              <p v-if="(codexDetail.relatedNpcIds || []).length > 0">关联人物：{{ codexDetail.relatedNpcIds.join('、') }}</p>
              <p v-if="(codexDetail.relatedMonsterIds || []).length > 0">关联妖兽：{{ codexDetail.relatedMonsterIds.join('、') }}</p>
              <p v-if="(codexDetail.relatedItemIds || []).length > 0">关联物件：{{ codexDetail.relatedItemIds.join('、') }}</p>
            </article>

            <p v-if="codexEntries.length === 0" class="empty-text">这一分类的资料尚未解锁，继续探索、交谈、击败敌手或取得关键物品后会逐步开启。</p>
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
