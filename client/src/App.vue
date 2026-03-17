<script setup lang="ts">
import { computed, onMounted, ref, watch } from 'vue'

import { useGameStore } from '@/stores/game'

const store = useGameStore()

const account = ref(store.account)
const password = ref('')
const characterName = ref('')
const command = ref('')
const activeTab = ref<'player' | 'quests' | 'inventory' | 'map' | 'team' | 'rank'>('player')

const quickCommands = [
  'look',
  'map',
  'go north',
  'go south',
  'go west',
  'talk 墨府总管',
  'accept qixuan_herb',
  'fight 青木狼',
  'practice 长春功',
  'breakthrough',
  'team create',
  'team info',
  'event',
]

const scene = computed(() => store.scene ?? {})
const player = computed(() => store.player ?? {})
const recentEvents = computed(() => [...store.events].reverse())
const exits = computed(() => (scene.value.exits as Record<string, any>[] | undefined) ?? [])
const inventory = computed(() => (player.value.inventory as Record<string, any>[] | undefined) ?? [])
const quests = computed(() => (player.value.quests as Record<string, any>[] | undefined) ?? [])
const npcs = computed(() => (scene.value.npcs as Record<string, any>[] | undefined) ?? [])
const monsters = computed(() => (scene.value.monsters as string[] | undefined) ?? [])
const shops = computed(() => (scene.value.shops as string[] | undefined) ?? [])
const teamMembers = computed(() => (player.value.team?.members as Record<string, any>[] | undefined) ?? [])

function setError(error: unknown) {
  store.error = error instanceof Error ? error.message : '发生未知错误'
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

async function submitCommand(value?: string) {
  const nextCommand = (value ?? command.value).trim()
  if (!nextCommand) {
    return
  }

  try {
    await store.executeCommand(nextCommand)
    command.value = ''
  } catch (error) {
    setError(error)
  }
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
  } catch (error) {
    setError(error)
  }
})

watch(activeTab, async (value) => {
  if (value !== 'rank' || !store.authenticated) {
    return
  }

  try {
    await store.loadRankings(store.rankingType)
  } catch (error) {
    setError(error)
  }
})
</script>

<template>
  <div class="shell">
    <header class="hero">
      <div>
        <p class="eyebrow">H5 · Protobuf · MUD</p>
        <h1>凡人修仙传 · 人界修行录</h1>
        <p class="subtitle">
          以文字为主的修仙世界，沿着七玄门、嘉元城、太南小会一路摸索灵根与机缘。
        </p>
      </div>
      <div class="hero-actions">
        <div class="status-chip" v-if="store.authenticated">
          <span>账号</span>
          <strong>{{ store.account }}</strong>
        </div>
        <button v-if="store.authenticated" class="ghost-button" @click="store.logout()">退出</button>
      </div>
    </header>

    <p v-if="store.error" class="error-banner">{{ store.error }}</p>

    <section v-if="!store.authenticated" class="auth-card">
      <div class="card-heading">
        <h2>入世登录</h2>
        <p>先走现有 `manager -> login -> game` 链路，再进入 MUD 世界。</p>
      </div>
      <div class="form-grid">
        <label>
          <span>账号</span>
          <input v-model="account" autocomplete="username" placeholder="例如 hanli_001" />
        </label>
        <label>
          <span>密码</span>
          <input v-model="password" type="password" autocomplete="current-password" placeholder="输入密码" />
        </label>
      </div>
      <div class="action-row">
        <button class="primary-button" :disabled="store.loading" @click="login(false)">登录</button>
        <button class="secondary-button" :disabled="store.loading" @click="login(true)">注册并登录</button>
      </div>
    </section>

    <section v-else-if="store.needCreateCharacter" class="auth-card">
      <div class="card-heading">
        <h2>塑造角色</h2>
        <p>当前版本默认一账号一个活跃角色，落点从七玄门外场开始。</p>
      </div>
      <div class="form-grid">
        <label>
          <span>角色名</span>
          <input v-model="characterName" maxlength="24" placeholder="例如 韩立" @keyup.enter="createCharacter()" />
        </label>
      </div>
      <div class="action-row">
        <button class="primary-button" :disabled="store.loading" @click="createCharacter()">踏入修仙路</button>
      </div>
    </section>

    <main v-else class="mud-layout">
      <section class="panel feed-panel">
        <div class="panel-header">
          <h2>世界消息</h2>
          <span>{{ recentEvents.length }} 条</span>
        </div>
        <div class="feed-list">
          <article v-for="event in recentEvents" :key="event.eventId" class="feed-item">
            <p class="feed-title">{{ event.title }}</p>
            <p class="feed-content">{{ event.content }}</p>
          </article>
        </div>
      </section>

      <section class="panel command-panel">
        <div class="panel-header">
          <div>
            <h2>{{ scene.sceneName || '未知场景' }}</h2>
            <p>{{ scene.regionName }}</p>
          </div>
          <div class="status-chip">
            <span>境界</span>
            <strong>{{ player.cultivation?.realmName || '凡躯' }}</strong>
          </div>
        </div>

        <section class="scene-card">
          <p class="scene-desc">{{ scene.description }}</p>
          <div class="meta-row">
            <span class="meta-tag" v-for="npc in npcs" :key="npc.npcId">人物 · {{ npc.name }}</span>
            <span class="meta-tag" v-for="monster in monsters" :key="monster">妖兽 · {{ monster }}</span>
            <span class="meta-tag" v-for="shop in shops" :key="shop">坊市 · {{ shop }}</span>
          </div>
          <div class="exit-row">
            <button
              v-for="exit in exits"
              :key="exit.direction"
              class="secondary-button small"
              @click="submitCommand(`go ${exit.direction}`)"
            >
              {{ exit.direction }} · {{ exit.targetSceneName }}
            </button>
          </div>
        </section>

        <section class="result-card" v-if="store.lastResult">
          <p class="result-title">{{ store.lastResult.title }}</p>
          <p class="result-summary">{{ store.lastResult.summary }}</p>
          <div class="meta-row">
            <span class="meta-tag" v-for="hint in store.lastResult.hints || []" :key="hint">{{ hint }}</span>
          </div>
        </section>

        <section class="quick-actions">
          <button
            v-for="item in quickCommands"
            :key="item"
            class="ghost-button small"
            @click="submitCommand(item)"
          >
            {{ item }}
          </button>
        </section>

        <form class="command-form" @submit.prevent="submitCommand()">
          <input
            v-model="command"
            placeholder="输入命令，例如 look / fight 青木狼 / chat world 道友好"
            :disabled="store.loading"
          />
          <button class="primary-button" :disabled="store.loading">执行</button>
        </form>
      </section>

      <section class="panel side-panel">
        <div class="tab-row">
          <button class="tab-button" :class="{ active: activeTab === 'player' }" @click="activeTab = 'player'">人物</button>
          <button class="tab-button" :class="{ active: activeTab === 'quests' }" @click="activeTab = 'quests'">任务</button>
          <button class="tab-button" :class="{ active: activeTab === 'inventory' }" @click="activeTab = 'inventory'">背包</button>
          <button class="tab-button" :class="{ active: activeTab === 'map' }" @click="activeTab = 'map'">地图</button>
          <button class="tab-button" :class="{ active: activeTab === 'team' }" @click="activeTab = 'team'">队伍</button>
          <button class="tab-button" :class="{ active: activeTab === 'rank' }" @click="activeTab = 'rank'">排行</button>
        </div>

        <div v-if="activeTab === 'player'" class="detail-stack">
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

        <div v-else-if="activeTab === 'quests'" class="detail-stack">
          <article v-for="quest in quests" :key="quest.questId" class="detail-card">
            <p class="detail-title">{{ quest.title }}</p>
            <p>{{ quest.description }}</p>
            <p>状态：{{ quest.status }} · 进度：{{ quest.progress }} / {{ quest.target }}</p>
          </article>
          <p v-if="quests.length === 0" class="empty-text">当前没有任务。</p>
        </div>

        <div v-else-if="activeTab === 'inventory'" class="detail-stack">
          <article v-for="item in inventory" :key="item.itemId" class="detail-card">
            <p class="detail-title">{{ item.name }}</p>
            <p>{{ item.description }}</p>
            <p>数量：{{ item.quantity }} <span v-if="item.equipped">· 已装备</span></p>
          </article>
          <p v-if="inventory.length === 0" class="empty-text">背包空空如也。</p>
        </div>

        <div v-else-if="activeTab === 'team'" class="detail-stack">
          <article class="detail-card">
            <p class="detail-title">{{ player.team?.teamName || '暂无队伍' }}</p>
            <p v-if="teamMembers.length === 0">使用 `team create` 创建队伍，或 `team join &lt;leader_account&gt;` 加入同道。</p>
            <p v-else>队伍编号：{{ player.team?.teamId }}</p>
          </article>
          <article v-for="member in teamMembers" :key="member.account" class="detail-card">
            <p class="detail-title">{{ member.displayName }}</p>
            <p>{{ member.account }} <span v-if="member.leader">· 队长</span></p>
          </article>
        </div>

        <div v-else-if="activeTab === 'rank'" class="detail-stack">
          <div class="action-row">
            <button class="ghost-button small" @click="loadRanking('realm')">境界榜</button>
            <button class="ghost-button small" @click="loadRanking('wealth')">财富榜</button>
            <button class="ghost-button small" @click="loadRanking('combat')">战力榜</button>
          </div>
          <article v-for="entry in store.rankings" :key="`${store.rankingType}-${entry.rank}`" class="detail-card">
            <p class="detail-title">#{{ entry.rank }} · {{ entry.characterName }}</p>
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
          <article class="detail-card" v-for="exit in exits" :key="exit.direction">
            <p class="detail-title">{{ exit.direction }}</p>
            <p>{{ exit.targetSceneName }}</p>
          </article>
        </div>
      </section>
    </main>
  </div>
</template>
