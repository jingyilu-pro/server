const job = (job_id, title, kind, scene_id, issuer_npc_id, submit_npc_id, summary, extras = {}) => ({
  job_id,
  title,
  kind,
  scene_id,
  issuer_npc_id,
  submit_npc_id,
  summary,
  description: extras.description ?? summary,
  requirements: extras.requirements ?? '',
  reward_summary: extras.reward_summary ?? '',
  command_hint: extras.command_hint ?? '',
  repeatable: extras.repeatable ?? true,
  route_tag: extras.route_tag ?? '',
  service_tag: extras.service_tag ?? '',
  related_quest_id: extras.related_quest_id ?? '',
});

const rumor = (source_id, scene_id, npc_id, topic, summary, body_lines, extras = {}) => ({
  source_id,
  scene_id,
  npc_id,
  topic,
  summary,
  body_lines,
  job_ids: extras.job_ids ?? [],
  quest_ids: extras.quest_ids ?? [],
  unlock_flags: extras.unlock_flags ?? [],
});

export default {
  jobs: [
    job('qixuan_runner', '外场跑腿', '工作任务', 'qixuan_square', 'li_feiyu', 'li_feiyu', '先替外场弟子跑几趟腿，熟悉山门和嘉元城之间的脚程。', {
      requirements: '会看路、会找人、会交东西。',
      reward_summary: '少量灵石、江湖人情、七玄门熟路。',
      command_hint: 'talk 厉飞雨 / go east / journal',
      route_tag: 'qixuan_gate',
      service_tag: 'mentor',
      related_quest_id: 'backslope_wolf_skin',
    }),
    job('market_errand', '坊市采办', '公众任务', 'jiayuan_market', 'mofu_steward', 'mofu_steward', '嘉元城集市里最不缺的就是跑腿、采买和顺手捎消息的人。', {
      requirements: '认得摊位、认得药材、认得谁说话算数。',
      reward_summary: '灵石、基础药材、集市门路。',
      command_hint: 'ask 墨府总管 about rumor / board / buy',
      route_tag: 'loose_cultivator',
      service_tag: 'board',
      related_quest_id: 'qixuan_herb',
    }),
    job('fair_copying', '小会抄录', '工作任务', 'tainan_fair', 'old_book_peddler', 'old_book_peddler', '太南小会里总有人愿花钱买一份抄录得清楚的消息。', {
      requirements: '肯听、肯记、肯来回跑。',
      reward_summary: '灵石、散修声望、更多风声入口。',
      command_hint: 'ask 旧书摊 about rumor / work / journal',
      route_tag: 'loose_cultivator',
      service_tag: 'rumor',
      related_quest_id: 'fair_rumor_packet',
    }),
    job('huangfeng_patrol', '外营巡查', '门派事务', 'huangfeng_outpost', 'huangfeng_steward', 'huangfeng_steward', '黄枫谷外营最看重稳字，巡查、送信、验货都算事务。', {
      requirements: '先把差事办稳，再谈更深的宗门机缘。',
      reward_summary: '贡献、外营信任、药园与藏经线入口。',
      command_hint: 'duty / talk 黄枫谷执事 / submit huangfeng_letter',
      route_tag: 'huangfeng_valley',
      service_tag: 'sect',
      related_quest_id: 'huangfeng_letter',
    }),
    job('spirit_feed_duty', '兽栏照料', '门派事务', 'spirit_beast_outer_gate', 'spirit_beast_steward', 'spirit_beast_steward', '灵兽山最基本也最实在的活，就是先把灵兽和虫圃照看好。', {
      requirements: '会送草料、会看灵兽脾气、愿意重复做。',
      reward_summary: '贡献、驭兽好感、灵虫与采药线入口。',
      command_hint: 'work / duty / talk 灵兽山管事',
      route_tag: 'spirit_beast_mountain',
      service_tag: 'sect',
      related_quest_id: 'spirit_feed_task',
    }),
    job('harbor_portage', '港口跑货', '工作任务', 'tiannan_harbor', 'seaport_broker', 'seaport_broker', '港口跑货最讲究认人认船认潮水，做熟了就是一条稳收入线。', {
      requirements: '能跑港区、认海货、不怕和船家打交道。',
      reward_summary: '灵石、海路门路、远航前置信用。',
      command_hint: 'ask 海商牙人 about rumor / work / travel',
      route_tag: 'chaos_sea_route',
      service_tag: 'trade',
      related_quest_id: 'chaos_sea_chart',
    }),
    job('backbay_shellwork', '后湾摸壳', '工作任务', 'harbor_backbay', 'backbay_fisher_wu', 'backbay_fisher_wu', '后湾潮线是海猎新手最稳的练手区，摸壳、拣藻、看潮都算活。', {
      requirements: '懂得见好就收，别一头扎进暗流里。',
      reward_summary: '海材、后湾声望、采珠入门。',
      command_hint: 'work / harvest / submit harbor_shell_task',
      route_tag: 'chaos_sea_route',
      service_tag: 'sea',
      related_quest_id: 'harbor_shell_task',
    }),
    job('sea_hunt_contract', '近海海猎', '工作任务', 'chaos_sea_port', 'captain_qu', 'captain_qu', '近港和群岛之间总有人缺会下水、会看潮、会收货的人。', {
      requirements: '有一点战斗力，也有一点收手的分寸。',
      reward_summary: '海材、灵石、远航好感。',
      command_hint: 'work / wanted / travel',
      route_tag: 'chaos_sea_route',
      service_tag: 'sea',
      related_quest_id: 'captain_supply',
    }),
    job('outer_sea_watch', '外海探潮', '高风险机会任务', 'chaos_sea_port', 'captain_qu', 'captain_qu', '曲船主开始让真正能活着回来的修士去外海中层认潮、认压、认退路。', {
      requirements: '最好已走通乱星海与虚天殿前置，不然多半连第一轮外海灵压都撑不过。',
      reward_summary: '结丹线索、外海主材、深海见闻。',
      command_hint: 'ask 曲船主 about rumor / travel / board',
      route_tag: 'outer_sea',
      service_tag: 'gold_core',
      related_quest_id: 'outer_sea_trail',
    }),
    job('xutian_probe', '残殿探线', '高风险机会任务', 'xutian_hall', 'palace_remnant_spirit', 'palace_remnant_spirit', '虚天殿不缺想进去的人，缺的是能活着把线索带出来的人。', {
      requirements: '肯冒险，也肯先读懂禁制提示。',
      reward_summary: '筑基前置材料、残钥线索、深层入口。',
      command_hint: 'help board / board / read 1 / inspect 守门残灵',
      route_tag: 'xutian_arc',
      service_tag: 'danger',
      related_quest_id: 'xutian_key',
    }),
    job('core_flame_watch', '残环探火', '高风险机会任务', 'xutian_hall', 'palace_remnant_spirit', 'palace_remnant_spirit', '守门残灵开始把后来人往残环与丹火灵脉更深处引，像是在挑还敢不敢继续赌命的人。', {
      requirements: '最好已摸清虚天残区的基本节律，否则很容易在古禁深处先乱了神。',
      reward_summary: '结丹辅材、古修见闻、残环门径。',
      command_hint: 'ask 守门残灵 about rumor / board / travel',
      route_tag: 'xutian_arc',
      service_tag: 'gold_core',
      related_quest_id: 'core_ruin_heart',
    }),
    job('star_abyss_collect', '星渊采心', '高风险机会任务', 'xutian_star_platform', 'star_tablet_spirit', 'star_tablet_spirit', '祭台残灵开始让人把路走到星渊潮眼，只为取那一枚真正够格的凝婴灵物。', {
      requirements: '结丹后段的修士才会认真考虑这条差事，太早去多半只会把命交给深海。',
      reward_summary: '凝婴线索、深海灵物、关键见闻。',
      command_hint: 'ask 祭台残灵 about rumor / travel / board',
      route_tag: 'star_abyss',
      service_tag: 'nascent_soul',
      related_quest_id: 'nascent_soul_gate',
    }),
    job('loose_market_shift', '棚市记账', '工作任务', 'loose_camp_square', 'loose_master_wen', 'guest_scribe_fan', '散修棚市最缺的不是热闹，而是有人肯把货、话和来路去路记明白。', {
      requirements: '识人、认货、肯跑腿。',
      reward_summary: '灵石、散修声望、棚市门路。',
      command_hint: 'ask 温散人 about rumor / board / journal',
      route_tag: 'loose_cultivator',
      service_tag: 'board',
      related_quest_id: 'loose_rumor_task',
    }),
    job('loose_herb_sorting', '药帐分拣', '工作任务', 'loose_medicine_tent', 'herb_tutor_qing', 'herb_tutor_qing', '药帐里永远缺一个肯把便宜药草也分清的人。', {
      requirements: '认草药、手稳、耐心足。',
      reward_summary: '药材、炼药熟路、散修人情。',
      command_hint: 'ask 青药师 about rumor / work / journal',
      route_tag: 'loose_cultivator',
      service_tag: 'gather',
    }),
    job('east_gate_notice_run', '东门传话', '公众任务', 'jiayuan_east_gate', 'gate_guard_lu', 'gate_guard_lu', '城门口的差事多半不起眼，却最能试出一个人会不会把口风和脚程都收住。', {
      requirements: '认得城里几条路，也知道哪些话不能在街心乱说。',
      reward_summary: '灵石、城门熟路、嘉元城口碑。',
      command_hint: 'ask 陆门吏 about rumor / travel / journal',
      route_tag: 'loose_cultivator',
      service_tag: 'rumor',
    }),
    job('mofu_guest_registry', '府前记名', '公众任务', 'mofu_front_hall', 'mofu_elder_housekeeper', 'mofu_elder_housekeeper', '墨府前厅最缺的不是客人，而是能把来客、令牌和口信都记清的人。', {
      requirements: '眼明手快，嘴严心稳。',
      reward_summary: '灵石、墨府人情、府内差遣入口。',
      command_hint: 'ask 府中老管事 about rumor / board / journal',
      route_tag: 'loose_cultivator',
      service_tag: 'board',
      related_quest_id: 'mofu_guest_token',
    }),
    job('tainan_gate_lead', '谷口引路', '公众任务', 'tainan_gate', 'wandering_broker', 'wandering_broker', '太南谷口每天都有人来问货、问人、问路，肯替人把线头理顺便是一门活。', {
      requirements: '认得摊位、会听风向、敢往血禁边上追半步。',
      reward_summary: '灵石、散修声望、太南消息口子。',
      command_hint: 'ask 流动牙人 about rumor / board / travel',
      route_tag: 'loose_cultivator',
      service_tag: 'rumor',
      related_quest_id: 'ruins_old_map',
    }),
    job('huangfeng_registry_copy', '内堂录名', '门派事务', 'huangfeng_hall', 'elder_ma', 'disciple_lu', '黄枫谷偏殿最看重的不是嘴上恭顺，而是能不能把交到案头的事录得清、办得稳。', {
      requirements: '守规矩、认名录、肯把杂务先做扎实。',
      reward_summary: '贡献、名录熟路、内堂信任。',
      command_hint: 'ask 马师伯 about rumor / duty / journal',
      route_tag: 'huangfeng_valley',
      service_tag: 'sect',
    }),
    job('medicine_terrace_round', '药台巡材', '门派事务', 'huangfeng_medicine_terrace', 'medicine_deacon', 'medicine_deacon', '药梯台每日都要点药、验苗、分辨哪些东西能入炉、哪些只能先晾着。', {
      requirements: '认药材、肯吃细活、知道轻重缓急。',
      reward_summary: '贡献、药材、药台门路。',
      command_hint: 'ask 药台执事 about rumor / work / journal',
      route_tag: 'huangfeng_valley',
      service_tag: 'alchemy',
      related_quest_id: 'medicine_moss',
    }),
    job('scripture_rubbing', '经廊抄卷', '门派事务', 'huangfeng_scripture', 'scripture_keeper', 'scripture_keeper', '藏经石廊最缺的不是会背口诀的人，而是肯把一页页旧卷慢慢抄到手上的人。', {
      requirements: '心稳、手稳、愿意从最浅的口诀开始磨。',
      reward_summary: '贡献、手册见闻、经廊信任。',
      command_hint: 'ask 守卷人 about rumor / duty / journal',
      route_tag: 'huangfeng_valley',
      service_tag: 'mentor',
      related_quest_id: 'huangfeng_manual',
    }),
    job('beast_pen_feed_round', '兽栏巡喂', '门派事务', 'spirit_beast_beast_pen', 'beast_feeder_zhou', 'beast_feeder_zhou', '灵兽山最基本也最磨人的活，就是把一圈圈兽栏按时喂稳、看稳、收拾稳。', {
      requirements: '不怕脏、不怕麻烦、懂得先把灵兽脾气摸清。',
      reward_summary: '贡献、饲养熟路、兽栏信任。',
      command_hint: 'ask 周饲兽 about rumor / work / journal',
      route_tag: 'spirit_beast_mountain',
      service_tag: 'sect',
    }),
    job('insect_garden_mist', '虫圃换饵', '门派事务', 'spirit_beast_insect_garden', 'insect_master_qin', 'insect_master_qin', '虫圃里最怕的不是虫凶，而是饵丸、湿度和气味哪一样先乱了。', {
      requirements: '胆大心细，肯照规矩试错。',
      reward_summary: '贡献、虫材、灵虫门路。',
      command_hint: 'ask 秦虫师 about rumor / work / journal',
      route_tag: 'spirit_beast_mountain',
      service_tag: 'gather',
    }),
    job('outer_hall_ledger', '外山点事', '门派事务', 'spirit_beast_hall', 'outer_deacon_du', 'outer_deacon_du', '外山执事堂里从不缺杂务，缺的是能把兽材、差遣和人手全都拢顺的人。', {
      requirements: '扛事、守时、知道先后次序。',
      reward_summary: '贡献、执事眼缘、外山权限。',
      command_hint: 'ask 杜执事 about rumor / duty / journal',
      route_tag: 'spirit_beast_mountain',
      service_tag: 'sect',
    }),
    job('tiannan_chart_copy', '坊市誊图', '工作任务', 'tiannan_market', 'chart_seller', 'chart_seller', '海商坊市里最值钱的常不是货，而是那张你抄得够不够准的旧海图。', {
      requirements: '识潮线、认旧字、愿意替人把图与口信都补齐。',
      reward_summary: '灵石、航线见闻、坊市门路。',
      command_hint: 'ask 卖图客 about rumor / board / journal',
      route_tag: 'chaos_sea_route',
      service_tag: 'trade',
    }),
    job('dock_rigging_shift', '码头紧索', '工作任务', 'tiannan_dock', 'old_shipwright', 'old_shipwright', '远航码头不怕活重，只怕有人在阵索和风骨上偷懒。', {
      requirements: '手脚利落，听得懂船匠指挥。',
      reward_summary: '灵石、远航信用、海路门径。',
      command_hint: 'ask 老船匠 about rumor / work / travel',
      route_tag: 'chaos_sea_route',
      service_tag: 'travel',
      related_quest_id: 'captain_supply',
    }),
    job('salt_house_packing', '盐棚拣货', '工作任务', 'harbor_salt_house', 'salt_house_keeper_lin', 'salt_house_keeper_lin', '晒盐棚里最稳的活从来不是抢货，而是把盐壳、海藻和能卖钱的东西一层层拣明白。', {
      requirements: '手快眼明，懂得什么能留、什么该卖。',
      reward_summary: '海材、灵石、后湾熟路。',
      command_hint: 'ask 林盐婆 about rumor / work / journal',
      route_tag: 'chaos_sea_route',
      service_tag: 'trade',
    }),
    job('net_field_sorting', '晒网分潮', '工作任务', 'harbor_net_field', 'net_master_peng', 'net_master_peng', '晒网场的活讲究快，也讲究你会不会从一堆网脚和海屑里先挑出值钱的那一点。', {
      requirements: '不怕湿、不怕乱、肯反复练手。',
      reward_summary: '海材、后湾口碑、近海图线索。',
      command_hint: 'ask 彭网师 about rumor / work / journal',
      route_tag: 'chaos_sea_route',
      service_tag: 'sea',
      related_quest_id: 'harbor_chart_task',
    }),
    job('isles_manifest_run', '群岛清舱', '工作任务', 'outer_isles_wharf', 'island_broker_shi', 'island_broker_shi', '群岛小埠的活不算凶险，却最考验你会不会把人、货和下一段航线都理顺。', {
      requirements: '认船、认货、肯替人跑埠头。',
      reward_summary: '灵石、群岛航线熟路、海商信任。',
      command_hint: 'ask 施岛牙 about rumor / board / travel',
      route_tag: 'chaos_sea_route',
      service_tag: 'travel',
      related_quest_id: 'outer_pearl_task',
    }),
    job('pearl_market_grading', '珠市拣成色', '工作任务', 'outer_isles_market', 'pearl_diver_lan', 'pearl_diver_lan', '珠市棚里最不缺珠壳，最缺的是肯把成色、药用和去处都分出来的人。', {
      requirements: '眼准、心稳、肯把潮汐时机记熟。',
      reward_summary: '海材、灵石、采珠门路。',
      command_hint: 'ask 蓝采珠 about rumor / board / journal',
      route_tag: 'chaos_sea_route',
      service_tag: 'sea',
      related_quest_id: 'outer_coral_task',
    }),
    job('outer_sea_sounding', '外海测潮', '高风险机会任务', 'outer_sea_mid', 'deck_mage', 'deck_mage', '真正的结丹准备不止靠丹方，也得有人敢在外海中层把潮压、灵压和退路一并看回来。', {
      requirements: '最好已走通近港和群岛海线，否则连第一轮灵压都未必扛得住。',
      reward_summary: '结丹见闻、外海主材、丹火线索。',
      command_hint: 'wanted',
      route_tag: 'outer_sea',
      service_tag: 'gold_core',
      related_quest_id: 'gold_core_gate',
    }),
    job('core_flame_sifting', '丹火筛脉', '高风险机会任务', 'core_flame_vein', 'rift_record_spirit', 'rift_record_spirit', '丹火灵脉不是拿到什么就算什么，真正值钱的是你能不能把灵脉里那一缕够稳的火意筛出来。', {
      requirements: '要扛得住地火，也要看得懂残纹与火脉如何互相咬合。',
      reward_summary: '结丹辅材、古修见闻、残环门径。',
      command_hint: 'harvest',
      route_tag: 'xutian_arc',
      service_tag: 'gold_core',
      related_quest_id: 'core_ruin_heart',
    }),
    job('ruin_ring_rubbing', '残环拓纹', '高风险机会任务', 'ancient_ruin_ring', 'wall_listener_qiu', 'wall_listener_qiu', '古修残环里真正带得出去的往往不是整件宝，而是那几段能让后人继续往下走的残纹。', {
      requirements: '会读残纹，肯在危险里停下来慢慢看。',
      reward_summary: '残环线索、古禁见闻、结丹门槛提示。',
      command_hint: 'board',
      route_tag: 'xutian_arc',
      service_tag: 'danger',
      related_quest_id: 'core_ruin_heart',
    }),
    job('star_abyss_tidewatch', '星渊候潮', '高风险机会任务', 'star_abyss', 'star_tablet_spirit', 'star_tablet_spirit', '走到星渊潮眼后，真正困难的不是进去，而是等到那一口能让灵物露面的潮时。', {
      requirements: '结丹后段只是门槛，更要有足够见闻和耐性。',
      reward_summary: '凝婴线索、深海灵物、潮眼门径。',
      command_hint: 'travel',
      route_tag: 'star_abyss',
      service_tag: 'nascent_soul',
      related_quest_id: 'nascent_soul_gate',
    }),
  ],
  rumor_sources: [
    rumor('qixuan_li_rumor', 'qixuan_square', 'li_feiyu', 'rumor', '厉飞雨知道哪些人只是嘴上说想修行，哪些人是真的敢把脚迈出去。', [
      '厉飞雨低声道：先别急着想着拜山入谷，先把山门外这一圈腿跑熟，才知道自己算不算真能吃这口饭。',
      '他说得很直白：嘉元城那边常有差事，办稳了，七玄门里看你的人自然会多。'
    ], {
      job_ids: ['qixuan_runner'],
      quest_ids: ['backslope_wolf_skin'],
    }),
    rumor('jiayuan_market_rumor', 'jiayuan_market', 'mofu_steward', 'rumor', '嘉元城的风声从来不只在嘴上，也在货、在人情和谁先跑到位里。', [
      '墨府总管抬眼道：会跑腿的人到处都有，可会把药、信、消息都送准的人不多。',
      '他指了指集市深处：你若想在城里混熟，先把眼前这几桩小事办利索。'
    ], {
      job_ids: ['market_errand'],
      quest_ids: ['qixuan_herb', 'mofu_guest_token'],
    }),
    rumor('tainan_book_rumor', 'tainan_fair', 'old_book_peddler', 'rumor', '太南小会最值钱的不是货，而是谁先把一条风声记牢。', [
      '旧书摊贩咳了一声：小会里人人都说自己消息灵，可真正值钱的，是你肯不肯把零碎话拼成一条路。',
      '他说若你愿意帮他抄、帮他记，他自然愿让你更早看到下一层风向。'
    ], {
      job_ids: ['fair_copying'],
      quest_ids: ['fair_rumor_packet'],
    }),
    rumor('tainan_xin_rumor', 'xin_house', 'xin_ruyin', '阵法风声', '辛如音愿意讲阵法，但只对肯做事的人讲。', [
      '辛如音轻声道：阵法不是听几句就会的，你若愿意替我找材料、送阵旗，我自然会把该讲的口子打开。',
      '她提到太南谷外与血禁边上最近都不太安稳，阵禁和异物的需求都在变。'
    ], {
      quest_ids: ['tainan_snake', 'tainan_array_flag'],
    }),
    rumor('huangfeng_steward_rumor', 'huangfeng_outpost', 'huangfeng_steward', 'rumor', '黄枫谷外营的路，不是靠嘴说进去的。', [
      '黄枫谷执事淡淡道：外营先看你办事稳不稳，再看你心性定不定。',
      '若想继续往药台和石廊走，先把外营手头的差事做干净。'
    ], {
      job_ids: ['huangfeng_patrol'],
      quest_ids: ['huangfeng_letter', 'medicine_moss'],
    }),
    rumor('spirit_steward_rumor', 'spirit_beast_outer_gate', 'spirit_beast_steward', 'rumor', '灵兽山外门先看你会不会照看活物，再决定值不值得教你更深的法子。', [
      '灵兽山管事道：灵兽、灵虫、药圃都是活东西，心一急就全乱。',
      '想在山里站稳，先从最细碎的照料活做起。'
    ], {
      job_ids: ['spirit_feed_duty'],
      quest_ids: ['spirit_feed_task', 'spirit_bug_task'],
    }),
    rumor('tiannan_broker_rumor', 'tiannan_harbor', 'seaport_broker', 'rumor', '天南港的真正门路，不在码头喧闹处，而在谁肯把活先做出来。', [
      '海商牙人压低声音：想上远船，先把港口这几条小线跑熟，别让人觉得你连潮水都认不清。',
      '他说近来乱星海风向古怪，能提前一步补齐物资的人，总能多活一程。'
    ], {
      job_ids: ['harbor_portage'],
      quest_ids: ['chaos_sea_chart', 'ruins_old_map'],
    }),
    rumor('backbay_wu_rumor', 'harbor_backbay', 'backbay_fisher_wu', 'rumor', '后湾是新手海猎的试手地，也是最容易让人轻敌的地方。', [
      '吴老渔道：后湾看着不深，可暗流、礁缝、壳妖都在等人粗心那一下。',
      '先从摸壳、拣藻和看潮练起，别急着把自己当老海手。'
    ], {
      job_ids: ['backbay_shellwork'],
      quest_ids: ['harbor_shell_task'],
    }),
    rumor('chaos_captain_rumor', 'chaos_sea_port', 'captain_qu', 'rumor', '曲船主对谁能在海上活下来这件事，看得比谁都明白。', [
      '曲船主只说了一句：近海不教人长本事，只教人先别死。',
      '若你真想往更深处去，就先把近港、群岛和补给活干稳。'
    ], {
      job_ids: ['sea_hunt_contract'],
      quest_ids: ['captain_supply', 'demon_fish_core'],
    }),
    rumor('outer_sea_rumor', 'chaos_sea_port', 'captain_qu', '结丹', '曲船主近来谈得更多的已不是近海生意，而是谁敢真正把船压进外海中层。', [
      '曲船主望着更深处的海色：到了外海，胆子、法力和退路都得一起备齐，缺一样都不够你活着回来。',
      '他提到最近有人在外海中层见到青焰般的潮下碎光，那东西正适合拿来稳住丹火。'
    ], {
      job_ids: ['outer_sea_watch'],
      quest_ids: ['outer_sea_trail', 'gold_core_gate'],
    }),
    rumor('xutian_spirit_rumor', 'xutian_hall', 'palace_remnant_spirit', 'rumor', '虚天殿里的残灵不会白白给路，但会给肯看懂提示的人留缝。', [
      '守门残灵的声音像从铜门后慢慢渗出来：贪者止于门外，识禁者方能再前。',
      '它提醒你，这里不只是打怪取物，更要学会读板子、看残纹、认哪一步是真的在给你路。'
    ], {
      job_ids: ['xutian_probe'],
      quest_ids: ['xutian_key'],
    }),
    rumor('core_flame_rumor', 'xutian_hall', 'palace_remnant_spirit', '古修残环', '守门残灵开始反复提起残环与丹火灵脉，像是默认你已经能往更深处走了。', [
      '守门残灵缓缓道：外殿之火只够照路，真正会试人心性的，是残环里那口还没散尽的古修余温。',
      '它提醒你，若手里仍握着旧库令牌，不妨试着把它带去残环深处，看古禁到底认不认你。'
    ], {
      job_ids: ['core_flame_watch'],
      quest_ids: ['core_ruin_heart'],
    }),
    rumor('star_abyss_rumor', 'xutian_star_platform', 'star_tablet_spirit', '凝婴', '祭台残灵近来把话题越提越远，已经不再只谈玄门，而是在谈更深海里的那口婴火。', [
      '祭台残灵低语：凝婴所缺的从不是一味丹，而是你有没有走到那一步、见到那一步、护住那一步。',
      '它提到星渊潮眼里偶有灵物流成心形，真正敢下去的人，通常已不再把结丹当终点。'
    ], {
      job_ids: ['star_abyss_collect'],
      quest_ids: ['nascent_soul_gate'],
    }),
    rumor('loose_master_wen_rumor', 'loose_camp_square', 'loose_master_wen', 'rumor', '温散人最懂散修起步最怕什么，也最懂第一条稳营生该从哪儿开。', [
      '温散人吹了吹茶面：散修若连棚市里谁在收货、谁在借宿、谁只会卖空话都分不清，出门再远也是白走。',
      '他让你先把棚市账和口风理顺，再谈往太南和海港那边追更大的门路。'
    ], {
      job_ids: ['loose_market_shift'],
      quest_ids: ['loose_rumor_task'],
    }),
    rumor('loose_herb_tutor_rumor', 'loose_medicine_tent', 'herb_tutor_qing', 'rumor', '青药师最看重的不是你手里药贵不贵，而是你肯不肯把便宜草木先认扎实。', [
      '青药师掀开药锅：散修手里常没有好东西，所以更得学会把每一味便宜药草都用对地方。',
      '她提醒你，药帐和棚市之间总有细活可做，认药的人比会吹嘘的人值钱得多。'
    ], {
      job_ids: ['loose_herb_sorting'],
      quest_ids: ['loose_stone_task'],
    }),
    rumor('loose_diviner_rumor', 'loose_diviner_mat', 'diviner_meng', '旧路', '孟卜师爱讲的从不是神神叨叨，而是哪条旧路仍有人能走通。', [
      '孟卜师拨了拨草签：散修最难的不是没有宗门，是常常不知道下一步该往哪边押。',
      '他提起太南谷口、嘉元东门和棚市借宿簿，说真正能换成活路的消息通常都藏在这些最不起眼的地方。'
    ], {
      job_ids: ['loose_market_shift', 'tainan_gate_lead'],
    }),
    rumor('guest_hall_rumor', 'loose_guest_hall', 'guest_scribe_fan', 'rumor', '借宿大棚每天都有人来有人走，范记名见过太多半路起势和半路折掉的人。', [
      '范记名边翻簿边道：真正能越走越远的人，通常都懂得先在借宿、记账和口信这些小事上把人情攒起来。',
      '他说棚里最近往天南港跑的人多了，群岛和后湾那边的门路正在慢慢热起来。'
    ], {
      job_ids: ['loose_market_shift'],
    }),
    rumor('east_gate_guard_rumor', 'jiayuan_east_gate', 'gate_guard_lu', 'rumor', '陆门吏站在城门口久了，最知道哪些话是风声，哪些话其实已经在往差事上落。', [
      '陆门吏压低声音：城里现在最怕的不是闹事，是有人把该传的话传错了地方。',
      '他让你先把东门、集市和墨府这三点跑熟，再谈自己有没有资格往更深处搅进去。'
    ], {
      job_ids: ['east_gate_notice_run', 'mofu_guest_registry'],
      quest_ids: ['qixuan_herb', 'mofu_guest_token'],
    }),
    rumor('mofu_registry_rumor', 'mofu_front_hall', 'mofu_elder_housekeeper', 'rumor', '府中老管事这些天最怕的不是客多，而是来客令、旧账和风声哪一样先乱。', [
      '老管事揉了揉眉心：墨府这阵子差事一桩接一桩，真肯替人把名录、口信和令牌都拢顺的人太少。',
      '他暗示你，若能把前厅这一步办稳，内院和城里的人自然会把更多门径漏给你。'
    ], {
      job_ids: ['mofu_guest_registry'],
      quest_ids: ['mofu_guest_token'],
    }),
    rumor('tainan_gate_broker_rumor', 'tainan_gate', 'wandering_broker', 'rumor', '流动牙人最喜欢看新人站在谷口发愣，因为那正说明你还没学会把风声当路。', [
      '流动牙人笑得很淡：谷口不缺货，不缺人，缺的是有人能先把哪条线值得追、哪条线只是吵闹分出来。',
      '他提起嘉元城旧账和血禁旧图，说真正值钱的路从来都是一段一段被人问出来的。'
    ], {
      job_ids: ['tainan_gate_lead'],
      quest_ids: ['ruins_old_map'],
    }),
    rumor('elder_ma_rumor', 'huangfeng_hall', 'elder_ma', 'rumor', '马师伯肯开口时，通常不是在讲大道理，而是在提醒你哪一步根基最不能省。', [
      '马师伯缓声道：黄枫谷里真正能让人走远的，多半都藏在内堂录名、药台巡材和旧卷抄写这些最慢的事里。',
      '他说若你连这些都嫌琐碎，后面无论是血禁还是结丹门槛，多半都会先嫌你心浮。'
    ], {
      job_ids: ['huangfeng_registry_copy', 'medicine_terrace_round', 'scripture_rubbing'],
      quest_ids: ['medicine_moss', 'huangfeng_manual'],
    }),
    rumor('scripture_keeper_rumor', 'huangfeng_scripture', 'scripture_keeper', '口诀和抄卷在守卷人眼里从不是杂活，而是照人心性的法子。', [
      '守卷人抬手抚过旧刻：许多人想一口气学会更高明的东西，却不肯把最浅的一句先抄稳。',
      '他提醒你，经廊抄卷、药台点材和内堂录名其实是一条线，都是在教人怎么把根基按次序搭起来。'
    ], {
      job_ids: ['scripture_rubbing', 'huangfeng_registry_copy'],
      quest_ids: ['huangfeng_manual'],
    }),
    rumor('beast_feeder_rumor', 'spirit_beast_beast_pen', 'beast_feeder_zhou', 'rumor', '周饲兽看过太多人把驭兽想成威风事，却忘了最先要学的是喂稳、照稳、收拾稳。', [
      '周饲兽拍着栏杆：灵兽不认你说了什么，只认你今天有没有把草料、药丸和脾气都照看明白。',
      '他提起虫圃和外山执事堂，说真正能往内山走的人，往往先是把这些最脏最细的活扛住了。'
    ], {
      job_ids: ['beast_pen_feed_round', 'insect_garden_mist', 'outer_hall_ledger'],
      quest_ids: ['spirit_feed_task', 'spirit_bug_task'],
    }),
    rumor('insect_master_rumor', 'spirit_beast_insect_garden', 'insect_master_qin', 'rumor', '秦虫师从不把灵虫当小玩意，在她眼里，能不能照料这种细物最见真本事。', [
      '秦虫师皱着眉：虫圃里怕的不是虫多，是你看不出哪一丝湿热、哪一味饵丸已经在往错处走。',
      '她让你先学会换饵、稳气味，再谈更深的采材和驭使。'
    ], {
      job_ids: ['insect_garden_mist'],
      quest_ids: ['spirit_bug_task'],
    }),
    rumor('outer_deacon_rumor', 'spirit_beast_hall', 'outer_deacon_du', 'rumor', '杜执事最清楚外山哪种人是会留下来的，哪种人只是图一时新鲜。', [
      '杜执事翻着册子道：兽栏、虫圃、饲草岭这些地方看着不起眼，可谁真能把事扛下来，我一眼就看得出来。',
      '他点明外山点事不是跑腿，而是给愿意往上走的人一块块垫脚石。'
    ], {
      job_ids: ['outer_hall_ledger', 'beast_pen_feed_round'],
      quest_ids: ['spirit_feed_task'],
    }),
    rumor('chart_seller_rumor', 'tiannan_market', 'chart_seller', 'rumor', '卖图客从不觉得图最值钱，他觉得值钱的是你会不会把图后头那一串人和货也一并看懂。', [
      '卖图客压着嗓子：海商坊市每天都有人来买残图，可真能把图誊准、把口风问全的人少得很。',
      '他说若你肯先做坊市誊图这类慢活，远航码头和群岛小埠那边自然会慢慢认你。'
    ], {
      job_ids: ['tiannan_chart_copy'],
    }),
    rumor('old_shipwright_rumor', 'tiannan_dock', 'old_shipwright', 'rumor', '老船匠口中的远航，从来不只是上船，而是把绳索、阵灯、补给和胆气一并备齐。', [
      '老船匠摸着船板：船骨差一寸，海上就能让你赔一条命；人心若差一寸，也是一样。',
      '他让你先在码头把紧索和补给活做稳，再谈群岛、外海和更深的旧路。'
    ], {
      job_ids: ['dock_rigging_shift', 'isles_manifest_run'],
      quest_ids: ['captain_supply'],
    }),
    rumor('salt_house_rumor', 'harbor_salt_house', 'salt_house_keeper_lin', 'rumor', '林盐婆知道后湾看似最不起眼的东西，往往正是药师和海商最舍不得断的货。', [
      '林盐婆把盐壳拨成一堆：后湾真正能养人的，不只是猎得着东西，而是能把盐壳、海藻和壳粉都卖对人。',
      '她让你先把晒盐棚和晒网场这些小活做熟，群岛珠市那边自然会认你是会过日子的。'
    ], {
      job_ids: ['salt_house_packing', 'net_field_sorting'],
    }),
    rumor('net_master_rumor', 'harbor_net_field', 'net_master_peng', 'rumor', '彭网师看海货不像看战利品，更像在看一张会不会断档的活路。', [
      '彭网师边补网边说：晒网场的活看着零碎，其实最能练人，慢一拍就少一份货，快过头又容易全毁。',
      '他提起后湾灯塔和群岛小埠，说真正的海路都是从这些看似杂乱的地方一段段拼出来的。'
    ], {
      job_ids: ['net_field_sorting', 'isles_manifest_run'],
      quest_ids: ['harbor_chart_task'],
    }),
    rumor('lamp_guard_rumor', 'harbor_lamp_tower', 'lamp_guard_xie', '风向', '谢灯守不卖货，只卖比货更值钱的东西：什么时候该出湾，什么时候该缩手。', [
      '谢灯守盯着灯焰：海上很多人不是死在浪里，是死在自以为再赶一程也无妨。',
      '他说若你学会看灯、看潮、看云，群岛和外海会比别人多给你一条退路。'
    ], {
      job_ids: ['salt_house_packing', 'isles_manifest_run'],
    }),
    rumor('island_broker_rumor', 'outer_isles_wharf', 'island_broker_shi', 'rumor', '施岛牙最会替人把群岛航线拆成一段段可活着走完的小路。', [
      '施岛牙笑道：群岛不大，可哪条小埠能补货、哪条礁路会翻脸、哪条船愿意带你，全是门路。',
      '他让你先替埠头把清舱和点货做稳，再慢慢往珠市和黑礁那边伸手。'
    ], {
      job_ids: ['isles_manifest_run', 'pearl_market_grading'],
      quest_ids: ['outer_pearl_task'],
    }),
    rumor('pearl_diver_rumor', 'outer_isles_market', 'pearl_diver_lan', 'rumor', '蓝采珠知道近海最会骗人的是哪层浪，也知道哪一颗珠子够格被留下。', [
      '蓝采珠轻轻转着珠壳：近海采珠最怕心急，成色差半分，去处就会差一截。',
      '她提到听潮坛和黑礁外缘，说真正的群岛门路从来不是一把捞出来的，是一轮轮候潮磨出来的。'
    ], {
      job_ids: ['pearl_market_grading'],
      quest_ids: ['outer_coral_task'],
    }),
    rumor('deck_mage_rumor', 'chaos_sea_ship', 'deck_mage', '结丹门径', '甲板术士近来谈得最多的已不是近港法阵，而是外海中层那股能稳住丹火的潮压。', [
      '甲板术士把阵灯调得更亮：想碰结丹门槛，先得学会在外海灵压里不乱手脚，丹火与退路都得一起算。',
      '他说若你真敢往外海中层再走一步，就别只想着拿东西，先把潮势、压感和能不能回来都看清。'
    ], {
      job_ids: ['outer_sea_sounding'],
      quest_ids: ['gold_core_gate'],
    }),
    rumor('rift_record_rumor', 'xutian_rune_garden', 'rift_record_spirit', '残纹丹火', '记纹残灵对后来人最感兴趣的，不是你带了什么，而是你能不能把丹火与残纹看成同一件事。', [
      '记纹残灵空声道：地火自会烧人，残纹自会迷人，真正够格的人得先看出它们为什么还会彼此照应。',
      '它暗示丹火灵脉和古修残环其实是一前一后的两道门，少看懂一边都进不深。'
    ], {
      job_ids: ['core_flame_sifting', 'ruin_ring_rubbing'],
      quest_ids: ['core_ruin_heart'],
    }),
    rumor('wall_listener_rumor', 'xutian_endless_wall', 'wall_listener_qiu', '残环回响', '听壁残识总像是在替古修残环留最后几句愿意被后来人听见的话。', [
      '听壁残识缓缓道：残环最贵的从来不是一件整宝，而是那几道仍肯回应后来人的旧纹与旧意。',
      '它提醒你，若真想从残环往星渊再挪一步，就得先学会把看见的东西带回去，而不是只把手伸过去。'
    ], {
      job_ids: ['ruin_ring_rubbing', 'star_abyss_tidewatch'],
      quest_ids: ['core_ruin_heart', 'nascent_soul_gate'],
    }),
    rumor('star_tablet_abyss_rumor', 'xutian_star_platform', 'star_tablet_spirit', '星渊潮眼', '祭台残灵如今偶尔不再只谈玄门，而是直接谈那口会决定凝婴成败的潮眼。', [
      '祭台残灵低声道：星渊潮眼给人的从不是机会，而是一次你敢不敢等、能不能护住神识的试探。',
      '它说真正要去候那一轮潮的人，通常已经不再把结丹当终点，而把每一步退路都提前算进命里。'
    ], {
      job_ids: ['star_abyss_tidewatch', 'star_abyss_collect'],
      quest_ids: ['nascent_soul_gate'],
    }),
  ],
};
