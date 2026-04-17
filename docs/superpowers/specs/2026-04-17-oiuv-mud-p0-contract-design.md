# 凡人修仙 MUD 对齐 oiuv-mud P0 契约收口设计

## 1. 设计结论

本轮目标固定为：

- 不新增大系统，不横向扩世界，而是把已经做出来的 `oiuv/mud` 对齐能力收口成稳定契约。
- 第一优先级是后端运行时拆分，其次是前端按契约降权，最后清理 P0 遗留和回归矩阵。
- 本轮交付重点是“更稳、更易迭代”，不是“再堆更多玩法”。

本轮聚焦的 P0 主链为：

- `help / commands / newbie`
- `rumor / ask <npc> about rumor`
- `work`
- `board / read / post / discard`
- `rank`
- `identity / contribution / reputation / unread board`
- `scene service tags / rumor topics / mentor hints`

本轮的产品意图是：

- 让当前项目更像成熟 MUD 的命令驱动壳，而不是继续依赖大文件堆功能。
- 让前后端之间的语义边界更清楚，后续继续对齐 `oiuv/mud` 时不再频繁返工。

## 2. 非目标

本轮不做：

- WebSocket / Telnet / FluffOS 改造
- 新后端服务、BFF、独立事件服务
- 大规模扩地图、扩门派、扩境界主线
- 重写前端壳或移除当前方向盘布局
- 把留言板、频道、工作系统升级成更重的独立数据库子系统

说明：

- 当前方向不是“能力不够”，而是“现有能力组织得还不够稳”。
- 本轮可以顺手修正体验不一致处，但不以新增玩法为交付目标。

## 3. 背景判断

当前仓库已经具备较完整的 `oiuv/mud` 对齐基础：

- 服务端已有 `help / commands / rumor / work / board / read / post / discard / rank / identity_track / service_tags`
- 前端已有 `StructuredPanel`、主文本流、场景提示、短命令签、底部输入与方向盘
- proto 已具备：
  - `CommandDefinition.aliases / usage / target_hint / visibility_scope`
  - `StructuredPanel.document_id / body_lines / panel_kind`
  - `SceneSnapshot.service_tags / rumor_topics / board_available / mentor_ids`
  - `PlayerSnapshot.identity_track / rank_level / contribution_state / reputation_state / unread_board_count`
  - `QuestSummary.quest_kind / repeatable / issuer_hint`

真正的问题在于：

- [`mud_game_runtime.cpp`](C:/Work/Projects/server/app/service/game/logic/mud_game_runtime.cpp) 承载过多责任，P0 主链逻辑高度聚集。
- [`App.vue`](C:/Work/Projects/server/client/src/App.vue) 已能消费大量契约字段，但仍保留较多前端硬编码玩法映射和猜测式组装。
- 当前最值钱的优化不是增加新功能，而是把已存在的 P0 能力整理成稳定边界。

## 4. 范围与边界

### 4.1 本轮范围

本轮只做两类核心工作：

- 服务端运行时拆分并稳定输出契约
- 前端消费逻辑降权并更依赖服务端直给语义

### 4.2 本轮边界

本轮坚持以下约束：

- 继续使用当前 `manager + login + game` C++ 服务
- 继续使用 `Vue3 + TS + Vite`
- 继续使用 `protobuf over HTTP + 短轮询`
- 保持凡人修仙世界观、名词、门派、场景与已有内容盘

### 4.3 完成定义

本轮完成不以“加了多少新内容”为准，而以以下结果为准：

- `mud_game_runtime.cpp` 明显瘦身，P0 逻辑不再继续堆叠在单文件中
- P0 主链行为不回退
- 前端对 `StructuredPanel / SceneSnapshot / PlayerSnapshot / QuestSummary` 的依赖更直接
- 前端静态玩法映射在 P0 主链上降为兜底，不再是主数据源

## 5. 服务端模块边界

服务端保留 `MudGameRuntime` 作为命令入口与总控，但把 P0 主链拆成聚焦责任面。

### 5.1 manual/help lane

负责：

- `help`
- `commands`
- `newbie`
- 帮助主题匹配
- 命令目录面板生成

边界要求：

- 只负责帮助与命令说明
- 不混入场景、排行、任务状态等非手册语义

### 5.2 board/work/rumor lane

负责：

- `rumor`
- `ask <npc> about rumor`
- `work`
- `board`
- `read`
- `post`
- `discard`

边界要求：

- 这部分视为同一条“公共发现链”
- 统一走 `StructuredPanel` + hints + inline commands 输出
- 保持现有轻量持久化与玩家本地隐藏逻辑，不升级成重系统

### 5.3 rank/identity lane

负责：

- `rank`
- `family`
- `duty`
- `identity_track`
- `rank_level`
- `contribution_state`
- `reputation_state`
- `unread_board_count`

边界要求：

- 统一承接“玩家社会身份摘要”
- 不把身份摘要逻辑散落在多个无关命令分支里

### 5.4 scene projection lane

负责把 `scene + player` 投影成终端真正需要的场景快照：

- `service_tags`
- `rumor_topics`
- `board_available`
- `mentor_ids`
- `local_board_entries`
- `presence_board`
- `exit_board`
- `available_short_commands`

边界要求：

- 只负责可见信息整理
- 不直接承担命令执行或玩法结算

### 5.5 panel builder lane

负责统一 `MudStructuredPanelState` 到 proto 输出的规范：

- `panel_id`
- `panel_kind`
- `document_id`
- `summary`
- `entries`
- `body_lines`
- `inline_commands`
- `render_mode`
- `style_id`

边界要求：

- 统一终端面板风格与语义
- 避免每个命令分支各自手写一套输出格式

## 6. 契约设计

### 6.1 总原则

P0 主链中的玩法语义由服务端定义，前端负责终端化呈现，而不是继续承担主要业务解释职责。

稳定数据流固定为：

- 世界配置 / 玩家状态 / 事件流
- 运行时分 lane 计算
- 统一 panel builder 产出 `StructuredPanel + SceneSnapshot + PlayerSnapshot`
- 通过 `bootstrap / command/execute / feed` 下发
- 前端写入单主文本流并给出快捷动作

### 6.2 CommandDefinition 契约

以下字段视为稳定契约：

- `aliases`
- `usage`
- `target_hint`
- `visibility_scope`

要求：

- 命令目录和帮助页直接使用这些字段
- 前端不再自己推断命令说明文本

### 6.3 StructuredPanel 契约

以下字段视为稳定契约：

- `panel_kind`
- `document_id`
- `body_lines`
- `inline_commands`

语义约束：

- `panel_kind` 定义面板属于帮助、榜单、帖子、工作板、风声札记等哪一类
- `document_id` 定义该面板对应的具体文档或实体标识
- `body_lines` 承担正文
- `inline_commands` 承担下一步建议动作

要求：

- 前端优先根据 `panel_kind + document_id` 理解语义
- `panel_id` 主要保留给样式、去重和局部兼容

### 6.4 SceneSnapshot 契约

以下字段视为稳定契约：

- `service_tags`
- `rumor_topics`
- `board_available`
- `mentor_ids`
- `local_board_entries`

要求：

- “此地可做什么”由服务端直接表达
- 前端只负责把这些信息编写成终端行文

### 6.5 PlayerSnapshot 契约

以下字段视为稳定契约：

- `identity_track`
- `rank_level`
- `contribution_state`
- `reputation_state`
- `unread_board_count`

要求：

- 身份提示、副 prompt、未读板帖、贡献与声望摘要均以这些字段为准
- 前端不再从其他零散状态二次拼装社会身份

### 6.6 QuestSummary 契约

以下字段视为稳定契约：

- `quest_kind`
- `repeatable`
- `issuer_hint`

要求：

- 当前线索和札记不再只显示标题与进度
- 任务属于何种来源与循环类型应由服务端直接标明

## 7. 前端降权原则

### 7.1 保留的前端职责

前端继续负责：

- 单主文本流渲染
- 面板压缩、截断和主次信息组织
- 快捷动作入口
- 兼容旧字段的 fallback

### 7.2 需要降权的逻辑

以下逻辑需要降权：

- 通过 `panel_id` 猜测面板业务语义
- 通过前端静态表推导场景主玩法
- 通过零散玩家字段自行拼出身份摘要

### 7.3 遗留静态表策略

像 `sceneQuestOffers / sceneSectOffers` 这类前端静态表：

- 本轮先降为兜底，不要求一口气删干净
- 在 P0 主链上不能继续作为主要数据源
- 如与服务端下发结果冲突，以服务端为准

### 7.4 overlay 策略

本轮不重写 overlay，但要继续收口：

- `help / commands / rumor / work / board / read / rank` 的核心体验优先留在主文本流
- `codex`、长文、低频阅读层可继续留在 overlay

## 8. 实施顺序

### 8.1 第一批：服务端拆分但不改行为

先拆：

- `help/commands`
- `board/work/rumor`
- `rank/identity`
- `panel builder`
- `scene snapshot projection`

要求：

- 命令入口仍由 runtime 统一分发
- 输出行为优先保持与当前一致
- 行为回归覆盖 `help / commands / rumor / work / board / read / post / discard / rank`

### 8.2 第二批：前端按契约降权

再收：

- `StructuredPanel` 主语义判断从 `panel_id` 转向 `panel_kind + document_id`
- 场景提示更多建立在 `service_tags / rumor_topics / board_available / mentor_ids`
- 身份提示更多建立在 `identity_track / rank_level / contribution_state / reputation_state / unread_board_count`

### 8.3 第三批：清理 P0 遗留与回归矩阵

最后完成：

- P0 主链对前端静态表的依赖下沉为兜底
- `help -> rumor/work/board -> rank/identity` 的发现链复核
- smoke、build、client test 固定成持续回归入口

## 9. 验收口径

### 9.1 代码验收

- [`mud_game_runtime.cpp`](C:/Work/Projects/server/app/service/game/logic/mud_game_runtime.cpp) 明显瘦身
- P0 逻辑已有明确模块归属，不再继续向 runtime 总文件累积
- [`App.vue`](C:/Work/Projects/server/client/src/App.vue) 中硬编码玩法判断减少

### 9.2 行为验收

以下行为不得回退：

- `help`
- `commands`
- `rumor`
- `ask <npc> about rumor`
- `work`
- `board`
- `read`
- `post`
- `discard`
- `rank`

同时需保持：

- `SceneSnapshot` 仍能清楚表达“此地能做什么”
- `PlayerSnapshot` 仍能清楚表达“我当前是什么身份、下一步差什么”

### 9.3 验证入口

交付基线固定包含：

- `npm --prefix client test -- --run`
- `npm --prefix client run build`
- `node scripts/build_mud_world.mjs`
- 现有 `mud_smoke` 或等价烟测，至少覆盖：
  - `help`
  - `rumor`
  - `board`
  - `work`
  - `rank`

## 10. 风险与约束

### 10.1 主要风险

- 运行时拆分过程中引入细微行为漂移
- 前端在降权过程中误删仍有价值的终端组织逻辑
- 遗留静态表与服务端新契约并存一段时间，可能出现双源不一致

### 10.2 控制策略

- 拆分优先保持行为不变
- 先抽边界，再做简化
- 旧逻辑保留兼容 fallback，再逐步删减
- 用固定 smoke 场景保护 P0 主链

## 11. 后续衔接

本设计完成后，下一步实现计划应继续遵循：

- 先拆服务端边界
- 再降前端业务解释权
- 最后清遗留并补回归矩阵

后续若继续深度对齐 `oiuv/mud`，优先级应保持：

- 契约稳定
- 终端一致性
- 可发现性链路
- 低风险迭代

而不是重新回到“大文件里继续堆功能”的路径。
