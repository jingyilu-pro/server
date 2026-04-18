const identityTrack = (track_id, name, kind, ranks, description, extras = {}) => ({
  track_id,
  name,
  kind,
  ranks,
  description,
  mentor_ids: extras.mentor_ids ?? [],
  service_unlocks: extras.service_unlocks ?? [],
});

const sceneService = (scene_id, service_tags, rumor_topics, extras = {}) => ({
  scene_id,
  service_tags,
  rumor_topics,
  board_available: extras.board_available ?? false,
  mentor_ids: extras.mentor_ids ?? [],
});

export default {
  identity_tracks: [
    identityTrack('loose_cultivator', '散修路', 'loose', ['行脚散修', '游方散人', '采真客', '洞府之主'], '散修不靠山门，也能靠风声、坊市、海路和手艺慢慢起势；走到结丹与凝婴前夜时，外海、残环与星渊便会成为新的试胆场。', {
      mentor_ids: ['xu_wanderer', 'loose_master_wen', 'seaport_broker'],
      service_unlocks: ['rumor', 'board', 'trade', 'travel'],
    }),
    identityTrack('qixuan_gate', '七玄门', 'sect', ['记名', '外门', '内门', '执事'], '七玄门不是一次性教程门派，而是一条从外场跑腿、堂前录事、弟子舍杂务、药圃打底再走到官道护送的长期起步线；它最适合把凡俗江湖、人情、营生和初阶修行接在一起。', {
      mentor_ids: ['li_feiyu', 'han_zhanglao'],
      service_unlocks: ['mentor', 'board', 'duty', 'gather', 'travel'],
    }),
    identityTrack('huangfeng_valley', '黄枫谷', 'sect', ['记名', '外门', '内门', '执事'], '黄枫谷重规矩、药园与基础功法，是炼气到筑基准备的重要正道路线；到了筑基后段，门内贡献与见闻会直接影响你能否继续摸到结丹门槛。', {
      mentor_ids: ['huangfeng_steward', 'elder_ma', 'medicine_deacon'],
      service_unlocks: ['mentor', 'sect', 'alchemy', 'board'],
    }),
    identityTrack('spirit_beast_mountain', '灵兽山', 'sect', ['记名', '外门', '内门', '执事'], '灵兽山偏向灵兽、灵虫、照料和采药，是资源循环感最强的一条门派线；中后期仍能通过高阶材料差事与外海采集维持稳定积累。', {
      mentor_ids: ['spirit_beast_steward', 'beast_feeder_zhou'],
      service_unlocks: ['mentor', 'sect', 'gather', 'board'],
    }),
  ],
  scene_services: [
    sceneService('qixuan_square', ['mentor', 'rumor', 'board'], ['山门风声', '外场差事'], {
      board_available: true,
      mentor_ids: ['li_feiyu'],
    }),
    sceneService('qixuan_hall', ['sect', 'mentor', 'duty'], ['堂前录事', '门内规矩'], {
      mentor_ids: ['han_zhanglao'],
    }),
    sceneService('qixuan_dormitory', ['work', 'mentor'], ['弟子舍杂务', '外门口风'], {
      mentor_ids: ['sun_er'],
    }),
    sceneService('qixuan_medicine_garden', ['work', 'gather', 'mentor'], ['药圃拣苗', '浅药入门'], {
      mentor_ids: ['field_steward'],
    }),
    sceneService('escort_post', ['travel', 'work', 'mentor'], ['官道风声', '押路试手'], {
      mentor_ids: ['escort_captain_shen'],
    }),
    sceneService('jiayuan_east_gate', ['rumor', 'travel'], ['东门风声', '城里差遣'], {
      mentor_ids: ['gate_guard_lu'],
    }),
    sceneService('jiayuan_market', ['board', 'trade', 'rumor'], ['嘉元城风声', '坊市采办'], {
      board_available: true,
      mentor_ids: ['mofu_steward'],
    }),
    sceneService('mofu_front_hall', ['board', 'rumor'], ['府内差遣', '来客名录'], {
      board_available: true,
      mentor_ids: ['mofu_elder_housekeeper'],
    }),
    sceneService('mofu_courtyard', ['rumor', 'mentor'], ['内院风声', '府内人情'], {
      mentor_ids: ['mo_caikuan', 'mo_fengwu'],
    }),
    sceneService('bamboo_forest', ['gather', 'danger'], ['林间小路', '蛇行竹影']),
    sceneService('herb_slope', ['gather', 'rumor'], ['黄精去路', '采药口风']),
    sceneService('tainan_gate', ['board', 'rumor', 'trade', 'travel'], ['谷口门路', '散修消息'], {
      board_available: true,
      mentor_ids: ['wandering_broker'],
    }),
    sceneService('tainan_fair', ['board', 'rumor', 'trade'], ['小会密闻', '散修消息'], {
      board_available: true,
      mentor_ids: ['old_book_peddler', 'xin_ruyin'],
    }),
    sceneService('loose_camp_square', ['board', 'rumor', 'trade', 'mentor'], ['散修营生', '棚市风声'], {
      board_available: true,
      mentor_ids: ['loose_master_wen'],
    }),
    sceneService('loose_market_lane', ['board', 'trade', 'rumor'], ['杂货消息', '旧图门路'], {
      board_available: true,
      mentor_ids: ['market_broker_hu'],
    }),
    sceneService('loose_medicine_tent', ['work', 'gather', 'mentor', 'rumor'], ['草药营生', '炼药门路'], {
      mentor_ids: ['herb_tutor_qing'],
    }),
    sceneService('loose_diviner_mat', ['rumor', 'mentor'], ['旧路消息', '识物口风'], {
      mentor_ids: ['diviner_meng'],
    }),
    sceneService('loose_guest_hall', ['board', 'rumor', 'work'], ['借宿消息', '散修往来'], {
      board_available: true,
      mentor_ids: ['guest_scribe_fan'],
    }),
    sceneService('xin_house', ['mentor', 'rumor'], ['阵法风声'], {
      mentor_ids: ['xin_ruyin', 'qi_yunxiao'],
    }),
    sceneService('mountain_path', ['travel', 'danger'], ['谷外旧路', '血禁门径']),
    sceneService('huangfeng_outpost', ['board', 'sect', 'mentor'], ['外营事务', '黄枫谷门径'], {
      board_available: true,
      mentor_ids: ['huangfeng_steward', 'elder_ma'],
    }),
    sceneService('huangfeng_hall', ['board', 'sect', 'mentor'], ['内堂录名', '黄枫规矩'], {
      board_available: true,
      mentor_ids: ['elder_ma', 'disciple_lu'],
    }),
    sceneService('huangfeng_medicine_terrace', ['board', 'sect', 'mentor', 'alchemy', 'gather'], ['药台巡材', '药园门路'], {
      board_available: true,
      mentor_ids: ['medicine_deacon'],
    }),
    sceneService('huangfeng_scripture', ['board', 'sect', 'mentor', 'rumor'], ['经廊抄卷', '旧卷口诀'], {
      board_available: true,
      mentor_ids: ['scripture_keeper'],
    }),
    sceneService('huangfeng_cloud_bridge', ['travel', 'sect', 'rumor'], ['云桥口信', '门内去路']),
    sceneService('blood_gate', ['danger', 'rumor', 'mentor'], ['血禁规矩', '入禁名单'], {
      mentor_ids: ['blood_warden'],
    }),
    sceneService('blood_forbidden_outer', ['danger', 'gather', 'rumor'], ['外围安全线', '禁地旧闻'], {
      mentor_ids: ['forbidden_scout'],
    }),
    sceneService('blood_swamp', ['danger', 'gather', 'rumor'], ['沼泽残息', '虫毒回声'], {
      mentor_ids: ['swamp_survivor'],
    }),
    sceneService('spirit_beast_outer_gate', ['board', 'sect', 'mentor', 'gather'], ['兽栏照料', '灵虫差遣'], {
      board_available: true,
      mentor_ids: ['spirit_beast_steward'],
    }),
    sceneService('spirit_beast_broker_lane', ['trade', 'sect', 'rumor'], ['兽材行情', '外山货路'], {
      mentor_ids: ['bug_trader_tao'],
    }),
    sceneService('spirit_beast_beast_pen', ['work', 'sect', 'mentor'], ['兽栏巡喂', '饲养门路'], {
      mentor_ids: ['beast_feeder_zhou'],
    }),
    sceneService('spirit_beast_insect_garden', ['work', 'gather', 'sect', 'rumor'], ['虫圃换饵', '灵虫门路'], {
      mentor_ids: ['insect_master_qin'],
    }),
    sceneService('spirit_beast_hall', ['board', 'sect', 'mentor'], ['外山点事', '执事差遣'], {
      board_available: true,
      mentor_ids: ['outer_deacon_du'],
    }),
    sceneService('spirit_beast_herb_ridge', ['work', 'gather', 'rumor'], ['饲草差遣', '外山药草'], {
      mentor_ids: ['ridge_keeper_pei'],
    }),
    sceneService('spirit_beast_taming_yard', ['work', 'sect', 'rumor'], ['驯兽试手', '幼兽脾性']),
    sceneService('spirit_beast_inner_path', ['travel', 'sect', 'rumor'], ['内山路引', '兽山规矩']),
    sceneService('tiannan_harbor', ['board', 'travel', 'trade', 'rumor'], ['港口风声', '远航门路'], {
      board_available: true,
      mentor_ids: ['seaport_broker'],
    }),
    sceneService('tiannan_market', ['board', 'trade', 'rumor'], ['坊市誊图', '海货门路'], {
      board_available: true,
      mentor_ids: ['chart_seller', 'sea_peddler'],
    }),
    sceneService('tiannan_dock', ['board', 'travel', 'mentor', 'rumor'], ['码头紧索', '远航准备'], {
      board_available: true,
      mentor_ids: ['old_shipwright'],
    }),
    sceneService('sea_wind_tower', ['travel', 'rumor'], ['风向异动', '潮势观察'], {
      mentor_ids: ['tower_watch'],
    }),
    sceneService('harbor_backbay', ['work', 'sea', 'rumor'], ['后湾潮路', '浅滩海猎'], {
      mentor_ids: ['backbay_fisher_wu'],
    }),
    sceneService('harbor_salt_house', ['work', 'trade', 'rumor'], ['盐棚拣货', '后湾海材'], {
      mentor_ids: ['salt_house_keeper_lin'],
    }),
    sceneService('harbor_net_field', ['work', 'sea', 'rumor'], ['晒网分潮', '近海图线'], {
      mentor_ids: ['net_master_peng'],
    }),
    sceneService('harbor_lamp_tower', ['travel', 'rumor'], ['后湾灯色', '出湾时机'], {
      mentor_ids: ['lamp_guard_xie'],
    }),
    sceneService('reef_shore', ['sea', 'gather', 'rumor'], ['礁影海材', '近海潮纹']),
    sceneService('chaos_sea_port', ['board', 'travel', 'sea', 'rumor'], ['近港海路', '近海海猎'], {
      board_available: true,
      mentor_ids: ['captain_qu'],
    }),
    sceneService('chaos_sea_ship', ['travel', 'rumor', 'mentor'], ['船上旧闻', '结丹门径'], {
      mentor_ids: ['deck_mage'],
    }),
    sceneService('chaos_sea_isle', ['danger', 'rumor', 'mentor'], ['残碑旧闻', '孤岛退路'], {
      mentor_ids: ['island_hermit'],
    }),
    sceneService('storm_route', ['travel', 'danger', 'rumor'], ['风暴断航', '深海退路']),
    sceneService('outer_isles_wharf', ['board', 'travel', 'trade', 'rumor'], ['群岛小埠', '清舱点货'], {
      board_available: true,
      mentor_ids: ['island_broker_shi'],
    }),
    sceneService('outer_isles_market', ['board', 'trade', 'sea', 'rumor'], ['珠市成色', '采珠门路'], {
      board_available: true,
      mentor_ids: ['pearl_diver_lan'],
    }),
    sceneService('outer_isles_watch_altar', ['travel', 'rumor', 'mentor'], ['听潮时机', '群岛风向'], {
      mentor_ids: ['altar_keeper_hua'],
    }),
    sceneService('outer_isles_lagoon', ['sea', 'gather', 'rumor'], ['潟湖候潮', '浅海虫珠']),
    sceneService('outer_isles_storm_tree', ['travel', 'rumor', 'mentor'], ['雷木风向', '外海借道'], {
      mentor_ids: ['storm_scout_qi'],
    }),
    sceneService('xutian_hall', ['board', 'danger', 'rumor'], ['残殿旧闻', '探禁提示'], {
      board_available: true,
      mentor_ids: ['palace_remnant_spirit'],
    }),
    sceneService('xutian_corridor', ['danger', 'rumor'], ['回廊残讯', '前殿退路']),
    sceneService('xutian_pill_room', ['danger', 'gather', 'rumor'], ['遗室残火', '丹渣旧方']),
    sceneService('xutian_star_platform', ['board', 'danger', 'rumor'], ['星纹旧闻', '凝婴线索'], {
      board_available: true,
      mentor_ids: ['star_tablet_spirit'],
    }),
    sceneService('xutian_inner_gate', ['danger', 'rumor'], ['内门试心', '深殿旧规']),
    sceneService('outer_sea_mid', ['travel', 'sea', 'rumor', 'danger', 'gold_core'], ['外海潮路', '结丹线索', '测潮退路']),
    sceneService('core_flame_vein', ['danger', 'gather', 'rumor', 'gold_core'], ['丹火灵脉', '结丹辅材', '火脉筛脉']),
    sceneService('xutian_endless_wall', ['danger', 'gather', 'rumor'], ['壁上残纹', '听壁回声'], {
      mentor_ids: ['wall_listener_qiu'],
    }),
    sceneService('ancient_ruin_ring', ['danger', 'board', 'rumor', 'gold_core'], ['古修残环', '残殿回响', '残纹拓录'], {
      board_available: true,
    }),
    sceneService('star_abyss', ['travel', 'danger', 'rumor', 'nascent_soul'], ['星渊潮眼', '凝婴灵物', '候潮守识']),
  ],
};
