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
    job('xutian_probe', '残殿探线', '高风险机会任务', 'xutian_hall', 'palace_remnant_spirit', 'palace_remnant_spirit', '虚天殿不缺想进去的人，缺的是能活着把线索带出来的人。', {
      requirements: '肯冒险，也肯先读懂禁制提示。',
      reward_summary: '筑基前置材料、残钥线索、深层入口。',
      command_hint: 'help board / board / read 1 / inspect 守门残灵',
      route_tag: 'xutian_arc',
      service_tag: 'danger',
      related_quest_id: 'xutian_key',
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
    rumor('xutian_spirit_rumor', 'xutian_hall', 'palace_remnant_spirit', 'rumor', '虚天殿里的残灵不会白白给路，但会给肯看懂提示的人留缝。', [
      '守门残灵的声音像从铜门后慢慢渗出来：贪者止于门外，识禁者方能再前。',
      '它提醒你，这里不只是打怪取物，更要学会读板子、看残纹、认哪一步是真的在给你路。'
    ], {
      job_ids: ['xutian_probe'],
      quest_ids: ['xutian_key'],
    }),
  ],
};
