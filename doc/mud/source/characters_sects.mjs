const sect = (sect_id, name, rank_title, join_scene_id, join_npc_id, description, extras = {}) => ({
  sect_id,
  name,
  rank_title,
  rank_titles: extras.rank_titles ?? [rank_title],
  chief_title: extras.chief_title ?? '首席',
  join_scene_id,
  join_npc_id,
  description,
  codex_entry_id: extras.codex_entry_id ?? '',
  joinable: extras.joinable ?? true,
});

const npc = (npc_id, name, scene_id, hint, dialogue, quest_ids = [], extras = {}) => ({
  npc_id,
  name,
  scene_id,
  hint,
  dialogue,
  quest_ids,
  sect_offer_id: extras.sect_offer_id ?? '',
  role: extras.role ?? '',
  description: extras.description ?? '',
});

export default {
  sects: [
    sect('qixuan_gate', '七玄门', '记名弟子', 'qixuan_hall', 'han_zhanglao', '凡俗江湖门派，却也是许多修士的命运起点。', {
      rank_titles: ['记名弟子', '外门弟子', '内门弟子', '执事'],
      chief_title: '首席弟子',
    }),
    sect('huangfeng_valley', '黄枫谷', '记名弟子', 'huangfeng_hall', 'elder_ma', '越国七派之一，重根基、重药理，是稳健修行路线的代表。', {
      rank_titles: ['记名弟子', '外门弟子', '内门弟子', '执事'],
      chief_title: '首席弟子',
    }),
    sect('yanyue_sect', '掩月宗', '掩月宗外门弟子', 'tainan_fair', 'fairy_chen', '擅长身法与法门，行事多清冷而机敏。', {
      joinable: false,
      rank_titles: ['外门弟子', '内门弟子', '执事'],
      chief_title: '月殿首席',
    }),
    sect('giant_sword_clan', '巨剑门', '巨剑门客卿', '', '', '越国七派之一，以强攻与剑修见长。', { joinable: false }),
    sect('spirit_beast_mountain', '灵兽山', '记名弟子', 'spirit_beast_outer_gate', 'spirit_beast_steward', '专精驭兽与培育灵虫灵兽之道。', {
      joinable: true,
      rank_titles: ['记名弟子', '外山弟子', '内山弟子', '执事'],
      chief_title: '灵兽首席',
    }),
    sect('mask_sky_school', '化刀坞', '化刀坞门人', '', '', '重杀伐与兵刃祭炼，作风凌厉。', { joinable: false }),
  ],
  npcs: [
    npc('li_feiyu', '厉飞雨', 'qixuan_square', '指点你先从嘉元城差事做起，把命保住再谈修仙。', '厉飞雨抱臂而立：先去嘉元城集市跑腿、练胆，别一上来就想做天上人。', ['backslope_wolf_skin'], { role: 'mentor', description: '七玄门里最像江湖人的那一类人，嘴硬心热。' }),
    npc('zhang_tie', '张铁', 'qixuan_square', '与你一同在外门熬日子，对江湖和修仙都半懂不懂。', '张铁挠头笑道：你若真闯出门道，记得回来请我喝酒。', [], { role: 'companion', description: '质朴直接，是最典型的凡人起点同伴。' }),
    npc('han_zhanglao', '韩长老', 'qixuan_hall', '看重能扛事的人，若你做成几件差事，自会给你一条路。', '韩长老缓缓道：凡人若能把一件小事做稳，才配谈修行。', [], { role: 'sect_master', sect_offer_id: 'qixuan_gate', description: '掌管七玄门外门事务，注重心性与韧劲。' }),
    npc('sun_er', '孙二', 'qixuan_dormitory', '丢了一张要命的药方纸，正急得团团转。', '孙二低声求你：那药方若让莫大夫知道是我弄丢的，我这月杂役工钱都保不住。', ['qixuan_stream_note'], { role: 'junior', description: '常见的外门小人物，却总能把你卷进第一波杂务。' }),
    npc('doctor_mo', '墨大夫', 'qixuan_stream', '你若肯办事，他也会赏你一张药散方子。', '墨大夫淡淡道：药理不难，难的是人心与火候。', [], { role: 'doctor', description: '懂药、懂人，也懂如何让人欠下人情。' }),
    npc('field_steward', '药圃执役', 'qixuan_medicine_garden', '这里的灵草虽浅，却足够教新手认药。', '药圃执役提醒你：别踩坏苗圃，不然先赔上半月杂役钱。', [], { role: 'gathering', description: '负责照料药圃，也负责提醒新人别乱来。' }),

    npc('gate_guard_lu', '陆门吏', 'jiayuan_east_gate', '熟门熟路，知道嘉元城近来都在议论什么。', '陆门吏压低声音：墨府最近不太平，别在城里乱打听。', [], { role: 'guard', description: '见惯城门风浪，对各路消息都略知一二。' }),
    npc('mofu_steward', '墨府总管', 'jiayuan_market', '最懂得把简单差事变成试炼的人。', '墨府总管抬眼道：会跑腿只是开始，跑得稳、跑得活，才算本事。', ['qixuan_herb'], { role: 'quest_giver', description: '嘉元城最适合新手接触任务循环的引路人。' }),
    npc('granny_yu', '余婆婆', 'jiayuan_market', '卖药卖消息，嘴里七分真三分假。', '余婆婆笑得眯起眼：黄精草和小道消息，我这儿都不算贵。', [], { role: 'vendor', description: '看似寻常摊主，实则是嘉元城消息流转的节点。' }),
    npc('mo_guard_captain', '墨府护院', 'mofu_gate', '他知道谁能进府，也知道谁该挡在门外。', '墨府护院沉声道：墨府这几日不接闲客，少惹事。', [], { role: 'guard', description: '对外来修士并不信任。' }),
    npc('mofu_elder_housekeeper', '府中老管事', 'mofu_front_hall', '焦头烂额地处理宾客、令牌和失窃小事。', '老管事叹道：越是风声紧的时候，越不能少一块令牌。', ['mofu_guest_token'], { role: 'quest_giver', description: '墨府内务的枢纽人物。' }),
    npc('mo_caikuan', '墨彩环', 'mofu_courtyard', '温婉中带着几分警惕，似乎总在观察来往之人。', '墨彩环轻声道：城里事多，人心更乱，能少掺和便少掺和。', [], { role: 'story', description: '墨府线的重要人物，代表嘉元城风波的人情面。' }),
    npc('mo_fengwu', '墨凤舞', 'mofu_courtyard', '行事利落，对外来帮手谈不上完全信任。', '墨凤舞目光锐利：真要帮忙，就别只动嘴。', [], { role: 'story', description: '墨府里的另一种锋利气质。' }),

    npc('wandering_broker', '流动牙人', 'tainan_gate', '知道小会谁在找货、谁在放话。', '流动牙人压低声音：太南谷里最值钱的不是货，是谁先知道消息。', ['ruins_old_map'], { role: 'broker', description: '散修市场里的典型中间人。' }),
    npc('fair_host', '小会司市', 'tainan_fair', '负责维持小会秩序，也负责把争执压到摊位之外。', '司市淡淡道：太南小会讲究先来后到，闹事的人通常都活不长。', [], { role: 'host', description: '散修集会规则的维护者。' }),
    npc('old_book_peddler', '旧书摊贩', 'tainan_fair', '专卖残卷和真假难辨的旧闻。', '旧书摊贩嘿嘿笑道：书里未必有真法门，可真假之间，常有生路。', ['fair_rumor_packet'], { role: 'vendor', description: '散修们接触术法与旧闻的入口。' }),
    npc('xin_ruyin', '辛如音', 'xin_house', '阵法天赋惊人，对妖蛇之胆和阵旗残片都很上心。', '辛如音推开案上阵盘：想帮我，就把能用的材料带来，别拿废话浪费时间。', ['tainan_snake'], { role: 'array_master', description: '太南谷中最鲜明的阵法天才代表。' }),
    npc('qi_yunxiao', '齐云霄', 'xin_house', '待人坦诚，常在辛如音身边帮着整理材料与消息。', '齐云霄笑着拱手：你若真能办成事，辛姑娘自然不会亏待你。', [], { role: 'support', description: '太南线里少有的直爽角色。' }),
    npc('talisman_vendor', '符铺摊主', 'talisman_street', '手里总有几张应急符，价格却从不算便宜。', '符铺摊主把符纸一拍：保命的东西，贵些才正常。', [], { role: 'vendor', description: '提供低阶符箓与护身道具。' }),
    npc('array_apprentice', '阵旗学徒', 'array_lane', '一边打工一边学阵法，对破阵盘残片极其看重。', '阵旗学徒无奈道：师父只让我收拾残局，真正的阵理却半句不肯多说。', ['tainan_array_flag'], { role: 'quest_giver', description: '让玩家接触阵禁和手札的桥梁人物。' }),
    npc('fairy_chen', '陈巧倩', 'array_lane', '身法轻灵，言谈间带着掩月宗门人的清冷和自持。', '陈巧倩淡淡一笑：若你真能从血禁里活着出来，再谈是否值得引荐。', [], { role: 'sect_recruiter', sect_offer_id: 'yanyue_sect', description: '掩月宗在太南线的代表人物。' }),

    npc('huangfeng_steward', '黄枫谷外事执事', 'huangfeng_outpost', '审人、收信、发任务，全看他一句话。', '外事执事翻着名录：会办事的人，黄枫谷从来不嫌多。', ['huangfeng_letter'], { role: 'quest_giver', description: '黄枫谷正式门径的看门人。' }),
    npc('elder_ma', '马师伯', 'huangfeng_hall', '气息温和却让人不敢怠慢，是黄枫谷一系的引路人。', '马师伯看你片刻：修行路长，先学会按规矩活着。', [], { role: 'sect_master', sect_offer_id: 'huangfeng_valley', description: '黄枫谷支线与入门节点的核心人物。' }),
    npc('disciple_lu', '吕师兄', 'huangfeng_hall', '负责抄录名册和安排外门差事。', '吕师兄点头：先把基础差事做好，再说进阶修行。', [], { role: 'senior', description: '典型宗门前辈形象。' }),
    npc('medicine_deacon', '药台执事', 'huangfeng_medicine_terrace', '比谁都清楚哪些药材值钱，哪些药材会要命。', '药台执事提醒你：灵药不是越多越好，关键是认得清。', ['medicine_moss'], { role: 'medicine', description: '黄枫谷药理系统的入门导师。' }),
    npc('scripture_keeper', '经廊守卷人', 'huangfeng_scripture', '把黄枫谷的法度和启蒙口诀看得极重。', '守卷人缓缓道：书卷可以借，心性却借不来。', ['huangfeng_manual'], { role: 'scripture', description: '黄枫谷功法与手册线的重要人物。' }),
    npc('maple_hunter', '枫岭猎户', 'huangfeng_foothill', '在外山混迹多年，最懂哪里会出风鸦与山猪。', '猎户把弓背回肩头：山里没那么多规矩，能活着回来就是规矩。', [], { role: 'hunter', description: '外山生态与采集线索的民间来源。' }),

    npc('blood_warden', '血禁执事', 'blood_gate', '对禁地规则极严，赏罚也最分明。', '血禁执事冷声道：进去之前想清楚，里面拿到的每一株灵草都得拿命去换。', ['blood_forbidden_token', 'blood_swamp_rescue'], { role: 'quest_giver', description: '血色禁地阶段的强规则人物。' }),
    npc('forbidden_scout', '禁地斥候', 'blood_forbidden_outer', '负责标记外围安全线，却不保证你能活着回到线内。', '禁地斥候道：雾色一深，谁也别逞能。', [], { role: 'scout', description: '提醒玩家禁地风险和线路。' }),
    npc('swamp_survivor', '沼泽幸存者', 'blood_swamp', '身上还残留着血雾和虫毒的味道。', '幸存者喘着气：若要过沼泽，先学会怕。', [], { role: 'survivor', description: '禁地危险的活注脚。' }),
    npc('orchid_gatherer', '采兰修士', 'blood_orchid_vale', '一边采药一边防备毒虫，眼神时刻不敢放松。', '采兰修士苦笑：这里的血兰确实值钱，可没命拿又能如何。', [], { role: 'gatherer', description: '血兰谷的即时氛围人物。' }),

    npc('seaport_broker', '海商牙人', 'tiannan_harbor', '掌管船票、货单与海图消息，是海港最重要的中间人。', '海商牙人笑道：你若真有货，我也真有路。', ['chaos_sea_chart'], { role: 'broker', description: '天南港向乱星海过渡的关键中介。' }),
    npc('harbor_guard', '港口甲士', 'tiannan_harbor', '负责压住港口上的冲突和偷渡者。', '港口甲士冷冷道：海上死的人够多了，别在岸上再添乱子。', [], { role: 'guard', description: '港口秩序维护者。' }),
    npc('sea_peddler', '海货商', 'tiannan_market', '卖的货大多来自乱星海，真假全凭你自己判断。', '海货商敲着贝壳：海里的东西都带点运气，你敢买就敢赚。', [], { role: 'vendor', description: '乱星海物产的第一层入口。' }),
    npc('chart_seller', '卖图客', 'tiannan_market', '掌握不少航线残图，只肯在合适的时候开价。', '卖图客压着嗓子：图可以卖，命可不打折。', [], { role: 'vendor', description: '提供航线、秘境与海域传闻。' }),
    npc('old_shipwright', '老船匠', 'tiannan_dock', '懂船、懂阵，也懂怎么让一艘船熬过风暴。', '老船匠摸着船板道：船有骨，人也得有骨，不然都撑不过海上那一夜。', ['captain_supply'], { role: 'craftsman', description: '海路与虚天殿入口的老资格人物。' }),
    npc('tower_watch', '望海守塔人', 'sea_wind_tower', '日日观风辨潮，对乱星海近来变化感受最深。', '守塔人望向海面：风向变了，说明远处又有东西醒了。', [], { role: 'watcher', description: '风暴航线与异象的情报来源。' }),
    npc('shadow_buyer', '暗巷买主', 'smuggler_alley', '出手干脆，绝不多问货物来处。', '暗巷买主一摊手：我只认东西，不认故事。', ['harbor_signal'], { role: 'broker', description: '灰色交易与偏门消息的入口。' }),

    npc('captain_qu', '曲船主', 'chaos_sea_port', '在近港混迹多年，知道怎样带新人穿过第一段暗流。', '曲船主啧了一声：海上不看嘴皮子，只看你能不能稳住船。', [], { role: 'captain', description: '乱星海航线引导人物。' }),
    npc('deck_mage', '甲板术士', 'chaos_sea_ship', '负责船上禁制与风帆法阵，对妖鱼材料很感兴趣。', '甲板术士抬手理了理阵灯：若想进更深的海域，你总得学点真正的法门。', ['demon_fish_core'], { role: 'mage', description: '从近战体系过渡到法术体系的指导角色。' }),
    npc('island_hermit', '孤岛隐士', 'chaos_sea_isle', '独居海岛多年，像是把不少古修秘密都埋在了碑文里。', '孤岛隐士摩挲着残碑：你若能看懂这上面的裂痕，就离虚天更近一步。', ['chaos_relic'], { role: 'hermit', description: '残碑孤岛与虚天线的重要解谜人物。' }),
    npc('reef_diver', '礁海潜修', 'reef_shore', '在浅滩和礁缝间摸索资源，最清楚灵鳞什么时候最容易采到。', '潜修拍了拍湿漉漉的袋子：海里的资源从不等人。', [], { role: 'gatherer', description: '乱星海采集系统的在地人物。' }),

    npc('palace_remnant_spirit', '守门残灵', 'xutian_hall', '半是守卫半是引路，像在审视每一个闯入者够不够资格。', '守门残灵声音空茫：残钥在手者，可入此殿一步。', ['xutian_key'], { role: 'spirit', description: '虚天殿门槛与规则的象征。' }),
    npc('star_tablet_spirit', '祭台残灵', 'xutian_star_platform', '仍记得当年祭台运转时的星纹轨迹，对演算极为执着。', '祭台残灵低语：星图不正，玄门便永不开。', ['xutian_star_map'], { role: 'spirit', description: '虚天殿深层阵禁与年历解锁的关键人物。' }),
  ],
};
