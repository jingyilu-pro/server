# 凡人修仙 MUD 对齐 `oiuv-mud` 实施补遗

## 目的
- 本文是对根目录《凡人修仙 MUD 对齐 oiuv-mud 改造计划（深度对齐整套 MUD）PLAN.md》的执行补遗。
- 原计划方向正确，但有几处实现层尚未落到“怎么做、存哪、怎么回归”的粒度；本补遗用于把这些缺口补成可执行约束。

## 已确认的实现原则
- 继续使用当前仓库技术路线：
  - `manager + login + game` C++ 服务
  - `Vue3 + TS + Vite`
  - `protobuf over HTTP + 短轮询`
- 不切 Telnet / WebSocket / FluffOS。
- 世界观、名词、门派、场景、物件全部保持凡人修仙，不引入金庸内容。
- 当前 `121` 房间世界包继续作为基础盘，不横向大扩图，先做共享世界密度重构。

## 复核结论
- 主计划方向是对的，产品定位、前后端形态、帮助/工作/公众任务/门派身份这些大项没有跑偏。
- 现阶段最容易漏掉的不是“还要不要加系统”，而是 5 类落地约束：
  - `范围收口`：哪些算阶段一必须交付，哪些只进骨架不闭环。
  - `验收口径`：每一块做到什么程度才算“完成”，避免看起来做了、实际不可玩。
  - `迁移回退`：旧角色、旧事件、旧前端缓存、旧场景快照在新协议下如何兜底。
  - `运营治理`：留言板、频道、世界事件、排行榜需要最低限度的治理规则。
  - `自动化回归`：帮助、风声、板帖、排行、建角、恢复、筑基主链都要有固定烟测入口。
- 因此本补遗新增的重点不是再加功能，而是把主计划补成“可执行、可验收、可长期维护”的版本。

## 当前阶段必须补齐的缺口总表

### A. 范围切片还需要更明确
- 主计划里“终端主界面 / 帮助系统 / 公众任务 / 工作 / 门派 / 散修 / 排行 / 留言板”都写了，但缺少 `P0 / P1 / P2` 的明确边界。
- 阶段一建议硬收口为：
  - `P0 必做`
    - 终端式单主文本流
    - `help / commands / newbie`
    - `ask <npc> about rumor`
    - `work / board / duty / wanted / travel / claim`
    - 房间级留言板 `board/read/post/discard`
    - 四条身份线的“摘要级闭环”
  - `P1 继续做`
    - 更完整的公众任务轮换
    - 更深的门派升阶
    - 房间服务标签全图覆盖
    - 排行与首席周结算
  - `P2 长线运营`
    - 板子治理
    - 服级事件调度
    - 更强的经济系统
    - 中后期境界扩线

### B. 验收口径需要写成 DoD
- 原计划写了“要有帮助、要有工作、要有留言板”，但缺少严格的完成定义。
- 阶段一建议统一采用以下 `Done Definition`：
  - `帮助系统`
    - 至少 `8` 个帮助主题
    - `help <topic>` 可直出主文本字符板
    - `commands` 能列出别名、usage、示例
  - `风声与公众任务`
    - 嘉元城、太南谷、天南港至少各 `1` 个 rumor 源
    - `ask <npc> about rumor` 返回正文、相关事务、后续提示
  - `工作系统`
    - `work` 至少能在 `7` 个关键场景给出工作摘要
    - 每个工作都具备 `发起人 / 地点 / 奖励 / 指令提示`
  - `留言板`
    - 有板子的房间必须支持 `board/read/post/discard`
    - 刷新和重登后帖子仍能看到
    - `discard` 只对当前玩家生效
  - `身份线`
    - 散修 / 七玄门 / 黄枫谷 / 灵兽山都能看到身份摘要、下一阶门槛、事务入口
  - `前端终端化`
    - 高信息密度内容全部进入主文本流
    - 不依赖高频 overlay 才能完成核心操作

### C. 迁移与兼容策略还要写清楚
- 主计划默认“兼容旧账号旧角色”，但最好补成明确规则：
  - `旧角色`
    - 缺失 `identity_track / contribution_state / unread_board_count` 时，服务端即时回填默认值
  - `旧前端`
    - 若新字段缺失，客户端回退到基础字符板渲染，不报错不白屏
  - `旧事件`
    - 旧 `GameEvent` 没有 `channel/tone/render_mode` 时，由客户端走默认日志样式
  - `旧世界包`
    - `help_topics/jobs/identity_tracks/rumor_sources` 缺项应在构建期失败，而不是运行期静默空掉

### D. 运营治理条目在主计划里偏弱
- 对齐 `oiuv/mud` 之后，长期最容易失控的是频道和留言板；这些应先写最低约束：
  - `留言板`
    - 单帖长度上限
    - 单账号发帖频率限制
    - 板帖保留数量与清理规则
  - `频道`
    - `world/trade/rumor` 的频率限制
    - 新手帮助优先级与刷屏兜底
  - `排行`
    - 结算频率
    - 同分排序规则
    - 首席榜的并列处理

### E. 自动化回归矩阵还不够完整
- 当前已有世界构建、前端测试、客户端 build、WSL `mud_smoke`，但建议把场景固定成矩阵：
  - `建角与恢复`
    - 注册 -> 登录 -> 建角 -> 刷新恢复
  - `帮助`
    - `help newbie`
    - `commands`
    - `help work`
  - `风声`
    - `ask <npc> about rumor`
  - `留言板`
    - `board -> post -> read -> discard`
  - `排行`
    - `rank`
    - `rank wealth`
  - `身份`
    - `duty`
    - `claim`
  - `筑基主链`
    - 七玄门 -> 太南 -> 黄枫谷 -> 天南港 -> 乱星海 -> 虚天殿 -> 筑基初期

## 需要补齐的执行缺口

### 1. 帮助系统的数据源与载体
- 新增 `doc/mud/source/mud_help_manual.mjs`。
- 数据结构采用“主题条目”而不是直接塞死字符串：
  - `topic_id`
  - `title`
  - `summary`
  - `body_lines`
  - `keywords`
  - `related_commands`
  - `inline_commands`
  - `category`
- 主题首批至少覆盖：
  - `newbie`
  - `commands`
  - `channels`
  - `work`
  - `freequest`
  - `board`
  - `rank`
  - `sects`
  - `map_tiannan`
  - `map_chaos_sea`

### 2. 公众任务 / 工作 / 风声的数据源
- 新增 `doc/mud/source/mud_jobs_rumors.mjs`。
- 把“公共委托板”和“问风声接活”拆成两层：
  - `rumor_sources`
    - 绑定场景或 NPC
    - 可被 `ask <npc> about rumor`、`rumor`、`listen` 命中
    - 指向可接 `job_id / quest_id / route_hint`
  - `jobs`
    - 定义工作循环
    - 可以是重复事务，不强制都落入当前 `MudQuestState`
- `jobs` 首批字段：
  - `job_id`
  - `title`
  - `kind`
  - `scene_id`
  - `issuer_npc_id`
  - `submit_npc_id`
  - `summary`
  - `description`
  - `requirements`
  - `reward_summary`
  - `command_hint`
  - `repeatable`
  - `route_tag`
  - `service_tag`
- `rumor_sources` 首批字段：
  - `source_id`
  - `scene_id`
  - `npc_id`
  - `topic`
  - `summary`
  - `body_lines`
  - `job_ids`
  - `quest_ids`
  - `unlock_flags`

### 3. 身份线 / 称号 / 场景服务标签
- 新增 `doc/mud/source/mud_titles_factions.mjs`。
- 首批结构拆分为：
  - `identity_tracks`
  - `scene_services`
- `identity_tracks` 负责散修与门派阶位，不直接把逻辑写死在运行时代码里：
  - `track_id`
  - `name`
  - `kind`
  - `ranks`
  - `mentor_ids`
  - `description`
  - `service_unlocks`
- `scene_services` 负责把房间标成：
  - `board_available`
  - `service_tags`
  - `mentor_ids`
  - `rumor_topics`

### 4. 留言板的最小可运行实现
- 本轮不新增独立 MySQL 板子表，先走“房间级板帖事件 + 玩家本地隐藏状态”的轻实现。
- 具体约束：
  - `post <title>=<content>` 在当前有板子的房间发帖。
  - 帖子写入全局事件流，`type=board_post`。
  - `title` 编码为：`<scene_id>|<subject>`。
  - `content` 存帖文正文。
  - `read <id>` 读取当前房间最近帖子。
  - `discard <id>` 不是删除全局帖子，而是写入玩家 `flags_json`，将该帖对当前玩家隐藏。
- 这样能满足：
  - 房间级共享可见
  - 有基础持久化
  - 不引入额外数据库迁移
- 后续若要做长期运营版，再抽出真正的 `mud_board_post` 表与管理能力。

### 5. Proto 字段的真实使用方式
- 本轮新增 proto 字段不能只“加定义不落地”，必须在服务端或前端至少有一处实际消费。
- 约束如下：
  - `CommandDefinition.aliases/usage/target_hint/visibility_scope`
    - 用于终端命令目录和帮助页
  - `StructuredPanel.document_id/body_lines/panel_kind`
    - 用于 help、留言板、rank ASCII 榜牌、工作说明
  - `SceneSnapshot.service_tags/rumor_topics/board_available/mentor_ids`
    - 用于主终端场景提示与命令引导
  - `PlayerSnapshot.identity_track/rank_level/contribution_state/reputation_state/unread_board_count`
    - 用于 prompt、副提示、family/duty/board 摘要
  - `QuestSummary.quest_kind/repeatable/issuer_hint`
    - 用于任务札记

### 6. 前端终端化的收口原则
- 不推倒当前已形成的“单主文本流 + 保留下方方向盘”方向。
- 本轮前端重点不是做更多组件，而是继续拆掉剩余出戏结构：
  - 高存在感卡片边框
  - 页面式感知过强的 overlay
  - 缺乏命令语义的操作按钮
- 高优先级渲染模式固定为：
  - `log_line`
  - `notice_block`
  - `board_block`
  - `dossier_block`
  - `roster_block`
  - `ascii_map`

## 本轮验收补充
- `help <topic>` 必须可用，且至少有 8 个主题。
- `commands` 必须列出高频命令、别名、示例。
- `ask <npc> about rumor` 在嘉元城、太南谷、天南港至少各有 1 处可触发。
- `work` 必须给出当前可做工作摘要。
- `board/read/post/discard` 必须形成最小闭环。
- `rank` 必须能直接在 `command/execute` 里生成终端面板，不依赖单独页面。
- 场景必须能下发并显示：
  - `service_tags`
  - `rumor_topics`
  - `board_available`
  - `mentor_ids`

## 后续阶段说明
- 这份补遗只解决“计划落地细节不足”的问题，不替代根目录主计划。
- 长线运营方向另见《凡人修仙 MUD 长线运营任务骨架.md》。

## 建议追加到根目录计划的执行顺序
1. 先收 `P0`：帮助、风声、工作、留言板、身份摘要、终端主界面。
2. 再补 `P1`：更多场景服务标签、更多 rumor 源、更多工作轮换、更多门派深度。
3. 最后再开 `P2`：周事件、首席结算、经济深化、治理与运营工具。

## 阶段一交付检查表
- `内容构建`
  - `doc/mud/source` 可稳定生成世界包
  - 引用缺失时构建失败
- `协议落地`
  - 新 proto 字段被服务端真实写入
  - 客户端对新增字段有至少一处真实消费
- `玩法闭环`
  - `help/newbie/commands`
  - `ask about rumor`
  - `work`
  - `board/read/post/discard`
  - `duty/wanted/travel/claim`
- `兼容恢复`
  - 旧号刷新可恢复
  - 新字段缺失时不白屏
- `运营准备`
  - 留言板和频道有基础约束
  - 排行和首席有固定结算口径
