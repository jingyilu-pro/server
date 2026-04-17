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
    identityTrack('qixuan_gate', '七玄门', 'sect', ['记名', '外门', '内门', '执事'], '七玄门适合作为凡俗江湖与修行世界之间的稳妥起步点。', {
      mentor_ids: ['li_feiyu', 'han_zhanglao'],
      service_unlocks: ['mentor', 'board', 'duty'],
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
    sceneService('jiayuan_market', ['board', 'trade', 'rumor'], ['嘉元城风声', '坊市采办'], {
      board_available: true,
      mentor_ids: ['mofu_steward'],
    }),
    sceneService('tainan_fair', ['board', 'rumor', 'trade'], ['小会密闻', '散修消息'], {
      board_available: true,
      mentor_ids: ['old_book_peddler', 'xin_ruyin'],
    }),
    sceneService('xin_house', ['mentor', 'rumor'], ['阵法风声'], {
      mentor_ids: ['xin_ruyin', 'qi_yunxiao'],
    }),
    sceneService('huangfeng_outpost', ['board', 'sect', 'mentor'], ['外营事务', '黄枫谷门径'], {
      board_available: true,
      mentor_ids: ['huangfeng_steward', 'elder_ma'],
    }),
    sceneService('spirit_beast_outer_gate', ['board', 'sect', 'mentor', 'gather'], ['兽栏照料', '灵虫差遣'], {
      board_available: true,
      mentor_ids: ['spirit_beast_steward'],
    }),
    sceneService('tiannan_harbor', ['board', 'travel', 'trade', 'rumor'], ['港口风声', '远航门路'], {
      board_available: true,
      mentor_ids: ['seaport_broker'],
    }),
    sceneService('harbor_backbay', ['work', 'sea', 'rumor'], ['后湾潮路', '浅滩海猎'], {
      mentor_ids: ['backbay_fisher_wu'],
    }),
    sceneService('chaos_sea_port', ['board', 'travel', 'sea', 'rumor'], ['近港海路', '近海海猎'], {
      board_available: true,
      mentor_ids: ['captain_qu'],
    }),
    sceneService('xutian_hall', ['board', 'danger', 'rumor'], ['残殿旧闻', '探禁提示'], {
      board_available: true,
      mentor_ids: ['palace_remnant_spirit'],
    }),
    sceneService('xutian_star_platform', ['board', 'danger', 'rumor'], ['星纹旧闻', '凝婴线索'], {
      board_available: true,
      mentor_ids: ['star_tablet_spirit'],
    }),
    sceneService('outer_sea_mid', ['travel', 'sea', 'rumor'], ['外海潮路', '结丹线索']),
    sceneService('core_flame_vein', ['danger', 'gather', 'rumor'], ['丹火灵脉', '结丹辅材']),
    sceneService('ancient_ruin_ring', ['danger', 'board', 'rumor'], ['古修残环', '残殿回响'], {
      board_available: true,
    }),
    sceneService('star_abyss', ['travel', 'danger', 'rumor'], ['星渊潮眼', '凝婴灵物']),
  ],
};
