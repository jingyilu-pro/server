# 凡人修仙 MUD 对齐 `oiuv/mud` 改造计划（深度对齐整套 MUD）

## Summary
- 产品目标改为“凡人修仙世界观下的共享世界修仙 MUD”，对齐 `oiuv/mud` 的成熟形态：终端式前端、命令优先、帮助系统、工作/公众任务/门派贡献/头衔/留言板/排行/频道/长期身份线。
- 世界组织固定为“共享世界沙盒”。玩家是原创角色，散修起步，可长期不入门，也可加入七玄门、黄枫谷、灵兽山等路线；韩立与原著剧情降为时代背景、传闻、稀有遭遇、世界事件和手册内容。
- 技术路线保持不变：继续使用现有 `manager + login + game` C++ 服务、`Vue3 + TS + Vite` 客户端、`protobuf over HTTP + 短轮询`、同域代理；不改 telnet/WebSocket/FluffOS。
- 当前内容基线继续沿用现有世界包：`121` 房间、`80` NPC、`62` 敌对目标、`121` 物品、`34` 任务、`462` 手册条目。本轮优先做“重组和密度提升”，不先横向扩图。

## Implementation Changes
### 1. 产品与内容重定位
- 把当前“七玄门到筑基初期”的章节体验，改造成 `新手入世 -> 公共生计 -> 散修/门派分流 -> 炼气成长 -> 筑基准备` 的共享世界闭环。
- 内容固定整理为 4 条长期可玩身份线：`散修 / 七玄门 / 黄枫谷 / 灵兽山`。其它势力先保留为世界势力、传闻源和后续开放对象。
- 当前任务统一重排为 4 类：
  - `新手引导任务`：只负责教会基本命令和保命。
  - `公众任务`：通过坊市、店家、牙人、船主、驿站等 NPC 用 `ask <npc> about rumor` 触发。
  - `工作任务`：药圃、采药、抄录、送信、海猎、驯兽、炼丹等可重复循环。
  - `门派事务`：贡献、阶位、头衔、门派资源入口。
- 现有“主线追踪”改成“当前线索 / 当前差事”，不再把玩家锁在单条剧情线上。

### 2. 前端完全对齐终端式 MUD 壳
- 主界面固定改成 `参考 oiuv/mud 的终端客户端 + 现有移动端单手布局`：
  - 顶部只保留连接/状态 prompt，两行内显示角色、境界、HP/MP/STA、位置、频道、风险提示。
  - 中部只保留一个主终端滚屏，聊天、场景描述、系统消息、战斗、人物资料、背包、任务、排行、帮助全部写入同一文本流。
  - 底部只保留 `输入框 + 发送键 + 8 个以内短命令签 + 现有方向盘`；方向盘保留。
- 视觉切到 `oiuv/mud` 式黑底终端：
  - 黑/深灰主底，等宽字体，少量 ANSI 语义色。
  - 去掉当前残留的卷轴/卡片/抽屉式高频表现；只保留极少数长文阅读层。
  - 高亮只用于频道、危险、奖励、地点、人名、头衔，不再让按钮和面板压过正文。
- 高频交互统一终端内嵌：
  - `score / hp / skills / bag / map / who / rank / help / quest / duty / board` 统一渲染成 ASCII/字符板。
  - 点击可见对象时不弹详细卡，改为把该对象介绍、可继续命令、相关线索直接追加到主终端。
- 登录、建角、恢复页一起改成 `终端连接页`：
  - 像参考项目的连接面板，但字段仍是现有账号/密码/建角资料。
  - 流程固定为 `账号 -> 连接 -> 塑形（姓名/出身/背景） -> 入世`。

### 3. 命令体系、帮助系统、信息架构按成熟 MUD 重组
- 命令体系固定按“帮助先行 + 同义词 + 终端自解释”收口：
  - 保留现有修仙命令：`look/go/talk/accept/submit/fight/use/practice/meditate/breakthrough/harvest/loot/cast/brew/chat/team/...`
  - 增加第一批 MUD 基础命令与别名：`help`, `commands`, `newbie`, `hp`, `score`, `skills`, `bag|i`, `say`, `tell`, `reply`, `who`, `rank`, `board`, `read`, `post`, `ask <npc> about rumor|topic`, `work`, `duty`, `save`
  - 所有高频命令都必须有中文描述、短别名、帮助条目和示例输出。
- 新增结构化帮助系统，内容源独立于手册：
  - 固定提供 `help/newbie/commands/channels/sects/work/freequest/rank/map_<region>` 等主题。
  - `help <topic>` 在主文本流输出字符版帮助，长文才进入低频阅读层。
  - `codex` 保留为世界 lore；`help` 负责玩法说明，二者不混用。
- 客户端命令入口从“玩法分类按钮”改成“终端命令目录 + 当前场景可做什么”：
  - 短命令签只保留 `看/听/问/行/札/囊/我/榜/图` 这类高频动作。
  - 更多命令靠 `help`、`commands`、场景提示和对象输出引导，不再靠页面式按钮矩阵。

### 4. 后端玩法生态按 `oiuv/mud` 方式打开，但全部凡人化
- 公众任务系统：
  - 在嘉元城、太南谷、天南港、散修坊、客栈/坊市 NPC 上实现 `ask about rumor` 取线索。
  - 每条公众任务走 `问讯 -> 接线 -> 执行 -> 提交 -> 后续传闻` 闭环，可重复、可轮换。
- 工作系统首批固定做 `采药 / 炼丹 / 送信 / 港口跑商 / 灵兽照料 / 抄录阵图 / 海猎采珠`。
  - 每个工作都有接取 NPC、地点、产出、门槛、成长维度和帮助文档。
- 门派与散修身份系统：
  - 散修链固定为 `行脚散修 -> 游方散人 -> 采真客 -> 洞府之主`。
  - 七玄门、黄枫谷、灵兽山统一用 `记名 -> 外门 -> 内门 -> 执事`。
  - 贡献/声望/头衔都从任务和行为积累，师门传授、配方、场景权限、称号都受其约束。
- 排行、频道、留言板：
  - 频道固定 `world / local / team / sect / trade / rumor / tell`。
  - 排行固定 `境界 / 财富 / 战力 / 丹道 / 游历 / 赏金 / 首席`。
  - 留言板做成房间级公共板，首批支持 `board / read / post / discard`。
- 世界事件与剧情使用方式：
  - 韩立年历、原著桥段、虚天殿等内容转成时代事件、稀有传闻、限时世界动态、手册条目、NPC 旧闻。
  - 不再把玩家强制写成“韩立本人”。

### 5. 数据与服务端组织
- `doc/mud/source/` 继续作为世界构建源，但新增三类源文件：
  - `mud_help_manual`：帮助主题、命令文案、地图说明、新手指南。
  - `mud_jobs_rumors`：公众任务池、工作定义、Rumor 源、提交规则、奖励规则。
  - `mud_titles_factions`：门派/散修阶位、称号、贡献门槛、导师与服务权限。
- 服务端继续由 `MudGameRuntime` 统一承接，不新增 BFF；重点是把已有 `StructuredPanel` 和 `GameEvent` 真正用成 MUD 成品文本输出器。
- 现有 `bootstrap -> command/execute -> feed/pull` 保持不变；`codex/list/detail` 继续保留。
- 当前 121 房间不先扩图，先按“城镇服务点 / 门派驻点 / 野外循环点 / 危险点 / 秘境入口”重标并补服务标签、Rumor 源和 ASCII 地图说明。

## Public API / Interface Changes
- `mud.proto`
  - `CommandDefinition` 增加：`aliases`, `usage`, `target_hint`, `visibility_scope`。
  - `StructuredPanel` 增加：`document_id`, `body_lines`, `panel_kind`，统一承接帮助文档、留言板帖子、ASCII 榜单和长文终端输出。
  - `SceneSnapshot` 增加：`service_tags`, `rumor_topics`, `board_available`, `mentor_ids`。
  - `PlayerSnapshot` 增加：`identity_track`, `rank_level`, `contribution_state`, `reputation_state`, `unread_board_count`。
  - `QuestSummary` 增加：`quest_kind`, `repeatable`, `issuer_hint`。
- 服务接口
  - 不新增主接口；帮助、留言板、工作、公众任务、排行、门派信息全部继续走 `POST /v1/game/command/execute`。
  - 仅在现有返回体里补充结构化字段和字符板正文。

## Test Plan
- 前端终端化回归
  - `1080x1920` 竖屏下，主界面只有 `prompt + 单终端滚屏 + 底部输入/短命令/方向盘`。
  - 聊天、查看人物、背包、帮助、排行都能在主终端中完成，不依赖高频弹窗。
  - 新号从登录到建角到进入七玄门，全流程是终端式单主屏。
- 系统玩法回归
  - `help/newbie/commands/work/freequest/rank/channels/map_<region>` 都能返回正确帮助。
  - `ask <npc> about rumor` 能在嘉元城、太南谷、天南港三类枢纽场景稳定产出公众任务线索。
  - 四条身份线 `散修 / 七玄门 / 黄枫谷 / 灵兽山` 都能完成接事、交事、升阶、拿头衔。
  - `board/read/post/discard` 在有留言板的房间可用，刷新与重连后未读状态正常。
- 内容回归
  - 当前 121 房间中，至少 60 个核心房间具备 `人物/服务/传闻/工作/资源/板子` 中至少 2 项。
  - 新玩家 90 分钟内至少能体验：1 次帮助查询、1 条公众任务、1 次工作循环、1 次聊天或留言、1 次门派或散修身份推进。
  - 当前内容仍能从七玄门一路玩到筑基初期，但推进方式变成共享世界玩法集合，而不是单线剧情服。
- 协议与恢复回归
  - `bootstrap -> command/execute -> feed/pull` 全程兼容旧账号旧角色。
  - 旧字段缺省时，前端能退回基础终端渲染，不出现白屏或空面板。
  - `npm --prefix client test -- --run`、`npm --prefix client run build`、`node scripts/build_mud_world.mjs`、`wsl .../mud_smoke` 继续作为交付基线。

## Assumptions And Defaults
- 参考基线来自 [`oiuv/mud` 仓库](https://github.com/oiuv/mud)、其 [Web 客户端](https://raw.githubusercontent.com/oiuv/mud/master/www/index.html) 与帮助体系中的 [新手](https://raw.githubusercontent.com/oiuv/mud/master/help/newbie)、[命令](https://raw.githubusercontent.com/oiuv/mud/master/help/cmds)、[特点](https://raw.githubusercontent.com/oiuv/mud/master/help/feature)、[公众任务](https://raw.githubusercontent.com/oiuv/mud/master/help/freequest)、[工作](https://raw.githubusercontent.com/oiuv/mud/master/help/work)、[留言板](https://raw.githubusercontent.com/oiuv/mud/master/help/board)；对齐的是其成品 MUD 形态，不是直接搬武侠世界或 FluffOS 驱动。
- 世界观、名词、场景、门派、道具、剧情锚点全部保留凡人修仙，不引入金庸门派和角色。
- 保持当前技术路线：`Vue3 + protobuf over HTTP + 短轮询 + manager/login/game + 同域代理`；不改 telnet/WebSocket。
- 第一阶段不再继续横向扩地图，先把现有 121 房间做成更像成熟 MUD 的共享世界生态。
- 本次是“深度对齐整套 MUD”，优先级固定为：
  - `终端主界面`
  - `帮助系统与命令体系`
  - `公众任务/工作/门派/散修生态`
  - `留言板/频道/排行`
  - `现有凡人内容重排`
