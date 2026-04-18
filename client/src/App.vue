<script setup lang="ts">
import { computed, nextTick, onMounted, reactive, ref, watch } from 'vue'

import {
  narrativeArcLabels,
  narrativeArcOrderForChapter,
  narrativeArcOrderForQuest,
  narrativeArcOrderForScene,
  sceneFollowupHints,
} from '@/lib/narrative'
import { formatHelpTopicTitle, formatProgressStageLabel } from '@/lib/progression'
import { useGameStore } from '@/stores/game'
import { directionLabelMap, worldMapEdges, worldMapNodes } from '@/lib/world-map'

type SideTab = 'player' | 'quests' | 'inventory' | 'map' | 'team' | 'rank' | 'codex'
type ComposerMode = 'chat' | 'command'
type RankingType = 'realm' | 'wealth' | 'combat' | 'alchemy' | 'travel' | 'bounty' | 'chief'
type CommandCategoryId =
  | 'social'
  | 'explore'
  | 'loops'
  | 'tasks'
  | 'combat'
  | 'spell'
  | 'cultivation'
  | 'gather'
  | 'alchemy'
  | 'trade'
  | 'group'
  | 'manual'
type OverlayPanel = 'none' | 'commands' | 'scene' | 'messages' | 'settings' | SideTab
type SceneInteractableKind = 'player' | 'npc' | 'shop' | 'monster' | 'resource' | 'loot' | 'hazard'
type ScrollPanelKey = 'topChat' | 'mainStory' | 'chatOverlay'
type PanelRenderMode = 'board_block' | 'dossier_block' | 'roster_block' | 'ascii_map' | 'notice_block'

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
  entryType: 'line'
  sequence: number
}

interface TimelinePanelLine {
  key: string
  text: string
  tone?: 'normal' | 'muted' | 'accent' | 'danger' | 'positive'
}

interface TimelinePanel {
  entryType: 'panel'
  key: string
  sequence: number
  panelId: string
  mark: string
  title: string
  compactTitle: string
  summary: string
  tone: DenseLine['tone']
  compact: boolean
  renderMode: PanelRenderMode
  styleId: string
  lines: TimelinePanelLine[]
  actions: CommandAction[]
}

type TimelineEntry = TimelineLine | TimelinePanel

interface DockCommandEntry {
  key: string
  label: string
  caption: string
  command?: string
  action?: CommandAction
  panel?: 'map'
  rankingType?: RankingType
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
const selectedBackgroundId = ref('')
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
const mainTimeline = ref<TimelineEntry[]>([])
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
  wanderer_camp: [{ id: 'wanderer_dewleaf_task', title: '露叶试手', summary: '许游方想先看看你会不会做最基础的采药活。' }],
  wanderer_creek: [{ id: 'wanderer_resin_task', title: '焦土取脂', summary: '蓝药娘缺一点黄琥脂，正好能带你熟悉山野采集和风险区。' }],
  jiayuan_market: [{ id: 'qixuan_herb', title: '墨府采药', summary: '嘉元城总管正急需黄精草，你若接下此事，便能借此熟悉坊市与采集路线。' }],
  mofu_front_hall: [{ id: 'mofu_guest_token', title: '墨府来客令', summary: '墨府前厅散落着来客令牌，适合继续熟悉搜寻与拾取。' }],
  escort_post: [{ id: 'escort_token_task', title: '官道清匪', summary: '东门驿棚的沈镖头正在看人，先替他办妥第一件官道差事。' }],
  relay_station: [{ id: 'escort_seal_task', title: '封签送驿', summary: '周驿吏急着找回封签，这条线会把你正式带进护送生态。' }],
  tainan_gate: [{ id: 'ruins_old_map', title: '残垣旧图', summary: '流动牙人正在替海港一带打听旧图下落，这会把你一路引向血禁残垣与天南港。' }],
  tainan_fair: [{ id: 'fair_rumor_packet', title: '小会密闻', summary: '旧书摊想买虚天传闻，适合把太南谷里的人物与摊位继续串起来。' }],
  xin_house: [{ id: 'tainan_snake', title: '太南异胆', summary: '辛如音需要妖蛇异胆试阵，这条线会带你熟悉太南谷附近的战斗与材料循环。' }],
  array_lane: [{ id: 'tainan_array_flag', title: '阵旗回收', summary: '阵旗巷学徒正收残片，这条线更偏向阵法与拾取。' }],
  loose_camp_square: [{ id: 'loose_rumor_task', title: '棚市风声', summary: '温散人想先看看你在散修棚市里会不会找路、找人、找消息。' }],
  loose_medicine_tent: [{ id: 'loose_stone_task', title: '石林识物', summary: '青药师缺一片月壳，刚好能带你熟悉散修坊周围的采集与识物。' }],
  huangfeng_outpost: [{ id: 'huangfeng_letter', title: '黄枫谷羽信', summary: '外营弟子正等人送一封羽信入谷，这是接触黄枫谷主线的入口。' }],
  huangfeng_medicine_terrace: [{ id: 'medicine_moss', title: '药台苔引', summary: '药梯台正在收灵苔药引，适合继续熟悉采药与炼制材料。' }],
  huangfeng_scripture: [{ id: 'huangfeng_manual', title: '藏经抄卷', summary: '藏经石廊的执事在等一卷误落抄卷，这条线会把你带往黄枫谷典籍区。' }],
  spirit_beast_outer_gate: [{ id: 'spirit_feed_task', title: '外门草料差', summary: '灵兽山外门先看你能不能把最基础的草料差事办稳。' }],
  spirit_beast_beast_pen: [{ id: 'spirit_bug_task', title: '灵虫粉翅', summary: '周饲兽需要几片墨蛾翅，这条线会把你带进灵兽山的日常循环。' }],
  blood_gate: [
    { id: 'blood_forbidden_token', title: '血禁采兰', summary: '血禁执事正在收血兰验阵，这是血色禁地的正式入口任务。' },
    { id: 'blood_swamp_rescue', title: '沼泽驱邪', summary: '血雾沼泽的虫群样本也有人悬赏，适合继续深入禁地。' },
  ],
  tiannan_harbor: [
    { id: 'ruins_old_map', title: '残垣旧图', summary: '旧图真正的买家在海港，交到海商牙人手里才能换到远航门路。' },
    { id: 'chaos_sea_chart', title: '乱星海海图', summary: '海商牙人正在找修补海图禁制的材料，这会继续把你推向乱星海。' },
  ],
  harbor_backbay: [{ id: 'harbor_shell_task', title: '后湾盐壳', summary: '吴老渔想先看看你会不会做最基础的后湾海猎活。' }],
  harbor_net_field: [{ id: 'harbor_chart_task', title: '后湾海图', summary: '彭网师正在整理后湾潮路，正缺一个能跑腿也能下水的人。' }],
  smuggler_alley: [{ id: 'harbor_signal', title: '暗巷接头', summary: '暗潮小巷有人在收接头暗号，适合补足港口支线。' }],
  chaos_sea_port: [
    { id: 'captain_supply', title: '远航补给', summary: '曲船主还缺远航物资，这条线会把你送上真正的海路。' },
    { id: 'outer_sea_trail', title: '外海见闻', summary: '近港只是门槛，真正往结丹门前摸去，要从外海中层开始学会认潮与认压。' },
  ],
  outer_isles_wharf: [{ id: 'outer_pearl_task', title: '群岛采珠', summary: '群岛小埠的人先看你会不会摸珠、会不会识潮，再决定值不值得带你出海。' }],
  outer_isles_market: [{ id: 'outer_coral_task', title: '风暴珊瑚', summary: '蓝采珠缺一截风暴珊瑚，这会让你更早接触群岛高价值采集点。' }],
  chaos_sea_ship: [
    { id: 'demon_fish_core', title: '妖鱼内丹', summary: '船上术士正在收妖鱼内丹，可顺手把海战与法术线串起来。' },
    { id: 'gold_core_gate', title: '结丹之门', summary: '若已把虚天祭台那条线走通，甲板术士会开始把你往真正的结丹主丹上带。' },
  ],
  chaos_sea_isle: [{ id: 'chaos_relic', title: '孤岛残碑', summary: '残碑孤岛的隐士需要残钥碎片，这会引出虚天殿前置线。' }],
  xutian_hall: [
    { id: 'xutian_key', title: '虚天残钥', summary: '守门残灵在等残钥，交齐后才能真正逼近虚天殿内层。' },
    { id: 'core_ruin_heart', title: '古修残环', summary: '守门残灵若开始提残环，说明你已被默认能往更深处赌一轮结丹线。' },
  ],
  xutian_star_platform: [
    { id: 'xutian_star_map', title: '星纹演算', summary: '祭台残灵正在收星纹拓片，这是逼近内殿玄门的最后一步。' },
    { id: 'nascent_soul_gate', title: '凝婴前夜', summary: '当祭台残灵开始谈星渊与婴火时，这条线就已经不再是普通结丹后段的准备了。' },
  ],
  xutian_rune_garden: [{ id: 'void_crystal_task', title: '裂隙晶砂', summary: '记纹残灵只认裂隙晶砂，这条线会把你带进虚天残区的炼制与高危循环。' }],
  xutian_shard_steps: [{ id: 'void_rune_task', title: '残纹归位', summary: '拾屑傀还在执行古老命令，你可以顺着它的需求继续深探残区。' }],
}

const sceneSectOffers: Record<string, Array<{ command: string; name: string; summary: string }>> = {
  qixuan_hall: [{ command: 'join qixuan_gate', name: '七玄门', summary: '七玄门适合凡人启程，在这里能打好最初的修行和江湖根基。' }],
  huangfeng_hall: [{ command: 'join huangfeng_valley', name: '黄枫谷', summary: '黄枫谷重视基础与心性，是越国七派里较稳的一条成长路线。' }],
  spirit_beast_outer_gate: [{ command: 'join spirit_beast_mountain', name: '灵兽山', summary: '灵兽山更偏向驭兽、照料、采药与灵虫线，适合喜欢长期养成和资源循环的玩家。' }],
  yanyue_peak: [{ command: 'join yanyue_sect', name: '掩月宗', summary: '掩月宗重视身法与法门，入门后更适合往灵动轻灵路线修行。' }],
}

const categoryLabels: Record<CommandCategoryId, string> = {
  social: '交流',
  explore: '探索',
  loops: '循环',
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

const rankingOptions: Array<{ id: RankingType; label: string }> = [
  { id: 'realm', label: '境界榜' },
  { id: 'wealth', label: '财富榜' },
  { id: 'combat', label: '战力榜' },
  { id: 'alchemy', label: '丹道榜' },
  { id: 'travel', label: '游历榜' },
  { id: 'bounty', label: '赏金榜' },
  { id: 'chief', label: '首席榜' },
]

const scene = computed(() => store.scene ?? {})
const player = computed(() => store.player ?? {})
const availableOrigins = computed(() => (store.availableOrigins as Record<string, any>[] | undefined) ?? [])
const availableBackgrounds = computed(() => (store.availableBackgrounds as Record<string, any>[] | undefined) ?? [])
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
const scenePaletteClass = computed(() => {
  if (!showGameView.value) {
    return ''
  }
  const raw = String(scene.value.paletteId ?? 'warm_ink').trim() || 'warm_ink'
  return `shell-palette-${raw.replace(/[^a-zA-Z0-9_-]/g, '_')}`
})
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
const selectedBackground = computed(
  () =>
    availableBackgrounds.value.find((background) => String(background.backgroundId ?? '') === selectedBackgroundId.value) ??
    availableBackgrounds.value[0] ??
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

function buildCatalogCommandAction(definition: Record<string, any>): CommandAction {
  const command = String(definition.command ?? '')
  const composerMode = String(definition.composerMode ?? 'command')
  const chat = String(definition.chatChannel ?? '')
  const aliases = ((definition.aliases as string[] | undefined) ?? [])
    .map((item) => String(item ?? '').trim())
    .filter(Boolean)
  const usage = String(definition.usage ?? '').trim()
  const targetHint = String(definition.targetHint ?? '').trim()
  const visibilityScope = String(definition.visibilityScope ?? '').trim()
  const detailParts = [
    String(definition.summary ?? '通过底层命令目录提供的操作。'),
    usage ? `用法 ${usage}` : '',
    aliases.length > 0 ? `别名 ${aliases.join('/')}` : '',
    targetHint ? `目标 ${targetHint}` : '',
    visibilityScope === 'scene' ? '同场可用' : visibilityScope === 'global' ? '行走天下皆可用' : '',
  ].filter(Boolean)
  return {
    key: `catalog-${String(definition.commandId ?? definition.label ?? command)}`,
    label: String(definition.label ?? command ?? '指令'),
    detail: detailParts.join(' · '),
    command,
    execute: Boolean(definition.executeImmediately),
    composer: composerMode === 'chat' ? 'chat' : 'command',
    chatChannel: chat === 'team' ? 'team' : 'world',
    prefillText: composerMode === 'chat' ? '' : command,
  }
}

function mergeActionsWithCatalog(
  categoryId: CommandCategoryId,
  actions: CommandAction[],
  catalog: Record<string, any>[],
) {
  const actionSignature = (action: CommandAction) =>
    action.composer === 'chat'
      ? `chat:${action.chatChannel ?? 'world'}`
      : `command:${action.command ?? ''}`

  const merged = [...actions]
  const seen = new Set(actions.map((action) => actionSignature(action)))

  catalog
    .filter((definition) => String(definition.category ?? '') === categoryId)
    .forEach((definition) => {
      const action = buildCatalogCommandAction(definition)
      const signature = actionSignature(action)
      if (seen.has(signature)) {
        return
      }
      seen.add(signature)
      merged.push(action)
    })

  return merged
}

const commandCategories = computed(() => {
  const commandCatalog = (player.value.commandCatalog as Record<string, any>[] | undefined) ?? []
  const availableSceneQuestOffers = (sceneQuestOffers[currentSceneId.value] ?? []).filter(
    (quest) => !currentQuestIds.value.has(quest.id),
  )
  const readySceneQuests = quests.value.filter(
    (quest) =>
      isQuestReadyToSubmit(quest) &&
      (sceneQuestOffers[currentSceneId.value] ?? []).some((offer) => offer.id === String(quest.questId ?? '')),
  )
  const firstMonster = monsters.value[0]

  const social = mergeActionsWithCatalog('social', [
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
  ], commandCatalog)

  const explore = mergeActionsWithCatalog('explore', [
    { key: 'look', label: '查看场景', detail: '重读当前场景描述。', command: 'look' },
    { key: 'map', label: '查看地图', detail: '查看整张人界地图。', command: 'map' },
    ...exits.value.map((exit) => ({
      key: `go-${String(exit.direction)}`,
      label: `前往${directionLabel(String(exit.direction))}`,
      detail: `移动到${String(exit.targetSceneName ?? exit.targetSceneId ?? '未知地点')}。`,
      command: `go ${String(exit.direction)}`,
    })),
  ], commandCatalog)

  const taskActions = mergeActionsWithCatalog('tasks', [
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
  ], commandCatalog)

  const loops = mergeActionsWithCatalog('loops', [
    {
      key: 'board',
      label: '公共委托',
      detail: '查看当前区域可接的事务和循环。',
      command: 'board',
    },
    {
      key: 'wanted',
      label: '悬赏目标',
      detail: '查看当前区域的怪物、掉落和风险。',
      command: 'wanted',
    },
    {
      key: 'claim',
      label: '领取奖励',
      detail: '查看是否有阶段或身份奖励可领。',
      command: 'claim',
    },
  ], commandCatalog)

  const consumables = inventory.value.filter((item) => ['consumable'].includes(String(item.itemType ?? '')))
  const combat = mergeActionsWithCatalog('combat', [
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
  ], commandCatalog)

  const spell = mergeActionsWithCatalog('spell', [
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
  ], commandCatalog)

  const cultivation = mergeActionsWithCatalog('cultivation', [
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
  ], commandCatalog)

  const gather = mergeActionsWithCatalog('gather', [
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
  ], commandCatalog)

  const alchemy = mergeActionsWithCatalog('alchemy', [
    ...(((player.value.recipes as Record<string, any>[] | undefined) ?? []).filter((item) => Boolean(item.unlocked))).map((item) => ({
      key: `brew-${String(item.recipeId)}`,
      label: `炼制·${String(item.name)}`,
      detail: String(item.description ?? '按配方炼制丹药与辅助物。'),
      command: `brew ${String(item.recipeId)}`,
    })),
  ], commandCatalog)

  const trade = mergeActionsWithCatalog('trade', [
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
  ], commandCatalog)

  const group = mergeActionsWithCatalog('group', [
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
  ], commandCatalog)

  const manual = mergeActionsWithCatalog('manual', codexCategories.map((category) => {
    const summary =
      codexSummaries.value.find((item) => String(item.category ?? item.entryId ?? '') === category) ?? null
    return {
      key: `manual-${category}`,
      label: category,
      detail: String(summary?.summary ?? '打开分类手册，查看当前已解锁条目。'),
      codexCategory: category,
    }
  }), commandCatalog)

  return [
    { id: 'social' as const, label: categoryLabels.social, actions: social },
    { id: 'explore' as const, label: categoryLabels.explore, actions: explore },
    { id: 'loops' as const, label: categoryLabels.loops, actions: loops },
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
  activeOverlay.value !== 'messages' &&
  activeOverlay.value !== 'settings'
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
        `境界：${formatProgressStageLabel(String(scenePlayer.realmName ?? '凡躯'))}`,
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
      {
        key: `interactable-ask-${String(npc.npcId ?? npcName)}`,
        label: `追问·${npcName}`,
        detail: '把 ask 指令先铺好，再顺着对话继续往深里问。',
        command: `ask ${npcName} `,
        execute: false,
        composer: 'command',
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

const statusPromptPrimary = computed(() => {
  const supplied = String(player.value.statusLineText ?? '').trim()
  if (supplied) {
    return supplied
  }
  const role = String(player.value.characterName ?? store.account ?? '无名散修') || '无名散修'
  const realm = formatProgressStageLabel(String(player.value.cultivation?.realmName ?? player.value.stageLabel ?? '凡躯'))
  const hp = `${String(player.value.hp ?? 0)}/${String(player.value.maxHp ?? 0)}`
  const mana = `${String(currentStatusAttributes.value.mana ?? player.value.statusAttributes?.mana ?? 0)}/${String(player.value.statusAttributes?.mana ?? 0)}`
  const sta = `${String(currentStatusAttributes.value.sta ?? player.value.statusAttributes?.sta ?? 0)}/${String(player.value.statusAttributes?.sta ?? 0)}`
  const location = sceneDisplayTitle.value
  const sceneCount = Number(scenePlayers.value.length ?? 0)
  return `${role} | ${realm} | 气血 ${hp} | 法力 ${mana} | 气力 ${sta} | ${location} | 同场 ${sceneCount} 人`
})

const statusPromptSecondary = computed(() => {
  const supplied = String(player.value.subpromptText ?? '').trim()
  if (supplied) {
    return supplied
  }
  const sectName = String(player.value.sect?.sectName ?? '').trim() || '散修'
  const channelMode =
    composerMode.value === 'chat'
      ? chatChannel.value === 'world'
        ? '世声'
        : '队声'
      : '落令'
  const roomLayer = String(scene.value.roomLayer ?? '').trim()
  const riskLevel = String(scene.value.riskLevel ?? '').trim()
  const riskPrompt = player.value.newbieProtected
    ? '山门庇护'
    : roomLayerFlavor(roomLayer) || (riskLevel ? `险候 ${riskLevel}` : '风平浪静')
  const mood = String(scene.value.ambientMood ?? '').trim()
  return [channelMode, sectName, riskPrompt, mood].filter(Boolean).join(' | ')
})

const sceneInlineTargets = computed(() => sceneInteractables.value.slice(0, 10))

const sceneExitTargets = computed(() => exits.value.slice(0, 6))

const shortCommandSpec: Record<
  string,
  { key: string; label: string; caption: string; command?: string; rankingType?: RankingType; panel?: 'map' }
> = {
  look: { key: 'dock-look', label: '看', caption: '重观', command: 'here' },
  here: { key: 'dock-look', label: '看', caption: '重观', command: 'here' },
  listen: { key: 'dock-listen', label: '听', caption: '风声', command: 'listen' },
  talk: { key: 'dock-talk', label: '问', caption: '交谈' },
  travel: { key: 'dock-travel', label: '行', caption: '路引', command: 'travel' },
  journal: { key: 'dock-journal', label: '札', caption: '札记', command: 'journal' },
  bag: { key: 'dock-bag', label: '囊', caption: '行囊', command: 'bag' },
  score: { key: 'dock-score', label: '我', caption: '内观', command: 'score' },
  map: { key: 'dock-map', label: '图', caption: '舆图', panel: 'map' },
}

const dockCommandEntries = computed<DockCommandEntry[]>(() => {
  const focusedNpcAction =
    selectedSceneInteractable.value?.kind === 'npc'
      ? selectedSceneInteractable.value.actions.find((action) => String(action.command ?? '').startsWith('talk '))
      : undefined
  const fallbackNpc = sceneInteractables.value.find((item) => item.kind === 'npc')
  const talkAction =
    focusedNpcAction ??
    fallbackNpc?.actions.find((action) => String(action.command ?? '').startsWith('talk ')) ??
    ({
      key: 'dock-talk-prefill',
      label: '问路求教',
      detail: '切到原始指令，手动填写要交谈的人物。',
      command: 'talk ',
      execute: false,
      composer: 'command',
    } as CommandAction)

  const shortCommands = ((player.value.availableShortCommands as string[] | undefined) ?? []).map((item) =>
    String(item ?? '')
      .trim()
      .toLowerCase(),
  )
  const desired = shortCommands.length > 0 ? shortCommands : ['look', 'listen', 'talk', 'travel', 'journal', 'bag', 'score', 'rank']
  const entries: DockCommandEntry[] = []
  desired.forEach((command) => {
    if (entries.length >= 8) {
      return
    }
    if (command === 'talk') {
      entries.push({ key: 'dock-talk', label: '问', caption: '交谈', action: talkAction })
      return
    }
    if (command === 'rank') {
      entries.push({ key: 'dock-rank', label: '榜', caption: '名榜', rankingType: store.rankingType })
      return
    }

    const spec = shortCommandSpec[command]
    if (!spec) {
      return
    }

    entries.push({
      key: spec.key,
      label: spec.label,
      caption: spec.caption,
      command: spec.command,
      panel: spec.panel,
      rankingType: spec.rankingType,
    })
  })
  return entries
})

const activeOverlayTitle = computed(() => {
  if (activeOverlay.value === 'settings') {
    return '卷末杂记'
  }
  if (activeOverlay.value === 'codex') {
    return selectedCodexCategory.value || '手册'
  }
  if (activeOverlay.value === 'none') {
    return ''
  }
  return '手册'
})

const activeOverlayCorner = computed(() => {
  if (activeOverlay.value === 'settings') {
    return store.account || '卷末'
  }
  if (activeOverlay.value === 'codex') {
    return '手册长卷'
  }
  return scene.value.sceneName || '修行界'
})

const composerTitle = computed(() =>
  composerMode.value === 'chat'
    ? chatChannel.value === 'world'
      ? '世界风声'
      : '队伍风声'
    : '落笔行令',
)

const composerPlaceholder = computed(() => {
  if (composerMode.value === 'chat') {
    return chatChannel.value === 'world' ? '此处落字，话会顺着风声传去世界频道' : '写下要传给队伍同伴的话'
  }
  return '直接输入指令，例如 look、talk 厉飞雨、go north'
})

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
    const questKind = String(activeQuest.questKind ?? '').trim()
    const issuerHint = String(activeQuest.issuerHint ?? '').trim()
    const repeatable = Boolean(activeQuest.repeatable)
    const flavorParts = [
      `行途所系：${String(activeQuest.title)}`,
      questKind ? `此事属${questKind}` : '',
      issuerHint ? `源头在${issuerHint}` : '',
      `眼下火候 ${String(activeQuest.progress ?? 0)} / ${String(activeQuest.target ?? 0)}`,
      repeatable ? '日后仍可再走一轮' : '',
    ].filter(Boolean)
    return flavorParts.join('，') + '。'
  }

  const localBoardEntries = ((scene.value.localBoardEntries as Record<string, any>[] | undefined) ?? []).filter((entry) =>
    String(entry.title ?? '').trim(),
  )
  if (localBoardEntries.length > 0) {
    const firstEntry = localBoardEntries[0]
    return `此地近来多谈「${String(firstEntry.title ?? '近事')}」：${String(firstEntry.summary ?? '')}`
  }

  const questOffer = (sceneQuestOffers[currentSceneId.value] ?? []).find(
    (quest) => !quests.value.some((item) => String(item.questId ?? '') === quest.id),
  )
  if (questOffer) {
    return `此地似有新事可问：${questOffer.title}。`
  }

  const followupHint = sceneFollowupHints[currentSceneId.value]
  if (followupHint) {
    return followupHint
  }

  if (currentNarrativeArc.value > 1) {
    return '先在此地观察局势，再与场景人物交谈梳理后续线索。'
  }

  return `不妨多与场景人物交谈，先从「${recommendedLoopFlavor(String(player.value.recommendedLoop ?? '采药炼丹'))}」入手。`
})

const sceneDisplayTitle = computed(() => {
  const regionName = String(scene.value.regionName ?? '').trim()
  const sceneName = String(scene.value.sceneName ?? '').trim()

  if (regionName && sceneName) {
    return `${regionName}-${sceneName}`
  }

  return sceneName || regionName || '修行界-无名之地'
})

function shortenText(value: string, limit = 4) {
  const normalized = value.trim()
  if (normalized.length <= limit) {
    return normalized
  }
  return `${normalized.slice(0, limit)}…`
}

function roomLayerFlavor(value: string) {
  const normalized = value.trim()
  if (!normalized) {
    return ''
  }
  if (normalized === '新手安全圈') {
    return '山门庇护'
  }
  if (normalized === '成长历练圈') {
    return '试手历练'
  }
  if (normalized === '筑基冲刺圈') {
    return '险地冲关'
  }
  return normalized
}

function loopTagFlavor(value: string) {
  const normalized = value.trim()
  if (!normalized) {
    return ''
  }
  const flavorMap: Record<string, string> = {
    门派事务: '门中差遣最盛',
    采药炼丹: '草木药气颇浓',
    护送跑商: '行脚与商旅频仍',
    巡山悬赏: '巡山缉猎不断',
    海猎采珠: '海猎采珠之事正热',
    残区探禁: '探禁寻残之人颇多',
  }
  return flavorMap[normalized] ?? normalized
}

function recommendedLoopFlavor(value: string) {
  const normalized = value.trim()
  if (!normalized) {
    return '听风看路'
  }
  const flavorMap: Record<string, string> = {
    采药炼丹: '采药试丹',
    护送跑商: '护送跑商',
    巡山悬赏: '巡山缉猎',
    门派事务: '门中差遣',
    海猎采珠: '海猎采珠',
    残区探禁: '探禁寻残',
  }
  return flavorMap[normalized] ?? normalized
}

function serviceTagFlavor(value: string) {
  const normalized = value.trim()
  if (!normalized) {
    return ''
  }
  const flavorMap: Record<string, string> = {
    mentor: '有前辈看路',
    rumor: '风声易得',
    board: '设有板面',
    trade: '可做买卖',
    sect: '门中差遣可接',
    gather: '采点在侧',
    travel: '可问路引',
    work: '手边有营生',
    sea: '海路营生正旺',
    danger: '近处多禁险',
  }
  return flavorMap[normalized] ?? normalized
}

function weeklyEventFlavor(event: Record<string, any>) {
  const title = String(event.title ?? '').trim()
  const riskLevel = String(event.riskLevel ?? event.risk_level ?? '').trim()
  const locationHint = String(event.locationHint ?? event.location_hint ?? '').trim()
  const commandHint = String(event.commandHint ?? event.command_hint ?? '').trim()
  const summary = String(event.summary ?? '').trim()
  const head = `${title || '无名周事'}${riskLevel ? `〔${riskLevel}〕` : ''}`
  const tail = locationHint
    ? `见于${locationHint}`
    : commandHint
      ? `可留心 ${commandHint}`
      : summary

  return [head, tail].filter(Boolean).join('，')
}

function weeklyEventDigest(events: Record<string, any>[]) {
  const featured = events
    .map((event) => weeklyEventFlavor(event))
    .filter(Boolean)
    .slice(0, 2)

  if (!featured.length) {
    return ''
  }

  const extraCount = Math.max(0, events.length - featured.length)
  return `本周风波正起：${featured.join('；')}。${extraCount > 0 ? `另有 ${extraCount} 桩周事待看。` : ''}`
}

function sceneLayerNarration(roomLayer: string, loopTags: string[]) {
  const layer = roomLayerFlavor(roomLayer)
  const loopFlavors = loopTags.map((item) => loopTagFlavor(item)).filter(Boolean)
  if (layer && loopFlavors.length > 0) {
    return `${layer}之地，${loopFlavors.join('，')}。`
  }
  if (layer) {
    return `${layer}之地，来往之人尚算有序。`
  }
  if (loopFlavors.length > 0) {
    return loopFlavors.join('，') + '。'
  }
  return ''
}

function boardLayerSuffixFlavor(value: string) {
  const normalized = value.trim()
  if (!normalized) {
    return ''
  }
  const flavorMap: Record<string, string> = {
    山门庇护: '门中庇护',
    试手历练: '可去试手',
    险地冲关: '深处凶险',
  }
  return flavorMap[normalized] ?? normalized
}

function flavorSceneBoardLine(value: string) {
  const normalized = value
    .replace(/新手安全圈/g, roomLayerFlavor('新手安全圈'))
    .replace(/成长历练圈/g, roomLayerFlavor('成长历练圈'))
    .replace(/筑基冲刺圈/g, roomLayerFlavor('筑基冲刺圈'))

  if (normalized.includes('｜')) {
    const segments = normalized.split('｜').map((item) => item.trim())
    if (segments.length >= 3) {
      segments[2] = boardLayerSuffixFlavor(segments[2])
      return segments.join('｜')
    }
  }

  return normalized
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
  {
    key: 'stage',
    label: '阶段',
    value: formatProgressStageLabel(String(player.value.stageLabel ?? '启程')),
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
  {
    key: 'loop',
    label: '推荐',
    value: String(player.value.recommendedLoop ?? '采药炼丹'),
  },
])

const orientationLayout = computed(() => {
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
      command: 'here',
    },
  ]
  const consumedExitKeys = new Set<string>()

  slotDirections.forEach((slotConfig) => {
    const exit = slotConfig.directions
      .map((direction) => exits.value.find((item) => String(item.direction ?? '') === direction))
      .find((item): item is (typeof exits.value)[number] => Boolean(item))
    if (exit) {
      const direction = String(exit.direction ?? '')
      consumedExitKeys.add(`${direction}:${String(exit.targetSceneId ?? '')}`)
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

  return {
    tiles,
    extraExits: exits.value.filter(
      (item) => !consumedExitKeys.has(`${String(item.direction ?? '')}:${String(item.targetSceneId ?? '')}`),
    ),
  }
})

const orientationTiles = computed(() => orientationLayout.value.tiles)

const orientationExtraExits = computed(() => orientationLayout.value.extraExits)

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
  const channel = String(event.channel ?? '').trim().toLowerCase()
  if (['world', 'team', 'local', 'tell'].includes(channel)) {
    return true
  }
  const type = normalizeEventType(String(event.type ?? ''))
  if (type === 'chat') {
    return true
  }
  return /^\[(world|public|team|local|tell)/iu.test(String(event.title ?? ''))
}

function chatChannelLabelFromEvent(event: Record<string, any>) {
  const channel = String(event.channel ?? '').trim().toLowerCase()
  if (channel === 'team') {
    return '队伍'
  }
  if (channel === 'local') {
    return '近聊'
  }
  if (channel === 'tell') {
    return '传音'
  }
  if (channel === 'world') {
    return '世界'
  }
  const title = String(event.title ?? '')
  if (/^\[team\]\s*/iu.test(title)) {
    return '队伍'
  }
  if (/^\[local\]\s*/iu.test(title)) {
    return '近聊'
  }
  if (/^\[tell/iu.test(title)) {
    return '传音'
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
  const suppliedTone = String(event.tone ?? '').trim().toLowerCase()
  if (['system', 'chat', 'quest', 'combat', 'hint'].includes(suppliedTone)) {
    return suppliedTone as DenseLine['tone']
  }
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

function panelLineToneClass(tone: TimelinePanelLine['tone'] = 'normal') {
  return `panel-line--${tone}`
}

function eventChannelLabel(event: Record<string, any>) {
  const channel = String(event.channel ?? '').trim().toLowerCase()
  if (channel === 'quest') {
    return '任务'
  }
  if (channel === 'combat') {
    return '战报'
  }
  if (channel === 'system') {
    return '系统'
  }
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

  const joinMatch = value.match(/^若想拜入(.+)，可(?:使用：)?join\s+.+$/u)
  if (joinMatch) {
    return `若想拜入${joinMatch[1]}，可直接输入 join 对应宗门。`
  }

  const replacements: Array<[RegExp, string]> = [
    [/^先用 look 查看场景人物$/u, '可先用 look 或 here 重看当前场景人物。'],
    [/^先用 look 查看场景妖兽$/u, '可先用 look 或 listen 察看当前场景妖兽。'],
    [/^先用 map 查看当前出口$/u, '可先用 travel 或 map 梳理当前去路。'],
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

function trimMainTimeline() {
  if (mainTimeline.value.length > 400) {
    mainTimeline.value.splice(0, mainTimeline.value.length - 400)
  }
}

function appendMainTimeline(lines: DenseLine[]) {
  const normalizedLines = lines.filter((line) => line.text.trim())
  if (!normalizedLines.length) {
    return
  }

  normalizedLines.forEach((line) => {
    mainTimelineSequence += 1
    mainTimeline.value.push({
      entryType: 'line',
      ...line,
      sequence: mainTimelineSequence,
    })
  })

  trimMainTimeline()
}

function appendMainPanel(panel: Omit<TimelinePanel, 'entryType' | 'sequence'> | null) {
  if (!panel) {
    return
  }

  const hasContent = panel.summary.trim() || panel.lines.some((line) => line.text.trim()) || panel.actions.length > 0
  if (!hasContent) {
    return
  }

  mainTimelineSequence += 1
  mainTimeline.value.push({
    entryType: 'panel',
    sequence: mainTimelineSequence,
    ...panel,
  })
  trimMainTimeline()
}

function resetMessageTimelines() {
  mainTimeline.value = []
  mainTimelineSequence = 0
  processedMainEventIds.clear()
  scrollPanels.topChat.autoFollow = true
  scrollPanels.mainStory.autoFollow = true
  scrollPanels.chatOverlay.autoFollow = true
}

function panelToneForId(panelId: string): DenseLine['tone'] {
  const normalized = panelId.trim().toLowerCase()
  if (['journal', 'tasks', 'board', 'claim', 'family'].includes(normalized)) {
    return 'quest'
  }
  if (['wanted', 'rank'].includes(normalized)) {
    return 'combat'
  }
  if (['listen', 'travel', 'map', 'scene', 'inspect', 'presence', 'exits'].includes(normalized)) {
    return 'hint'
  }
  if (normalized === 'who') {
    return 'chat'
  }
  return 'system'
}

function sceneInteractablePanelTone(kind: SceneInteractableKind): DenseLine['tone'] {
  if (kind === 'monster' || kind === 'hazard') {
    return 'combat'
  }
  if (kind === 'resource' || kind === 'loot') {
    return 'hint'
  }
  if (kind === 'player') {
    return 'chat'
  }
  return 'system'
}

function panelActionLabelFromCommand(command: string, fallback: string) {
  const normalized = command.trim().toLowerCase()
  if (normalized === 'board') {
    return '看委托'
  }
  if (normalized === 'duty') {
    return '看事务'
  }
  if (normalized === 'wanted') {
    return '看悬赏'
  }
  if (normalized === 'travel') {
    return '看路引'
  }
  if (normalized === 'journal') {
    return '翻札记'
  }
  if (normalized === 'bag') {
    return '开行囊'
  }
  if (normalized === 'score') {
    return '内观'
  }
  if (normalized === 'family') {
    return '看门第'
  }
  if (normalized === 'meditate') {
    return '调息'
  }
  if (normalized === 'breakthrough') {
    return '冲关'
  }
  if (normalized.startsWith('fight ')) {
    return `出手·${shortenText(fallback, 4)}`
  }
  if (normalized.startsWith('harvest ')) {
    return `采集·${shortenText(fallback, 4)}`
  }
  if (normalized.startsWith('inspect ')) {
    return `察看·${shortenText(fallback, 4)}`
  }
  if (normalized.startsWith('contribute ')) {
    return `上交·${shortenText(fallback, 4)}`
  }
  if (normalized.startsWith('submit ')) {
    return `交付·${shortenText(fallback, 4)}`
  }
  if (normalized.startsWith('talk ')) {
    return `交谈·${shortenText(fallback, 4)}`
  }
  if (normalized.startsWith('go ')) {
    return `前往·${shortenText(fallback, 4)}`
  }
  if (normalized.startsWith('join ')) {
    return `拜入·${shortenText(fallback, 4)}`
  }
  if (normalized.startsWith('claim')) {
    return `领取·${shortenText(fallback, 4)}`
  }
  if (normalized.startsWith('use ')) {
    return `使用·${shortenText(fallback, 4)}`
  }
  return shortenText(fallback, 6) || '执行'
}

function buildPanelActionsFromEntries(panelId: string, entries: Record<string, any>[] = []) {
  const actions: CommandAction[] = []
  entries.forEach((entry) => {
    if (actions.length >= 4) {
      return
    }
    const command = String(entry.command ?? '').trim()
    if (!command) {
      return
    }
    const title = String(entry.title ?? '执行')
    actions.push({
      key: `panel-action-${panelId}-${String(entry.entryId ?? title)}`,
      label: panelActionLabelFromCommand(command, title),
      detail: String(entry.summary ?? entry.status ?? '继续这一步行动。'),
      command,
      execute: !command.endsWith(' '),
      composer: 'command',
    })
  })
  return actions
}

function panelRenderProfile(panelId: string) {
  const normalized = panelId.trim().toLowerCase()
  switch (normalized) {
    case 'board':
      return { mark: '委', compact: true, entryLimit: 4, summaryLimit: 1 }
    case 'duty':
      return { mark: '务', compact: true, entryLimit: 3, summaryLimit: 1 }
    case 'wanted':
      return { mark: '悬', compact: true, entryLimit: 4, summaryLimit: 1 }
    case 'travel':
      return { mark: '途', compact: true, entryLimit: 4, summaryLimit: 1 }
    case 'claim':
      return { mark: '赏', compact: true, entryLimit: 3, summaryLimit: 1 }
    case 'score':
    case 'hp':
      return { mark: '我', compact: true, entryLimit: 4, summaryLimit: 1 }
    case 'family':
      return { mark: '门', compact: true, entryLimit: 4, summaryLimit: 1 }
    case 'journal':
    case 'tasks':
      return { mark: '札', compact: true, entryLimit: 4, summaryLimit: 1 }
    case 'bag':
      return { mark: '囊', compact: true, entryLimit: 4, summaryLimit: 1 }
    case 'listen':
      return { mark: '闻', compact: true, entryLimit: 4, summaryLimit: 1 }
    case 'who':
      return { mark: '众', compact: true, entryLimit: 4, summaryLimit: 1 }
    case 'presence':
      return { mark: '见', compact: true, entryLimit: 6, summaryLimit: 1 }
    case 'exits':
      return { mark: '途', compact: true, entryLimit: 6, summaryLimit: 1 }
    case 'rank':
      return { mark: '榜', compact: true, entryLimit: 5, summaryLimit: 1 }
    case 'work':
      return { mark: '工', compact: true, entryLimit: 4, summaryLimit: 1 }
    case 'help':
      return { mark: '助', compact: false, entryLimit: 8, summaryLimit: 2 }
    case 'commands':
      return { mark: '令', compact: false, entryLimit: 10, summaryLimit: 2 }
    case 'read':
      return { mark: '帖', compact: false, entryLimit: 8, summaryLimit: 2 }
    case 'map':
      return { mark: '图', compact: true, entryLimit: 6, summaryLimit: 1 }
    default:
      return { mark: '札', compact: false, entryLimit: 6, summaryLimit: 2 }
  }
}

function panelMetaParts(panelId: string, entry: Record<string, any>) {
  const normalized = panelId.trim().toLowerCase()
  const category = String(entry.category ?? '').trim()
  const locationHint = String(entry.locationHint ?? '').trim()
  const rewardSummary = String(entry.rewardSummary ?? '').trim()

  if (normalized === 'travel') {
    return [locationHint, rewardSummary].filter(Boolean)
  }
  if (normalized === 'wanted') {
    return [locationHint, rewardSummary ? `掉落：${rewardSummary}` : ''].filter(Boolean)
  }
  if (normalized === 'claim') {
    return [rewardSummary ? `所得：${rewardSummary}` : '', locationHint].filter(Boolean)
  }
  if (['board', 'duty', 'journal', 'tasks'].includes(normalized)) {
    return [locationHint, rewardSummary ? `所得：${rewardSummary}` : ''].filter(Boolean)
  }
  if (normalized === 'who') {
    return [locationHint, category].filter(Boolean)
  }
  if (normalized === 'listen') {
    return [locationHint, category].filter(Boolean)
  }
  if (normalized === 'week') {
    return [locationHint, category, rewardSummary ? `可循：${rewardSummary}` : ''].filter(Boolean)
  }
  if (normalized === 'bag') {
    return [category, rewardSummary].filter(Boolean)
  }
  return [category, locationHint, rewardSummary ? `所得：${rewardSummary}` : ''].filter(Boolean)
}

function scenePanelMark(kind: SceneInteractableKind) {
  const marks: Record<SceneInteractableKind, string> = {
    player: '众',
    npc: '人',
    shop: '物',
    monster: '战',
    resource: '采',
    loot: '遗',
    hazard: '禁',
  }
  return marks[kind] ?? '札'
}

function buildTimelinePanelFromStructuredPanel(panel: Record<string, any>, index: number): Omit<TimelinePanel, 'entryType' | 'sequence'> | null {
  const panelId = String(panel.panelId ?? panel.panel_id ?? `panel-${index}`).trim() || `panel-${index}`
  const panelDocumentId = String(panel.documentId ?? panel.document_id ?? '').trim()
  const panelKind = String(panel.panelKind ?? panel.panel_kind ?? '').trim()
  const title =
    panelKind === 'help_topic'
      ? formatHelpTopicTitle(panelDocumentId, String(panel.title ?? panelId).trim() || '无名札板')
      : String(panel.title ?? panelId).trim() || '无名札板'
  const compactTitle = String(panel.compactTitle ?? panel.compact_title ?? title).trim() || title
  const summary = String(panel.summary ?? '').trim()
  const profile = panelRenderProfile(panelId)
  const sourceEntries = (panel.entries as Record<string, any>[] | undefined) ?? []
  const asciiLines = ((panel.asciiLines as string[] | undefined) ?? (panel.ascii_lines as string[] | undefined) ?? [])
    .map((line) => String(line ?? '').trim())
    .filter(Boolean)
  const bodyLines = ((panel.bodyLines as string[] | undefined) ?? (panel.body_lines as string[] | undefined) ?? [])
    .map((line) => String(line ?? '').trim())
    .filter(Boolean)
  const entries = sourceEntries.slice(0, profile.entryLimit)

  const lines: TimelinePanelLine[] = []
  if (asciiLines.length > 0) {
    asciiLines.slice(0, Math.max(profile.entryLimit + 2, 6)).forEach((text, entryIndex) => {
      lines.push({
        key: `${panelId}-ascii-${entryIndex}`,
        text,
        tone: entryIndex === 0 ? 'accent' : 'normal',
      })
    })
  } else if (bodyLines.length > 0) {
    bodyLines.slice(0, Math.max(profile.entryLimit + 2, 8)).forEach((text, entryIndex) => {
      lines.push({
        key: `${panelId}-body-${entryIndex}`,
        text,
        tone:
          panelKind === 'help_topic' || panelKind === 'command_manual'
            ? entryIndex === 0
              ? 'accent'
              : 'normal'
            : entryIndex === 0
              ? 'accent'
              : 'normal',
      })
    })
  } else {
    entries.forEach((entry, entryIndex) => {
      const entryTitle = String(entry.title ?? '条目').trim()
      const entryStatus = String(entry.status ?? '').trim()
      const entrySummary = String(entry.summary ?? '').trim()
      lines.push({
        key: `${panelId}-title-${entryIndex}`,
        text: `• ${entryTitle}${entryStatus ? `〔${entryStatus}〕` : ''}`,
        tone: entryIndex === 0 ? 'accent' : 'normal',
      })
      if (entrySummary) {
        splitDenseText(entrySummary, profile.summaryLimit).forEach((text, detailIndex) => {
          lines.push({
            key: `${panelId}-summary-${entryIndex}-${detailIndex}`,
            text: `  ${text}`,
            tone: 'normal',
          })
        })
      }
      const meta = panelMetaParts(panelId, entry)
      if (meta.length > 0) {
        lines.push({
          key: `${panelId}-meta-${entryIndex}`,
          text: `  ${meta.join(' · ')}`,
          tone: 'muted',
        })
      }
    })
  }
  if (asciiLines.length === 0 && sourceEntries.length > entries.length) {
    lines.push({
      key: `${panelId}-more`,
      text: `……尚有 ${sourceEntries.length - entries.length} 条，可再行此札细看。`,
      tone: 'muted',
    })
  }
  if (bodyLines.length > Math.max(profile.entryLimit + 2, 8)) {
    lines.push({
      key: `${panelId}-body-more`,
      text: `……尚有 ${bodyLines.length - Math.max(profile.entryLimit + 2, 8)} 段，可再翻后文。`,
      tone: 'muted',
    })
  }

  const inlineCommands = ((panel.inlineCommands as string[] | undefined) ?? (panel.inline_commands as string[] | undefined) ?? [])
    .map((command) => String(command ?? '').trim())
    .filter(Boolean)
  const actionSeedEntries =
    inlineCommands.length > 0
      ? inlineCommands.map((command, actionIndex) => ({
          entryId: `${panelId}-inline-${actionIndex}`,
          title: command,
          summary: '可继续顺着这一札往下行。',
          command,
        }))
      : entries

  return {
    key: `timeline-panel-${panelId}-${mainTimelineSequence + index + 1}`,
    panelId,
    mark:
      panelKind === 'help_topic'
        ? '助'
        : panelKind === 'command_manual'
          ? '令'
          : panelKind === 'board_post'
            ? '帖'
            : panelKind === 'job_board'
              ? '工'
              : panelKind === 'leaderboard'
                ? '榜'
                : profile.mark,
    title,
    compactTitle,
    summary,
    tone: panelToneForId(panelId),
    compact: profile.compact,
    renderMode: String(panel.renderMode ?? panel.render_mode ?? (panelId === 'map' ? 'ascii_map' : 'board_block')) as PanelRenderMode,
    styleId: String(panel.styleId ?? panel.style_id ?? 'mud-tablet'),
    lines,
    actions: buildPanelActionsFromEntries(panelId, actionSeedEntries),
  }
}

function buildSceneInteractablePanel(item: SceneInteractable): Omit<TimelinePanel, 'entryType' | 'sequence'> {
  const lines: TimelinePanelLine[] = []
  splitDenseText(item.description, 3).forEach((text, index) => {
    lines.push({
      key: `${item.key}-desc-${index}`,
      text,
      tone: index === 0 ? 'accent' : 'normal',
    })
  })
  item.meta.slice(0, 4).forEach((meta, index) => {
    lines.push({
      key: `${item.key}-meta-${index}`,
      text: `· ${meta}`,
      tone: 'muted',
    })
  })

  return {
    key: `scene-panel-${item.key}-${mainTimelineSequence + 1}`,
    panelId: `scene:${item.kind}`,
    mark: scenePanelMark(item.kind),
    title: item.title,
    compactTitle: item.title,
    summary: item.subtitle,
    tone: sceneInteractablePanelTone(item.kind),
    compact: true,
    renderMode: 'notice_block',
    styleId: item.kind === 'monster' ? 'beast-slip' : 'scene-slip',
    lines,
    actions: item.actions.slice(0, 3),
  }
}

function rankingLabel(kind: RankingType) {
  return rankingOptions.find((option) => option.id === kind)?.label ?? '名榜'
}

function buildRankingPanel(kind: RankingType): Omit<TimelinePanel, 'entryType' | 'sequence'> {
  const lines: TimelinePanelLine[] = store.rankings.slice(0, 5).map((entry, index) => {
    const name = String(entry.characterName ?? entry.account ?? '无名修士')
    const title = String(entry.title ?? entry.realmName ?? entry.sectName ?? '散修')
    const extra = String(entry.extra ?? '').trim()
    return {
      key: `rank-${kind}-${index}`,
      text: `${name} · 第 ${String(entry.rank ?? index + 1)} 名 · ${title}${extra ? ` · ${extra}` : ''} · 分数 ${String(entry.score ?? 0)}`,
      tone: index === 0 ? 'accent' : 'normal',
    }
  })

  return {
    key: `rank-panel-${kind}-${mainTimelineSequence + 1}`,
    panelId: 'rank',
    mark: '榜',
    title: rankingLabel(kind),
    compactTitle: '名榜',
    summary: store.rankings.length > 0 ? `当前可见 ${store.rankings.length} 名修士。` : '此刻还没有名榜记录。',
    tone: 'combat',
    compact: true,
    renderMode: 'roster_block',
    styleId: 'jianghu-board',
    lines,
    actions: [],
  }
}

function buildMapPanel(): Omit<TimelinePanel, 'entryType' | 'sequence'> {
  const lines: TimelinePanelLine[] = [
    {
      key: `map-current-${currentSceneId.value}`,
      text: `此身所在：${sceneDisplayTitle.value}`,
      tone: 'accent',
    },
  ]

  const slots: Array<{ label: string; directions: string[] }> = [
    { label: '北', directions: ['north', 'up'] },
    { label: '西', directions: ['west'] },
    { label: '东', directions: ['east'] },
    { label: '南', directions: ['south', 'down'] },
  ]

  slots.forEach((slot, index) => {
    const exit = slot.directions
      .map((direction) => exits.value.find((item) => String(item.direction ?? '') === direction))
      .find((item): item is (typeof exits.value)[number] => Boolean(item))
    lines.push({
      key: `map-slot-${index}`,
      text: `${slot.label}：${exit ? String(exit.targetSceneName ?? exit.targetSceneId ?? '未知去路') : '前路未显'}`,
      tone: exit ? 'normal' : 'muted',
    })
  })

  if (orientationExtraExits.value.length > 0) {
    lines.push({
      key: `map-extra-${currentSceneId.value}`,
      text: `余路：${orientationExtraExits.value
        .map((exit) => `${directionLabel(String(exit.direction))}往 ${String(exit.targetSceneName ?? exit.targetSceneId ?? '未知之地')}`)
        .join('；')}`,
      tone: 'muted',
    })
  }

  lines.push({
    key: `map-tip-${currentSceneId.value}`,
    text: '舆图只记近路，若想看更远航路与路引，可再行一札 travel。',
    tone: 'muted',
  })

  const actions = exits.value.slice(0, 4).map((exit) => ({
    key: `map-go-${String(exit.direction)}`,
    label: directionLabel(String(exit.direction)).replace('方', ''),
    detail: `前往${String(exit.targetSceneName ?? exit.targetSceneId ?? '未知之地')}。`,
    command: `go ${String(exit.direction)}`,
  }))

  if (actions.length < 4) {
    actions.push({
      key: 'map-travel',
      label: '路引',
      detail: '查看更远路线和下一站建议。',
      command: 'travel',
    })
  }

  return {
    key: `map-panel-${currentSceneId.value}-${mainTimelineSequence + 1}`,
    panelId: 'map',
    mark: '图',
    title: '近身舆图',
    compactTitle: '舆图',
    summary: `${String(scene.value.regionName ?? '此地')}一带的去路都记在这里。`,
    tone: 'hint',
    compact: true,
    renderMode: 'ascii_map',
    styleId: 'scroll-map',
    lines,
    actions,
  }
}

function buildSceneSnapshotLines() {
  const lines: DenseLine[] = []
  const sceneDescription = String(scene.value.sceneBrief ?? scene.value.description ?? '').trim()
  const sceneAftertaste = String(scene.value.sceneAftertaste ?? '').trim()
  const roomLayer = String(scene.value.roomLayer ?? '').trim()
  const loopTags = ((scene.value.loopTags as string[] | undefined) ?? []).map((item) => String(item).trim()).filter(Boolean)
  const serviceTags = ((scene.value.serviceTags as string[] | undefined) ?? [])
    .map((item) => serviceTagFlavor(String(item ?? '')))
    .filter(Boolean)
  const rumorTopics = ((scene.value.rumorTopics as string[] | undefined) ?? [])
    .map((item) => String(item ?? '').trim())
    .filter(Boolean)
  const mentorIds = ((scene.value.mentorIds as string[] | undefined) ?? [])
    .map((item) => String(item ?? '').trim())
    .filter(Boolean)
  const boardAvailable = Boolean(scene.value.boardAvailable)
  const identityTrack = String(player.value.identityTrack ?? '').trim()
  const rankLevel = Number(player.value.rankLevel ?? 0)
  const contributionState = String(player.value.contributionState ?? '').trim()
  const reputationState = String(player.value.reputationState ?? '').trim()
  const unreadBoardCount = Number(player.value.unreadBoardCount ?? 0)
  const weeklyEvents = ((store.weeklyEvents as Record<string, any>[] | undefined) ?? []).filter(
    (event) => String(event.title ?? event.summary ?? '').trim(),
  )

  if (roomLayer || loopTags.length > 0) {
    const layerNarration = sceneLayerNarration(roomLayer, loopTags)
    lines.push({
      key: `scene-layer-${currentSceneId.value}-${mainTimelineSequence + 1}`,
      tag: '局势',
      text: layerNarration || [roomLayer, loopTags.join('、')].filter(Boolean).join(' · '),
      tone: 'hint',
    })
  }

  const serviceParts = [
    boardAvailable ? '此地设有板面' : '',
    rumorTopics.length > 0 ? `风声多绕「${rumorTopics.slice(0, 2).join('、')}」` : '',
    mentorIds.length > 0 || serviceTags.some((item) => item.includes('前辈')) ? '有前辈可指路' : '',
    ...serviceTags.slice(0, 2),
  ].filter(Boolean)
  if (serviceParts.length > 0) {
    lines.push({
      key: `scene-services-${currentSceneId.value}-${mainTimelineSequence + 1}`,
      tag: '门径',
      text: serviceParts.join('，') + '。',
      tone: 'hint',
    })
  }

  const weeklyEventText = weeklyEventDigest(weeklyEvents)
  if (weeklyEventText) {
    lines.push({
      key: `scene-weekly-${currentSceneId.value}-${mainTimelineSequence + 1}`,
      tag: '周讯',
      text: weeklyEventText,
      tone: 'hint',
    })
  }

  const identityParts = [
    identityTrack ? `${identityTrack}${rankLevel > 0 ? `·${rankLevel}阶` : ''}` : '',
    reputationState,
    contributionState,
    unreadBoardCount > 0 ? `尚有 ${unreadBoardCount} 张板帖未曾细看` : '',
  ].filter(Boolean)
  if (identityParts.length > 0) {
    lines.push({
      key: `scene-identity-${currentSceneId.value}-${mainTimelineSequence + 1}`,
      tag: '身分',
      text: identityParts.join('；') + '。',
      tone: 'hint',
    })
  }

  splitDenseText(sceneDescription, 3).forEach((text, index) => {
    lines.push({
      key: `scene-description-${currentSceneId.value}-${mainTimelineSequence + index + 1}`,
      tag: index === 0 ? '眼前' : '景语',
      text,
      tone: 'system',
    })
  })

  if (sceneMissionText.value) {
    lines.push({
      key: `scene-mission-${currentSceneId.value}-${mainTimelineSequence + 1}`,
      tag: '线索',
      text: sceneMissionText.value,
      tone: 'quest',
    })
  }
  if (sceneAftertaste) {
    lines.push({
      key: `scene-aftertaste-${currentSceneId.value}-${mainTimelineSequence + 1}`,
      tag: '余韵',
      text: sceneAftertaste,
      tone: 'hint',
    })
  }

  return lines
}

function buildSceneSnapshotPanels() {
  const panels: Array<Omit<TimelinePanel, 'entryType' | 'sequence'>> = []

  const presenceBoard = ((scene.value.presenceBoard as string[] | undefined) ?? [])
    .map((line) => flavorSceneBoardLine(String(line ?? '').trim()))
    .filter(Boolean)
  if (presenceBoard.length > 0) {
    panels.push({
      key: `scene-presence-${currentSceneId.value}-${mainTimelineSequence + 1}`,
      panelId: 'presence',
      mark: '见',
      title: '眼前所见',
      compactTitle: '眼前',
      summary: '可见人影、物件与异状，都在此札。',
      tone: 'system',
      compact: true,
      renderMode: 'roster_block',
      styleId: 'scene-presence',
      lines: presenceBoard.slice(0, 8).map((text, index) => ({
        key: `scene-presence-line-${index}`,
        text,
        tone: index === 0 ? 'accent' : 'normal',
      })),
      actions: sceneInteractables.value.slice(0, 6).flatMap((item) => item.actions.slice(0, 1)).slice(0, 3),
    })
  }

  const exitBoard = ((scene.value.exitBoard as string[] | undefined) ?? [])
    .map((line) => flavorSceneBoardLine(String(line ?? '').trim()))
    .filter(Boolean)
  if (exitBoard.length > 0) {
    panels.push({
      key: `scene-exits-${currentSceneId.value}-${mainTimelineSequence + 1}`,
      panelId: 'exits',
      mark: '途',
      title: '可行去路',
      compactTitle: '去路',
      summary: '前后去处与路上缓急，都在这边。',
      tone: 'hint',
      compact: true,
      renderMode: 'ascii_map',
      styleId: 'exit-slip',
      lines: exitBoard.slice(0, 6).map((text, index) => ({
        key: `scene-exit-line-${index}`,
        text,
        tone: index === 0 ? 'accent' : 'normal',
      })),
      actions: exits.value.slice(0, 3).map((exit) => ({
        key: `scene-exit-go-${String(exit.direction)}`,
        label: `去${directionLabel(String(exit.direction)).replace('方', '')}`,
        detail: `前往${String(exit.targetSceneName ?? exit.targetSceneId ?? '未知之地')}。`,
        command: `go ${String(exit.direction)}`,
      })),
    })
  }

  const suggestedActionLines: TimelinePanelLine[] = []
  const suggestedActions: CommandAction[] = []
  const seenCommands = new Set<string>()

  const sceneActionDisplayText = (action: CommandAction, targetTitle = '') => {
    const commandText = String(action.command ?? '').trim()
    const normalized = commandText.toLowerCase()
    if (normalized.startsWith('talk ') && targetTitle) {
      return `talk ${targetTitle}`
    }
    if (normalized.startsWith('ask ') && targetTitle) {
      return `ask ${targetTitle}`
    }
    if (normalized.startsWith('inspect ') && targetTitle) {
      return `inspect ${targetTitle}`
    }
    if (normalized.startsWith('fight ') && targetTitle) {
      return `fight ${targetTitle}`
    }
    if (normalized.startsWith('harvest ') && targetTitle) {
      return `harvest ${targetTitle}`
    }
    if (normalized.startsWith('loot ') && targetTitle) {
      return `loot ${targetTitle}`
    }
    if (normalized.startsWith('buy ') && targetTitle) {
      return `buy ${targetTitle}`
    }
    return commandText
  }

  const appendSuggestedAction = (action: CommandAction | undefined, text?: string) => {
    if (!action) {
      return
    }
    const commandText = String(action.command ?? '').trim()
    if (!commandText) {
      return
    }
    const signature = `${commandText}:${action.execute === false ? 'prefill' : 'run'}`
    if (seenCommands.has(signature)) {
      return
    }
    seenCommands.add(signature)
    suggestedActionLines.push({
      key: `scene-action-line-${suggestedActionLines.length + 1}`,
      text: text ?? commandText,
      tone: suggestedActionLines.length === 0 ? 'accent' : 'normal',
    })
    if (suggestedActions.length < 4) {
      suggestedActions.push(action)
    }
  }

  sceneInteractables.value.slice(0, 4).forEach((item) => {
    const preferredAction =
      item.actions.find((action) => String(action.command ?? '').startsWith('talk ')) ??
      item.actions.find((action) => String(action.command ?? '').startsWith('ask ')) ??
      item.actions.find((action) => String(action.command ?? '').startsWith('inspect ')) ??
      item.actions.find((action) => String(action.command ?? '').startsWith('fight ')) ??
      item.actions.find((action) => String(action.command ?? '').startsWith('harvest ')) ??
      item.actions.find((action) => String(action.command ?? '').startsWith('loot ')) ??
      item.actions.find((action) => String(action.command ?? '').startsWith('buy ')) ??
      item.actions[0]

    appendSuggestedAction(preferredAction, preferredAction ? sceneActionDisplayText(preferredAction, item.title) : '')
  })

  exits.value.slice(0, 3).forEach((exit) => {
    const direction = String(exit.direction ?? '').trim()
    if (!direction) {
      return
    }
    appendSuggestedAction(
      {
        key: `scene-action-exit-${direction}`,
        label: `去${directionLabel(direction).replace('方', '')}`,
        detail: `前往${String(exit.targetSceneName ?? exit.targetSceneId ?? '未知之地')}。`,
        command: `go ${direction}`,
      },
      `go ${direction}`,
    )
  })

  appendSuggestedAction({
    key: 'scene-action-listen',
    label: '听风声',
    detail: '收一收此地风声。',
    command: 'listen',
  })

  if (suggestedActionLines.length > 0) {
    panels.push({
      key: `scene-actions-${currentSceneId.value}-${mainTimelineSequence + 1}`,
      panelId: 'scene_actions',
      mark: '行',
      title: '此刻可做',
      compactTitle: '可做',
      summary: '照着这些短字落令，便能顺势前行。',
      tone: 'hint',
      compact: true,
      renderMode: 'notice_block',
      styleId: 'scene-command',
      lines: suggestedActionLines.slice(0, 6),
      actions: suggestedActions.slice(0, 3),
    })
  }

  return panels
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

function buildResultTimelinePanels(result: Record<string, any>) {
  return ((result.panels as Record<string, any>[] | undefined) ?? [])
    .slice(0, 3)
    .map((panel, index) => buildTimelinePanelFromStructuredPanel(panel, index))
    .filter((panel): panel is Omit<TimelinePanel, 'entryType' | 'sequence'> => Boolean(panel))
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

function openSettingsOverlay() {
  activeOverlay.value = 'settings'
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
  activeOverlay.value = 'none'
  appendMainPanel(buildSceneInteractablePanel(item))
  void maybeFollowPanel('mainStory')
}

function commandCategoryClass(id: CommandCategoryId) {
  return `command-tone-${id}`
}

function sceneInteractableClass(item: SceneInteractable) {
  return [
    `rail-button--${item.kind}`,
    {
      active: selectedSceneInteractable.value?.key === item.key,
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
  if (overlay !== 'commands' && overlay !== 'scene' && overlay !== 'messages' && overlay !== 'settings') {
    activeTab.value = overlay
  }
  activeOverlay.value = overlay
}

function closeOverlay() {
  activeOverlay.value = 'none'
}

async function openRankingPanel(kind: RankingType = store.rankingType) {
  try {
    await store.loadRankings(kind)
    appendMainPanel(buildRankingPanel(kind))
    await maybeFollowPanel('mainStory')
  } catch (error) {
    setError(error)
  }
}

function triggerDockCommand(entry: DockCommandEntry) {
  if (entry.action) {
    applyAction(entry.action)
    return
  }

  if (entry.command) {
    void submitComposer(entry.command)
    return
  }

  if (entry.panel === 'map') {
    appendMainPanel(buildMapPanel())
    void maybeFollowPanel('mainStory')
    return
  }

  if (entry.rankingType) {
    void openRankingPanel(entry.rankingType)
  }
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
    if (!selectedBackgroundId.value) {
      throw new Error('请先选择一个凡俗背景。')
    }

    await store.createCharacter(characterName.value, selectedOriginId.value, selectedBackgroundId.value)
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

async function loadRanking(kind: RankingType) {
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
  () => availableBackgrounds.value.map((background) => String(background.backgroundId ?? '')).join('|'),
  () => {
    if (!availableBackgrounds.value.length) {
      selectedBackgroundId.value = ''
      return
    }

    if (availableBackgrounds.value.some((background) => String(background.backgroundId ?? '') === selectedBackgroundId.value)) {
      return
    }

    selectedBackgroundId.value = String(availableBackgrounds.value[0]?.backgroundId ?? '')
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
    buildSceneSnapshotPanels().forEach((panel) => appendMainPanel(panel))
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
    buildResultTimelinePanels(result).forEach((panel) => appendMainPanel(panel))
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
  <div class="shell" :class="[{ 'shell--game': showGameView }, scenePaletteClass]">
    <header v-if="showGameView" class="top-banner">
      <div class="prompt-strip">
        <p class="prompt-line">{{ statusPromptPrimary }}</p>
        <p class="prompt-subline">{{ statusPromptSecondary }}</p>
      </div>
    </header>

    <header v-else class="hero">
      <div class="brand-copy">
        <p class="eyebrow">暖墨纸灯 · 纯文字修仙</p>
        <h1>凡人修仙 MUD</h1>
        <p class="subtitle">此卷不演大戏，只记你在修行界里亲眼所见、亲耳所闻与亲手走过的路。</p>
      </div>
    </header>

    <p v-if="displayError" class="error-banner">{{ displayError }}</p>

    <section v-if="!store.authenticated" class="auth-card">
      <div class="card-heading">
        <h2>启卷入世</h2>
        <p>写下道号与口令，灯下翻卷，便可再次踏入这片以文字为形的修行界。新注册账号仅支持英文字母和数字。</p>
      </div>
      <div class="form-grid">
        <label>
          <span>道号</span>
          <input v-model="account" autocomplete="username" placeholder="例如 hanli001" />
        </label>
        <label>
          <span>口令</span>
          <input v-model="password" type="password" autocomplete="current-password" placeholder="写入口令" />
        </label>
      </div>
      <div class="action-row">
        <button type="button" class="primary-button" :disabled="store.loading" @click="login(false)">续上旧卷</button>
        <button type="button" class="secondary-button" :disabled="store.loading" @click="login(true)">新开一卷</button>
      </div>
    </section>

    <section v-else-if="showCreateCharacterView" class="auth-card">
      <div class="card-heading">
        <h2>立下名帖</h2>
        <p>当前道号：{{ store.account }}。先立名，再定来处与凡俗旧业。若这卷中本就有人，也可先试着把旧身影唤回来。</p>
      </div>
      <div class="form-grid single-column">
        <label>
          <span>名帖</span>
          <input v-model="characterName" maxlength="24" placeholder="例如 韩立" @keyup.enter="createCharacter()" />
        </label>
        <div>
          <span>来处</span>
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
        <div>
          <span>旧业</span>
          <div class="origin-grid">
            <button
              v-for="background in availableBackgrounds"
              :key="background.backgroundId"
              type="button"
              class="origin-card"
              :class="{ active: selectedBackgroundId === String(background.backgroundId) }"
              @click="selectedBackgroundId = String(background.backgroundId)"
            >
              <strong>{{ background.name }}</strong>
              <small>{{ background.focusLabel }}</small>
              <p>{{ background.description }}</p>
            </button>
          </div>
          <article v-if="selectedBackground" class="detail-card origin-preview-card">
            <p class="detail-title">{{ selectedBackground.name }} · {{ selectedBackground.starterTitle }}</p>
            <p>{{ selectedBackground.description }}</p>
            <p>发展重点：{{ selectedBackground.focusLabel }}</p>
          </article>
        </div>
      </div>
      <div class="action-row">
        <button type="button" class="primary-button" :disabled="store.loading || !selectedOriginId || !selectedBackgroundId" @click="createCharacter()">
          落笔成名
        </button>
        <button type="button" class="secondary-button" :disabled="store.loading" @click="retryBootstrap()">唤回旧身</button>
        <button type="button" class="ghost-button" :disabled="store.loading" @click="store.logout()">回到卷首</button>
      </div>
    </section>

    <main v-else-if="showGameView" class="mobile-layout mobile-layout--terminal">
      <section class="surface-panel scene-panel">
        <div class="scene-main-board scene-main-board--single">
          <div class="scene-board-header scene-board-header--terminal">
            <div class="scene-board-copy">
              <p class="scene-board-kicker">{{ scene.ambientMood || '灯影微温' }}</p>
              <p class="scene-board-title">{{ sceneDisplayTitle }}</p>
            </div>
          </div>

          <div class="story-console story-console--single">
            <div ref="storyViewport" class="story-log" @scroll="handleViewportScroll('mainStory')">
              <template v-for="entry in mainTimeline" :key="entry.key">
                <article
                  v-if="entry.entryType === 'line'"
                  class="story-line"
                  :class="denseToneClass(entry.tone)"
                >
                  <span class="story-tag">{{ entry.tag }}</span>
                  <p class="story-text">{{ entry.text }}</p>
                </article>

                <article
                  v-else
                  class="story-panel"
                  :class="[
                    denseToneClass(entry.tone),
                    { 'story-panel--compact': entry.compact },
                    `story-panel--${entry.renderMode}`,
                    `story-panel-style--${entry.styleId}`,
                  ]"
                >
                  <header class="story-panel-header">
                    <div class="story-panel-copy">
                      <p class="story-panel-title">{{ entry.compact ? entry.compactTitle : entry.title }}</p>
                      <p v-if="entry.summary" class="story-panel-summary">{{ entry.summary }}</p>
                    </div>
                    <span class="story-panel-mark">{{ entry.mark }}</span>
                  </header>
                  <div class="story-panel-body">
                    <p
                      v-for="line in entry.lines"
                      :key="line.key"
                      class="story-panel-line"
                      :class="panelLineToneClass(line.tone)"
                    >
                      {{ line.text }}
                    </p>
                  </div>
                  <div v-if="entry.actions.length > 0" class="story-panel-actions">
                    <button
                      v-for="action in entry.actions"
                      :key="action.key"
                      type="button"
                      class="story-panel-action"
                      @click="applyAction(action)"
                    >
                      {{ action.label }}
                    </button>
                  </div>
                </article>
              </template>
              <p v-if="mainTimeline.length === 0" class="empty-text">当前还没有新的场景记录，先试着观察、交谈或移动吧。</p>
            </div>
          </div>

          <div class="scene-orientation-card scene-orientation-card--restored">
            <div class="scene-orientation-frame scene-orientation-frame--restored">
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
                <span class="orientation-caption">{{ tile.caption }}</span>
              </button>
            </div>

            <div v-if="orientationExtraExits.length > 0" class="orientation-extra-row orientation-extra-row--restored">
              <button
                v-for="exit in orientationExtraExits"
                :key="`orientation-extra-${String(exit.direction)}-${String(exit.targetSceneId ?? exit.targetSceneName ?? '')}`"
                type="button"
                class="orientation-extra-button"
                @click="submitComposer(`go ${String(exit.direction)}`)"
              >
                <span>{{ directionLabel(String(exit.direction)) }}方</span>
                <strong>{{ String(exit.targetSceneName ?? exit.targetSceneId ?? '未知去路') }}</strong>
              </button>
            </div>
          </div>
        </div>
      </section>

      <footer class="bottom-dock bottom-dock--terminal">
        <div class="dock-nav-grid">
          <button
            v-for="entry in dockCommandEntries"
            :key="entry.key"
            type="button"
            class="dock-nav-button"
            @click="triggerDockCommand(entry)"
          >
            <span>{{ entry.label }}</span>
            <small>{{ entry.caption }}</small>
          </button>
        </div>

        <div class="mode-row mode-row--terminal">
          <button
            type="button"
            class="mode-button"
            :class="{ active: composerMode === 'chat' && chatChannel === 'world' }"
            @click="applyAction({ key: 'chat-world', label: '', detail: '', composer: 'chat', chatChannel: 'world' })"
          >
            世声
          </button>
          <button
            type="button"
            class="mode-button"
            :class="{ active: composerMode === 'chat' && chatChannel === 'team' }"
            @click="applyAction({ key: 'chat-team', label: '', detail: '', composer: 'chat', chatChannel: 'team' })"
          >
            队声
          </button>
          <button
            type="button"
            class="mode-button"
            :class="{ active: composerMode === 'command' }"
            @click="composerMode = 'command'"
          >
            落令
          </button>
          <button
            type="button"
            class="mode-button mode-button--subtle"
            @click="void openCodexOverlay()"
          >
            长卷
          </button>
          <button
            type="button"
            class="mode-button mode-button--subtle"
            @click="openSettingsOverlay()"
          >
            卷末
          </button>
        </div>

        <div class="composer-caption">{{ composerTitle }}</div>
        <form class="command-form" @submit.prevent="submitComposer()">
          <input v-model="composerText" :placeholder="composerPlaceholder" :disabled="store.loading" />
          <button class="primary-button" :disabled="store.loading">
            {{ composerMode === 'chat' ? '传声' : '落令' }}
          </button>
        </form>
      </footer>
    </main>

    <section v-else-if="store.authenticated" class="auth-card">
      <div class="card-heading">
        <h2>寻回旧影</h2>
        <p>当前道号：{{ store.account }}。登录态仍在，但卷中旧身尚未完整显形；可再试一次唤回，或先回到卷首重整气机。</p>
      </div>
      <div class="action-row">
        <button type="button" class="primary-button" :disabled="store.loading" @click="retryBootstrap()">再唤一次</button>
        <button type="button" class="secondary-button" :disabled="store.loading" @click="store.logout()">回到卷首</button>
      </div>
    </section>

    <div v-if="store.authenticated && activeOverlay !== 'none'" class="overlay-backdrop" @click.self="closeOverlay()">
      <section class="overlay-sheet" :class="{ 'overlay-sheet--wide': activeOverlay === 'codex' }">
        <div class="overlay-grabber" aria-hidden="true"></div>
        <header class="overlay-header">
          <div>
            <p class="section-kicker">{{ activeOverlay === 'codex' ? '长卷' : '卷末' }}</p>
            <h2>{{ activeOverlayTitle }}</h2>
          </div>
          <div class="overlay-header-actions">
            <span class="panel-corner-label">{{ activeOverlayCorner }}</span>
            <button type="button" class="ghost-button tiny-button" @click="closeOverlay()">收起</button>
          </div>
        </header>

        <div v-if="activeOverlay === 'codex'" class="overlay-body">
          <div class="detail-stack">
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
        </div>

        <div v-else-if="activeOverlay === 'settings'" class="overlay-body overlay-body--settings">
          <div class="detail-stack settings-stack">
            <article class="detail-card settings-card">
              <p class="detail-title">此卷在身</p>
              <p>道号：{{ store.account || '未署名' }}</p>
              <p>角色：{{ player.characterName || '未显形' }} · 境界：{{ formatProgressStageLabel(String(player.cultivation?.realmName || player.stageLabel || '凡躯')) }}</p>
              <p>所在：{{ sceneDisplayTitle }}</p>
            </article>

            <article class="detail-card settings-card">
              <p class="detail-title">卷内近况</p>
              <p>当前模式：{{ composerTitle }}</p>
              <p>宗门与来处：{{ player.sect?.sectName || '散修' }} · {{ player.race?.originName || '未定来处' }}</p>
              <p>主线：{{ sceneMissionText || '此地暂无明示线索，可先听风声、看去路、与人交谈。' }}</p>
            </article>

            <div class="action-row settings-action-row">
              <button type="button" class="secondary-button" :disabled="store.loading" @click="void submitComposer('here')">再看此地</button>
              <button type="button" class="secondary-button" :disabled="store.loading" @click="retryBootstrap()">重整卷页</button>
              <button type="button" class="primary-button" :disabled="store.loading" @click="store.logout()">离界</button>
            </div>
          </div>
        </div>
      </section>
    </div>
  </div>
</template>
