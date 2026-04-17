const background = (background_id, name, description, starter_title, focus_label, attribute_bonus, starter_inventory) => ({
  background_id,
  name,
  description,
  starter_title,
  focus_label,
  attribute_bonus,
  starter_inventory,
});

const item = (item_id, name, item_type, description, price, extras = {}) => ({
  item_id,
  name,
  item_type,
  description,
  price,
  hp_restore: extras.hp_restore ?? 0,
  mana_restore: extras.mana_restore ?? 0,
  sen_restore: extras.sen_restore ?? 0,
  sta_restore: extras.sta_restore ?? 0,
  exp_gain: extras.exp_gain ?? 0,
  skill_level_gain: extras.skill_level_gain ?? 0,
  attack_bonus: extras.attack_bonus ?? 0,
  defense_bonus: extras.defense_bonus ?? 0,
  spell_damage_bonus: extras.spell_damage_bonus ?? 0,
  spell_haste_bonus: extras.spell_haste_bonus ?? 0,
  consumable: extras.consumable ?? false,
  equipable: extras.equipable ?? false,
  tags: extras.tags ?? [],
});

const skill = (skill_id, name, category, description, extras = {}) => ({
  skill_id,
  name,
  category,
  description,
  governing_attribute: extras.governing_attribute ?? 'int_attr',
  starter: extras.starter ?? false,
  chapter: extras.chapter ?? '',
});

const spell = (spell_id, name, element, description, mana_cost, extras = {}) => ({
  spell_id,
  name,
  element,
  description,
  mana_cost,
  power: extras.power ?? 22,
  required_realm_stage: extras.required_realm_stage ?? 1,
  granted_by_item_id: extras.granted_by_item_id ?? '',
  chapter: extras.chapter ?? '',
});

const recipe = (recipe_id, name, description, result_item_id, result_quantity, ingredient_items, extras = {}) => ({
  recipe_id,
  name,
  description,
  result_item_id,
  result_quantity,
  ingredient_items,
  station_scene_id: extras.station_scene_id ?? '',
  npc_id: extras.npc_id ?? '',
  success_rate: extras.success_rate ?? 0.8,
  required_skill_id: extras.required_skill_id ?? 'spirit_brew',
  chapter: extras.chapter ?? '',
});

const formation = (formation_id, name, description, scene_id, effect_summary) => ({
  formation_id,
  name,
  description,
  scene_id,
  effect_summary,
});

const quest = (
  quest_id,
  title,
  description,
  issuer_npc_id,
  submit_npc_id,
  required_item_id,
  required_item_count,
  reward_spirit_stone,
  reward_exp,
  reward_item_id,
  reward_item_count,
  extras = {},
) => ({
  quest_id,
  title,
  description,
  issuer_npc_id,
  submit_npc_id,
  required_item_id,
  required_item_count,
  reward_spirit_stone,
  reward_exp,
  reward_item_id,
  reward_item_count,
  reward_sect_id: extras.reward_sect_id ?? '',
  unlock_rules: extras.unlock_rules ?? [],
  chapter: extras.chapter ?? '',
});

const scene = (scene_id, name, region_name, map_x, map_y, description, exits, extras = {}) => ({
  scene_id,
  name,
  region_name,
  map_x,
  map_y,
  description,
  exits,
  npc_ids: extras.npc_ids ?? [],
  monster_ids: extras.monster_ids ?? [],
  shop_item_ids: extras.shop_item_ids ?? [],
  resource_node_ids: extras.resource_node_ids ?? [],
  ground_loot_ids: extras.ground_loot_ids ?? [],
  hazard_ids: extras.hazard_ids ?? [],
  codex_entry_ids: extras.codex_entry_ids ?? [],
  chapter: extras.chapter ?? '',
  pvp_enabled: extras.pvp_enabled ?? false,
  rumors: extras.rumors ?? [],
  room_type: extras.room_type ?? '',
  risk_level: extras.risk_level ?? '',
  landmark: extras.landmark ?? '',
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

const monster = (
  monster_id,
  name,
  scene_id,
  hp,
  attack,
  defense,
  reward_spirit_stone,
  reward_exp,
  drop_item_id,
  drop_item_count,
  extras = {},
) => ({
  monster_id,
  name,
  scene_id,
  hp,
  attack,
  defense,
  reward_spirit_stone,
  reward_exp,
  drop_item_id,
  drop_item_count,
  description: extras.description ?? '',
  kind: extras.kind ?? '妖兽',
  element: extras.element ?? '',
});

const resourceNode = (node_id, name, scene_id, description, drop_item_id, drop_item_count, extras = {}) => ({
  node_id,
  name,
  scene_id,
  description,
  drop_item_id,
  drop_item_count,
  cooldown_ms: extras.cooldown_ms ?? 60000,
  required_skill_id: extras.required_skill_id ?? '',
});

const groundLoot = (loot_id, scene_id, item_id, quantity, description, extras = {}) => ({
  loot_id,
  scene_id,
  item_id,
  quantity,
  description,
  one_time: extras.one_time ?? true,
});

const hazard = (hazard_id, scene_id, name, description, extras = {}) => ({
  hazard_id,
  scene_id,
  name,
  description,
  hp_cost: extras.hp_cost ?? 0,
  mana_cost: extras.mana_cost ?? 0,
  sta_cost: extras.sta_cost ?? 0,
  resist_key: extras.resist_key ?? '',
});

const codexEntry = (entry_id, category, title, summary, content, extras = {}) => ({
  entry_id,
  category,
  title,
  summary,
  content,
  unlock_rules: extras.unlock_rules ?? [],
  related_scene_ids: extras.related_scene_ids ?? [],
  related_npc_ids: extras.related_npc_ids ?? [],
  related_monster_ids: extras.related_monster_ids ?? [],
  related_item_ids: extras.related_item_ids ?? [],
  related_sect_ids: extras.related_sect_ids ?? [],
});

function oppositeDirection(direction) {
  const pairs = {
    north: 'south',
    south: 'north',
    east: 'west',
    west: 'east',
    up: 'down',
    down: 'up',
  };
  return pairs[direction] ?? 'west';
}

function stepForDirection(direction) {
  if (direction === 'north') {
    return { x: 0, y: -8 };
  }
  if (direction === 'south') {
    return { x: 0, y: 8 };
  }
  if (direction === 'east') {
    return { x: 8, y: 0 };
  }
  if (direction === 'west') {
    return { x: -8, y: 0 };
  }
  if (direction === 'up') {
    return { x: 6, y: -6 };
  }
  if (direction === 'down') {
    return { x: 6, y: 6 };
  }
  return { x: 8, y: 0 };
}

function pushLinearArea(targetScenes, targetPatches, spec) {
  const backDirection = oppositeDirection(spec.anchor_direction);
  const step = stepForDirection(spec.anchor_direction);

  targetPatches.push({
    scene_id: spec.anchor_scene_id,
    exits: {
      [spec.anchor_direction]: spec.rooms[0].scene_id,
    },
    chapter: spec.chapter,
  });

  spec.rooms.forEach((room, index) => {
    const exits = {};
    if (index === 0) {
      exits[backDirection] = spec.anchor_scene_id;
    } else {
      exits[backDirection] = spec.rooms[index - 1].scene_id;
    }
    if (index < spec.rooms.length - 1) {
      exits[spec.anchor_direction] = spec.rooms[index + 1].scene_id;
    }

    const map_x = spec.base_x + step.x * index;
    const map_y = spec.base_y + step.y * index;
    targetScenes.push(
      scene(room.scene_id, room.name, spec.region_name, map_x, map_y, room.description, exits, {
        chapter: spec.chapter,
        pvp_enabled: room.pvp_enabled ?? false,
        shop_item_ids: room.shop_item_ids ?? [],
        rumors: room.rumors ?? [],
        room_type: room.room_type ?? '',
        risk_level: room.risk_level ?? '',
        landmark: room.landmark ?? room.name,
      }),
    );
  });
}

const backgrounds = [
  background('herbalist', '药农', '常年与药圃和山地打交道，更容易看懂灵草生长的脾气。', '识草少年', '采药与炼制', { spi: 0, gin: 1, str: 0, per: 1, int_attr: 1, cha: 0, luc: 0 }, [
    { item_id: 'herbal_hoe', quantity: 1, equipped: false },
    { item_id: 'yellow_essence_grass', quantity: 1, equipped: false },
  ]),
  background('hunter', '猎户', '习惯在荒野里判断足迹、风向和危险，起手更适合生存与追猎。', '山野行脚', '追踪与战斗', { spi: 0, gin: 0, str: 1, per: 1, int_attr: 0, cha: 0, luc: 1 }, [
    { item_id: 'hunter_knife', quantity: 1, equipped: false },
    { item_id: 'harbor_dry_food', quantity: 1, equipped: false },
  ]),
  background('scholar', '书生', '读过书、记得路、善记规矩，适合在坊市、典籍和阵禁里慢慢起势。', '执卷入世', '识别与推演', { spi: 1, gin: 0, str: 0, per: 0, int_attr: 2, cha: 0, luc: 0 }, [
    { item_id: 'scribe_brush', quantity: 1, equipped: false },
    { item_id: 'merchant_ledgers', quantity: 1, equipped: false },
  ]),
  background('escort', '镖师', '凡俗江湖里闯荡多年，知道怎么护货、护人，也知道什么时候该先保命。', '行镖老手', '护送与抗压', { spi: 0, gin: 1, str: 1, per: 0, int_attr: 0, cha: 0, luc: 0 }, [
    { item_id: 'escort_badge', quantity: 1, equipped: false },
    { item_id: 'cloth_vest', quantity: 1, equipped: false },
  ]),
  background('fisher', '渔家', '对水路、潮汐和风势更敏锐，适合较早切进海猎和近海探索。', '临海渔子', '海路与采集', { spi: 1, gin: 0, str: 0, per: 1, int_attr: 0, cha: 0, luc: 1 }, [
    { item_id: 'fisher_net', quantity: 1, equipped: false },
    { item_id: 'harbor_dry_food', quantity: 1, equipped: false },
  ]),
];

const items = [
  item('herbal_hoe', '药锄', 'tool', '常年翻土除草用的小锄，也能当简陋采药工具。', 26, { tags: ['tool', 'starter'] }),
  item('hunter_knife', '猎户短刀', 'weapon', '用来剥皮和防身的短刀，轻便而结实。', 48, { attack_bonus: 4, equipable: true, tags: ['weapon', 'starter'] }),
  item('scribe_brush', '记事笔', 'tool', '书生随身携带的硬毫笔，用来记录配方、地图和账册。', 18, { tags: ['tool', 'starter'] }),
  item('escort_badge', '旧镖牌', 'token', '一块磨损严重的旧镖牌，象征你在凡俗里混过不少年头。', 12, { tags: ['identity'] }),
  item('fisher_net', '细麻渔网', 'tool', '补过许多次的旧渔网，丢进浅滩和礁缝都还能凑合用。', 28, { tags: ['tool', 'starter'] }),
  item('traveler_boots', '行脚靴', 'armor', '鞋底厚实耐磨，适合长时间走山路和土路。', 68, { defense_bonus: 3, spell_haste_bonus: 1, equipable: true, tags: ['armor'] }),
  item('hunting_bow', '旧木猎弓', 'weapon', '弓臂不算强，但比赤手空拳更适合牵制荒野妖物。', 86, { attack_bonus: 6, equipable: true, tags: ['weapon'] }),
  item('loose_mantle', '散修斗篷', 'armor', '不起眼的深色斗篷，很适合在坊市和荒野之间来回穿梭。', 76, { defense_bonus: 4, equipable: true, tags: ['armor'] }),
  item('marsh_cloak', '泽行披风', 'armor', '做过防潮处理的披风，适合沼地和潮湿区域。', 92, { defense_bonus: 5, equipable: true, tags: ['armor'] }),
  item('beast_whistle', '驭兽短哨', 'tool', '灵兽山外门常见的驭兽工具，能稳定低阶灵兽情绪。', 96, { tags: ['tool', 'sect'] }),
  item('reef_lantern', '礁灯', 'treasure', '一盏能在潮湿海风里稳稳燃着的灵灯。', 128, { defense_bonus: 3, equipable: true, tags: ['treasure', 'sea'] }),
  item('sea_hunt_chart', '海猎图', 'manual', '记录近海潮汐、礁区与常见海妖迁移时辰的手绘图。', 132, { skill_level_gain: 1, consumable: true, tags: ['manual', 'map'] }),
  item('void_guard_charm', '裂隙护符', 'talisman', '将裂隙残纹压进符纸后制成的护符，能稍稍稳住心神。', 158, { defense_bonus: 7, consumable: true, tags: ['talisman', 'array'] }),
  item('storm_anchor_charm', '定风符', 'talisman', '在风暴海域里尤其好用，能帮修士稳住法力节奏。', 148, { mana_restore: 20, consumable: true, tags: ['talisman', 'wind'] }),
  item('riftbreaking_note', '破隙札记', 'manual', '残灵留下的阵禁笔记，专讲如何看穿古禁外层裂纹。', 176, { skill_level_gain: 1, consumable: true, tags: ['manual', 'array'] }),
  item('identify_manual', '鉴物笔录', 'manual', '散修间常传的记物册，教你用最便宜的方式辨别货色。', 102, { skill_level_gain: 1, consumable: true, tags: ['manual'] }),
  item('wanderer_powder', '游方散', 'consumable', '用山野草木和冷泉水配出的简药，止疲劳最有效。', 42, { hp_restore: 38, sta_restore: 20, consumable: true, tags: ['medicine'] }),
  item('escort_pill', '护路丸', 'consumable', '镖路上常备的小药丸，关键时刻能提一口气。', 56, { hp_restore: 42, sen_restore: 12, consumable: true, tags: ['medicine'] }),
  item('animal_treat', '灵兽饵丸', 'consumable', '喂给低阶灵兽的小丸子，驯养时比空手硬来更稳妥。', 64, { sta_restore: 18, consumable: true, tags: ['food', 'sect'] }),
  item('moon_blessing_draught', '望月露', 'consumable', '近海岛民会在月升时饮用的灵露，能同时缓和神念和法力。', 118, { mana_restore: 34, sen_restore: 24, consumable: true, tags: ['medicine', 'sea'] }),
  item('spirit_beast_manual', '灵兽饲养录', 'manual', '灵兽山外山弟子的入门手册，写满了喂养、巡圈和避险经验。', 128, { skill_level_gain: 1, consumable: true, tags: ['manual', 'sect'] }),
  item('loose_market_talisman', '散修护身符', 'talisman', '太南散修最爱买的护身符，主打便宜耐用。', 104, { defense_bonus: 4, consumable: true, tags: ['talisman'] }),
  item('dewleaf_herb', '露叶草', 'herb', '清晨叶尖会挂上一层淡淡灵露，是配药的温和底材。', 18, { tags: ['herb'] }),
  item('stone_bark', '石树皮', 'material', '长在乱石坡边的硬皮树上，晒干后可作粗药引。', 22, { tags: ['material'] }),
  item('fox_tail_grass', '狐尾草', 'herb', '纤细蓬松，常被荒狐拖着在山路边乱窜。', 20, { tags: ['herb'] }),
  item('amber_resin', '黄琥脂', 'material', '从焦土和老松根里挖出的树脂，炼药时很容易锁香。', 28, { tags: ['material'] }),
  item('night_bat_wing', '夜蝠翼片', 'material', '晒干后又轻又薄，可入药也可炼成轻身辅助。', 24, { tags: ['material'] }),
  item('clear_marrow_water', '净髓泉水', 'material', '取自寒泉深处的灵水，最适合调药。', 26, { tags: ['material'] }),
  item('bandit_token', '匪徒腰牌', 'quest', '截路匪徒身上搜出的腰牌，往往能换到悬赏钱。', 18, { tags: ['quest'] }),
  item('caravan_seal', '货队封签', 'quest', '印着货队标识的蜡封封签，通常只在护送线里有用。', 16, { tags: ['quest'] }),
  item('road_mugwort', '路艾', 'herb', '官道边经常能见到的粗生药草，味道辛烈。', 16, { tags: ['herb'] }),
  item('relay_ore', '驿铁砂', 'material', '驿站边沟里筛出的细铁砂，是炼制粗药釜的便宜材料。', 20, { tags: ['material'] }),
  item('bridge_moss', '桥石苔', 'herb', '老桥边最湿冷的石头上才会生的苔类。', 16, { tags: ['herb'] }),
  item('spirit_ink', '灵墨', 'material', '混着灵砂调成的稠墨，散修常拿它描地图和画符。', 24, { tags: ['material', 'array'] }),
  item('ink_shell', '墨壳片', 'material', '色泽发暗的小硬壳，常被散修拿来做便宜护片或药引。', 22, { tags: ['material'] }),
  item('moon_shell', '月壳', 'material', '表面会在夜里反一点冷光的小壳，常见于山池与海湾。', 24, { tags: ['material', 'sea'] }),
  item('shadow_weed', '影草', 'herb', '长在背阴处，摸上去总带一丝凉意。', 18, { tags: ['herb'] }),
  item('bamboo_mark', '竹签记号', 'quest', '散修营地里用来传递约定地点的竹签。', 14, { tags: ['quest'] }),
  item('beast_bone', '灵兽骨片', 'material', '打磨后可做练手法器，也可入驭兽药丸。', 28, { tags: ['material', 'sect'] }),
  item('ink_moth_wing', '墨蛾翅', 'material', '沾着细细鳞粉，制成药散时能帮助药性挂留。', 28, { tags: ['material'] }),
  item('cliff_egg_shell', '崖卵壳', 'material', '从高崖巢里掉下来的空壳，异常坚韧。', 26, { tags: ['material'] }),
  item('spirit_beast_feed', '灵兽草料', 'material', '经简单灵化处理的兽料，是灵兽山基础差事的常用品。', 22, { tags: ['material', 'sect'] }),
  item('salt_shell', '盐壳', 'material', '海湾边晒出的粗盐和贝壳混在一起，很多人拿它磨药。', 20, { tags: ['material', 'sea'] }),
  item('tide_sand', '潮砂', 'material', '涨落潮之间最容易收集到的湿润灵砂。', 22, { tags: ['material', 'sea'] }),
  item('white_gull_feather', '白鸥翎', 'material', '海湾白鸥掉落的翎羽，轻而韧。', 18, { tags: ['material', 'sea'] }),
  item('drift_fin', '漂鳍', 'material', '从近海低阶海妖身上取得的薄鳍片，灵气不强却很稳定。', 24, { tags: ['material', 'sea'] }),
  item('sea_kelp_bundle', '海灵藻束', 'herb', '沾着盐味的海藻束，炼制宁神类丹药时很常见。', 18, { tags: ['herb', 'sea'] }),
  item('moon_pearl', '望月珠', 'material', '近海修士很喜欢拿它换消息，夜里泛光尤其明显。', 34, { tags: ['material', 'sea'] }),
  item('storm_coral', '风暴珊瑚', 'material', '只有风浪大的礁区才会采到的珊瑚枝。', 36, { tags: ['material', 'sea'] }),
  item('black_reef_tooth', '黑礁齿', 'material', '来自黑礁猎物的硬齿，适合做尖锐部件。', 32, { tags: ['material', 'sea'] }),
  item('moon_salt', '月盐', 'material', '月夜潮水退去后才最容易收集到的细盐。', 24, { tags: ['material', 'sea'] }),
  item('void_crystal', '裂隙晶砂', 'material', '从裂隙里震落的细小晶粒，会轻微刺痛指尖。', 44, { tags: ['material', 'array'] }),
  item('rift_rune', '残纹碎片', 'material', '古修禁制剥落下来的残纹碎片，能拿来研究裂口规律。', 46, { tags: ['material', 'array'] }),
  item('star_shard', '星屑', 'material', '像碎掉的星纹一样细碎，光看着就让人头皮发麻。', 40, { tags: ['material', 'array'] }),
  item('void_dust', '虚尘', 'material', '从静默库室里扫出来的细灰，里面带着淡淡禁制味。', 34, { tags: ['material', 'array'] }),
  item('ash_silk', '灰丝', 'material', '裂隙妖物吐出的灰色丝线，韧性惊人。', 30, { tags: ['material'] }),
];

const skills = [
  skill('focus_art', '凝神诀', '争斗', '把散乱心神收束起来，是很多散修都练过的应急小诀。', { governing_attribute: 'spi', chapter: '散修启途' }),
  skill('identify_art', '鉴物法', '探险', '学会先看货色再动手，能少吃很多亏。', { governing_attribute: 'int_attr', chapter: '太南散修坊' }),
  skill('silent_step', '敛息步', '探险', '走得更轻，也更适合在冲突区先看清局势。', { governing_attribute: 'per', chapter: '越京官道' }),
  skill('beast_taming', '驭兽识性', '探险', '不是高深驭兽术，却足够让你看懂低阶灵兽的脾气。', { governing_attribute: 'gin', chapter: '灵兽山外岭' }),
  skill('net_cast', '撒网诀', '炼制', '海边散修和渔家常用的小法门，擅长处理浅滩和海货。', { governing_attribute: 'per', chapter: '天南外港' }),
  skill('rift_lore', '裂隙残纹', '阵禁', '专讲古禁裂口和残纹变化，是虚天残区的保命学问。', { governing_attribute: 'int_attr', chapter: '虚天残区' }),
];

const spells = [
  spell('blazing_ring', '烈环术', '火', '在身前推出一圈炽热灵焰，适合压制近身目标。', 18, { power: 28, required_realm_stage: 2, chapter: '越京官道' }),
  spell('ice_mirror', '凝镜术', '冰', '先以镜面寒意卸力，再把寒气反推出去。', 18, { power: 27, required_realm_stage: 2, chapter: '太南散修坊' }),
  spell('thunder_chain', '连雷术', '雷', '以细小雷弧连续压迫目标神念和法力运转。', 20, { power: 30, required_realm_stage: 2, chapter: '乱星近海群岛' }),
  spell('wind_escape', '遁风诀', '风', '更偏保命和位移的风系术法，适合冲突区脱身。', 17, { power: 24, required_realm_stage: 2, chapter: '天南外港' }),
  spell('corrosion_mark', '蚀痕印', '蚀', '在敌手护体上留下难以抹去的蚀痕，后劲十足。', 20, { power: 31, required_realm_stage: 3, chapter: '虚天残区' }),
  spell('venom_dart', '毒芒术', '毒', '把毒性压成一道极细光芒，命中后极难清理。', 18, { power: 26, required_realm_stage: 2, chapter: '灵兽山外岭' }),
];

const recipes = [
  recipe('wanderer_powder_recipe', '游方散调配', '露叶草、狐尾草和净髓泉水最适合调成走山路用的小药。', 'wanderer_powder', 1, [
    { item_id: 'dewleaf_herb', quantity: 1 },
    { item_id: 'fox_tail_grass', quantity: 1 },
    { item_id: 'clear_marrow_water', quantity: 1 },
  ], { station_scene_id: 'wanderer_camp', npc_id: 'xu_wanderer', chapter: '散修启途' }),
  recipe('escort_pill_recipe', '护路丸炼制', '把路艾和桥石苔压住药性，能做出镖师最常用的护路丸。', 'escort_pill', 1, [
    { item_id: 'road_mugwort', quantity: 1 },
    { item_id: 'bridge_moss', quantity: 1 },
  ], { station_scene_id: 'relay_station', npc_id: 'relay_clerk_zhou', chapter: '越京官道' }),
  recipe('animal_treat_recipe', '灵兽饵丸和制', '灵兽草料掺上骨粉，是灵兽山最常见的基础差事。', 'animal_treat', 1, [
    { item_id: 'spirit_beast_feed', quantity: 1 },
    { item_id: 'beast_bone', quantity: 1 },
  ], { station_scene_id: 'spirit_beast_beast_pen', npc_id: 'beast_feeder_zhou', chapter: '灵兽山外岭' }),
  recipe('moon_draught_recipe', '望月露调和', '望月珠、月盐和海灵藻最适合炼成宁神灵露。', 'moon_blessing_draught', 1, [
    { item_id: 'moon_pearl', quantity: 1 },
    { item_id: 'moon_salt', quantity: 1 },
    { item_id: 'sea_kelp_bundle', quantity: 1 },
  ], { station_scene_id: 'outer_isles_market', npc_id: 'pearl_diver_lan', chapter: '乱星近海群岛' }),
  recipe('void_guard_recipe', '裂隙护符压灵', '裂隙晶砂与残纹碎片压进灰丝，可成一枚粗用护符。', 'void_guard_charm', 1, [
    { item_id: 'void_crystal', quantity: 1 },
    { item_id: 'rift_rune', quantity: 1 },
    { item_id: 'ash_silk', quantity: 1 },
  ], { station_scene_id: 'xutian_rune_garden', npc_id: 'rift_record_spirit', chapter: '虚天残区', success_rate: 0.72 }),
];

const formations = [
  formation('wanderer_camp_array', '夜宿息风阵', '散修夜里最常布的小阵，只求稳睡一觉。', 'wanderer_camp', '减少夜间野外损耗。'),
  formation('escort_beacon_array', '路驿照警阵', '护送线驿站常见的小阵，专门防夜袭和偷货。', 'relay_station', '提高安全与预警能力。'),
  formation('beast_pen_array', '安兽圈阵', '灵兽山饲养低阶灵兽时最常用的安抚阵。', 'spirit_beast_beast_pen', '让灵兽更稳定。'),
  formation('outer_isles_tide_array', '听潮阵', '群岛散修依据潮势改出来的小阵，兼顾采珠与避险。', 'outer_isles_watch_altar', '减少海潮区域惩罚。'),
  formation('void_rift_array', '裂隙压纹阵', '残灵用来延缓裂隙进一步扩张的残缺古阵。', 'xutian_void_rift', '降低裂隙场景的持续损耗。'),
];

const manual_codex_entries = [
  codexEntry('codex_shared_world_road', '韩立年历', '时代流闻·散修自立', '韩立式谨慎并不等于每个人都要走他的单线故事。', '如今的天南更像一张长期展开的修仙江湖，散修、宗门、海商、驿站、坊市与秘境消息同时并存。玩家只是其中一名原创修士，要在共享世界里慢慢站稳脚跟。', {
    unlock_rules: [{ trigger: 'enter_scene', target_id: 'wanderer_trail' }],
    related_scene_ids: ['wanderer_trail', 'loose_camp_square'],
  }),
  codexEntry('codex_shared_world_sect', '宗门志', '势力并存·四线起步', '七玄门、黄枫谷、灵兽山与散修路并行存在。', '阶段一不再把宗门只当剧情门槛，而是把它们做成长线身份路线。你可以长期当散修，也可以晚些再决定是否入门。', {
    unlock_rules: [{ trigger: 'enter_scene', target_id: 'spirit_beast_outer_gate' }],
    related_scene_ids: ['qixuan_hall', 'huangfeng_hall', 'spirit_beast_outer_gate'],
    related_sect_ids: ['qixuan_gate', 'huangfeng_valley', 'spirit_beast_mountain'],
  }),
  codexEntry('codex_shared_world_hunt', '地理志', '外港海猎', '天南外港到乱星近海之间，是最像共享世界日常的资源线。', '这里没有唯一主角的剧情演出，只有海潮、货单、海妖、船票、队伍、风暴和一次次重复却有积累的海猎循环。', {
    unlock_rules: [{ trigger: 'enter_scene', target_id: 'harbor_backbay' }],
    related_scene_ids: ['harbor_backbay', 'outer_isles_wharf'],
  }),
  codexEntry('codex_shared_world_rift', '地理志', '虚天残区', '虚天殿内外不止一条主线，而是一片会长期吞人也会长期产出的高风险区域。', '裂隙、残纹、残灵与掉落共同构成了高危 MUD 区域。这里更接近“长期争夺与探索场”，而不是一次性的过场副本。', {
    unlock_rules: [{ trigger: 'enter_scene', target_id: 'xutian_void_rift' }],
    related_scene_ids: ['xutian_void_rift', 'xutian_star_platform', 'xutian_inner_gate'],
  }),
];

const scenes = [];
const scene_patches = [];

pushLinearArea(scenes, scene_patches, {
  anchor_scene_id: 'qixuan_backslope',
  anchor_direction: 'west',
  region_name: '七玄外野',
  chapter: '散修启途',
  base_x: -2,
  base_y: 28,
  rooms: [
    { scene_id: 'wanderer_trail', name: '散修古道', description: '一条被脚步踏得发白的土路，许多想离开山门却又不敢走太远的人都会先到这里试试胆。', room_type: '野外', risk_level: '安全' },
    { scene_id: 'wanderer_camp', name: '落脚草棚', description: '几座草棚和旧火塘拼成了临时歇脚地，游方散修与凡俗行脚人常在此交换口粮和消息。', room_type: '营地', risk_level: '安全', shop_item_ids: ['wanderer_powder', 'traveler_boots'] },
    { scene_id: 'wanderer_creek', name: '乱石溪湾', description: '溪水绕着乱石打着旋，岸边长着几簇叶尖挂露的小草。', room_type: '野外', risk_level: '历练' },
    { scene_id: 'wanderer_brush', name: '狐草灌丛', description: '灌木间常见浅浅爪印，风一吹，狐尾草就在草尖来回乱晃。', room_type: '野外', risk_level: '历练' },
    { scene_id: 'wanderer_stone_slope', name: '乱石坡', description: '碎石滚落声时有传来，坡上偶有健硕野物驻足望向来人。', room_type: '野外', risk_level: '历练' },
    { scene_id: 'wanderer_old_temple', name: '残庙旧址', description: '断墙下还压着些香灰和旧竹签，像是谁曾在这里短暂躲过风雨。', room_type: '遗迹', risk_level: '安全' },
    { scene_id: 'wanderer_firefield', name: '焦土地', description: '一片被旧火烧过的地面泛着暗黄颜色，翻开表土还能摸到温热树脂。', room_type: '野外', risk_level: '历练' },
    { scene_id: 'wanderer_fox_den', name: '狐穴坡', description: '坡脚下密密麻麻全是小洞口，空气里带着一股野兽久居的土腥味。', room_type: '野外', risk_level: '冲突', pvp_enabled: true },
    { scene_id: 'wanderer_watch_tower', name: '断木望台', description: '由旧木架撑起来的简陋望台已经斜了半边，但仍能望见远近几条山路。', room_type: '哨点', risk_level: '安全' },
    { scene_id: 'wanderer_cold_pool', name: '寒潭边', description: '潭水颜色极深，边缘结着薄薄水汽，附近草木都显得格外清冷。', room_type: '野外', risk_level: '历练' },
  ],
});

pushLinearArea(scenes, scene_patches, {
  anchor_scene_id: 'jiayuan_east_gate',
  anchor_direction: 'north',
  region_name: '越京官道',
  chapter: '越京官道',
  base_x: 28,
  base_y: 8,
  rooms: [
    { scene_id: 'escort_post', name: '东门驿棚', description: '离嘉元城最近的一处驿棚，护送队、镖局人和零散行商总在这里凑成临时队伍。', room_type: '驿站', risk_level: '安全' },
    { scene_id: 'escort_road', name: '碎石官道', description: '车辙和马蹄把官道压得结结实实，偶尔也会留下打斗后拖行的痕迹。', room_type: '官道', risk_level: '历练' },
    { scene_id: 'escort_wayside', name: '路旁荒亭', description: '荒亭柱脚被刀痕刮得斑驳，墙角还丢着半截折断的竹竿。', room_type: '官道', risk_level: '历练' },
    { scene_id: 'bandit_gully', name: '截路沟', description: '这条沟壑视野狭窄，最适合埋伏截路，来往货队都不愿在此久停。', room_type: '野外', risk_level: '冲突', pvp_enabled: true },
    { scene_id: 'broken_bridge', name: '断桥坎', description: '半截石桥垮在溪沟上，桥面潮湿，苔痕和水声让人很难放松警惕。', room_type: '官道', risk_level: '历练' },
    { scene_id: 'relay_station', name: '换马驿', description: '驿站里堆着木箱、旧鞍和铁砂袋，护送队最喜欢在这里补给和换人。', room_type: '驿站', risk_level: '安全', shop_item_ids: ['escort_pill', 'hunting_bow', 'marsh_cloak'] },
    { scene_id: 'granary_yard', name: '粮场外院', description: '晒场上铺着薄薄一层谷壳，墙边站着几个提防盗匪的守夜人。', room_type: '院落', risk_level: '安全' },
    { scene_id: 'night_inn', name: '夜宿破栈', description: '客栈灯火昏黄，掌柜说话总压着嗓子，像生怕吵醒不该吵醒的人。', room_type: '客栈', risk_level: '安全' },
    { scene_id: 'border_field', name: '边田荒地', description: '路边农田早已半荒，天一黑就很容易分不清脚边是沟还是伏兵。', room_type: '野外', risk_level: '冲突', pvp_enabled: true },
    { scene_id: 'yue_watch_pass', name: '越境望隘', description: '再往前就是更远的凡俗疆界，望隘上的人对陌生修士尤其敏感。', room_type: '关隘', risk_level: '历练' },
  ],
});

pushLinearArea(scenes, scene_patches, {
  anchor_scene_id: 'tainan_gate',
  anchor_direction: 'down',
  region_name: '太南散修坊',
  chapter: '太南散修坊',
  base_x: 42,
  base_y: 42,
  rooms: [
    { scene_id: 'loose_camp_gate', name: '散修地棚口', description: '临时搭出的地棚口挂着几面旧布幌，散修们把这里当作真正属于自己的落脚处。', room_type: '营地', risk_level: '安全' },
    { scene_id: 'loose_camp_square', name: '散修棚市', description: '摊布、竹椅、木盆和药炉挤在一起，空气里全是低阶修士忙着求活的味道。', room_type: '坊市', risk_level: '安全', shop_item_ids: ['loose_mantle', 'identify_manual', 'loose_market_talisman'] },
    { scene_id: 'loose_market_lane', name: '杂货棚巷', description: '巷道狭窄，货却杂得出奇，从粗药、烂卷到假地图什么都能见到。', room_type: '坊市', risk_level: '安全' },
    { scene_id: 'loose_medicine_tent', name: '药帐', description: '棚中挂着晒药串和几口黑锅，懂草药的散修经常在这里接小活。', room_type: '营地', risk_level: '安全' },
    { scene_id: 'loose_diviner_mat', name: '卜席角', description: '几张草席围出一片半私密空间，擅长占验和识物的人总在这里坐着看人。', room_type: '营地', risk_level: '安全' },
    { scene_id: 'loose_training_ground', name: '试手空地', description: '许多散修在这里练手、试符、比身法，也顺便互相看看斤两。', room_type: '校场', risk_level: '冲突', pvp_enabled: true },
    { scene_id: 'loose_stone_forest', name: '乱石林', description: '一根根怪石把视线切得极碎，藏身和设伏都很方便。', room_type: '野外', risk_level: '历练' },
    { scene_id: 'loose_hidden_pool', name: '暗池', description: '池面看着平静，水下却时常有东西轻轻掠过去。', room_type: '野外', risk_level: '历练' },
    { scene_id: 'loose_guest_hall', name: '借宿大棚', description: '破旧的大棚里排着临时木铺，许多外来散修都靠这里凑合一夜。', room_type: '营地', risk_level: '安全' },
    { scene_id: 'loose_bamboo_stage', name: '竹台', description: '搭高的竹台原本用来讲价和宣告消息，后来也常被拿来公开邀战。', room_type: '营地', risk_level: '冲突', pvp_enabled: true },
  ],
});

pushLinearArea(scenes, scene_patches, {
  anchor_scene_id: 'huangfeng_foothill',
  anchor_direction: 'west',
  region_name: '灵兽山外岭',
  chapter: '灵兽山外岭',
  base_x: 4,
  base_y: 48,
  rooms: [
    { scene_id: 'spirit_beast_outer_gate', name: '灵兽山外门', description: '山门不似黄枫谷那般庄严，却随处都能见到灵兽活动留下的痕迹。', room_type: '宗门', risk_level: '安全' },
    { scene_id: 'spirit_beast_broker_lane', name: '兽材街', description: '店铺里挂着兽皮、喂料和虫笼，来往修士谈的都是饲养、巡山和换材。', room_type: '坊市', risk_level: '安全', shop_item_ids: ['beast_whistle', 'animal_treat', 'spirit_beast_manual'] },
    { scene_id: 'spirit_beast_beast_pen', name: '外山兽栏', description: '一圈圈木栏把低阶灵兽分开，粗心的人在这里总会被狠狠教训一次。', room_type: '宗门', risk_level: '安全' },
    { scene_id: 'spirit_beast_insect_garden', name: '灵虫圃', description: '湿热药气和虫鸣交叠成片，稍不留神就会把手伸进不该碰的地方。', room_type: '宗门', risk_level: '历练' },
    { scene_id: 'spirit_beast_herb_ridge', name: '饲草岭', description: '山坡上的灵草并不珍贵，却是外门弟子天天都得盯着的日常命根。', room_type: '野外', risk_level: '历练' },
    { scene_id: 'spirit_beast_worm_marsh', name: '虫泽', description: '泥水、枯草和灵虫壳在这里混成一层薄臭，久了连鞋底都像会活过来。', room_type: '野外', risk_level: '冲突', pvp_enabled: true },
    { scene_id: 'spirit_beast_taming_yard', name: '驯兽场', description: '许多年轻弟子在此练习驾驭和安抚，吼声、鞭声和命令声从早到晚不断。', room_type: '校场', risk_level: '历练' },
    { scene_id: 'spirit_beast_inner_path', name: '内山小径', description: '再往里便更接近内山地界，脚边时不时能见到不属于低阶灵兽的爪印。', room_type: '宗门', risk_level: '历练' },
    { scene_id: 'spirit_beast_bone_cave', name: '兽骨洞', description: '洞里挂着晒干的骨片与旧笼架，是外门最不讨喜但最缺不了的地方。', room_type: '洞窟', risk_level: '历练' },
    { scene_id: 'spirit_beast_cliff_nest', name: '崖巢', description: '峭壁之间有不少弃巢和碎卵壳，风一大便吹得满天乱响。', room_type: '野外', risk_level: '历练' },
    { scene_id: 'spirit_beast_stream', name: '驭兽溪', description: '山溪边经常有人带灵兽来饮水和清洗伤口，地面全是深深浅浅的爪印。', room_type: '野外', risk_level: '安全' },
    { scene_id: 'spirit_beast_hall', name: '外山执事堂', description: '真正的执事事务都在这里分派，谁偷懒、谁能扛事，一眼就看得出来。', room_type: '宗门', risk_level: '安全' },
  ],
});

pushLinearArea(scenes, scene_patches, {
  anchor_scene_id: 'tiannan_harbor',
  anchor_direction: 'down',
  region_name: '天南外港',
  chapter: '天南外港',
  base_x: 20,
  base_y: 90,
  rooms: [
    { scene_id: 'harbor_backbay', name: '后湾口', description: '主港之外的一片后湾，最适合小船出入，也最容易藏下见不得光的海货。', room_type: '港埠', risk_level: '安全' },
    { scene_id: 'harbor_fish_lane', name: '鱼棚街', description: '木台上满是鱼篓、盐桶和风干海货，叫卖声昼夜不断。', room_type: '坊市', risk_level: '安全', shop_item_ids: ['reef_lantern', 'sea_hunt_chart', 'harbor_dry_food'] },
    { scene_id: 'harbor_salt_house', name: '晒盐棚', description: '竹架上摊满盐壳和海藻，白得晃眼，脚下则永远黏着一层湿沙。', room_type: '港埠', risk_level: '安全' },
    { scene_id: 'harbor_net_field', name: '晒网场', description: '渔网一张张铺开，风一吹便像无数黑影在地上爬。', room_type: '港埠', risk_level: '历练' },
    { scene_id: 'harbor_cliff_ladder', name: '崖梯', description: '贴着海崖打出来的木梯一路往下，走快了很容易把人直接晃进浪里。', room_type: '海岸', risk_level: '历练' },
    { scene_id: 'harbor_drift_shore', name: '漂木滩', description: '退潮后常有漂木、断桅和奇怪碎片被卷上岸来。', room_type: '海岸', risk_level: '历练' },
    { scene_id: 'harbor_tide_pool', name: '潮池', description: '退潮后留下的一片片浅池里藏着不少小东西，也藏着不少咬人的嘴。', room_type: '海岸', risk_level: '历练' },
    { scene_id: 'harbor_reef_steps', name: '礁阶', description: '阶状礁石一直延伸到浪里，站得太深便很容易和海妖抢地盘。', room_type: '海域', risk_level: '冲突', pvp_enabled: true },
    { scene_id: 'harbor_lamp_tower', name: '后湾灯塔', description: '比主港风塔矮一些，却正好能看清后湾小船和偷渡路线。', room_type: '港埠', risk_level: '安全' },
    { scene_id: 'harbor_hidden_cove', name: '暗湾', description: '礁壁把这片小湾遮得极严，许多不想见光的交易都爱往这里挪。', room_type: '海岸', risk_level: '冲突', pvp_enabled: true },
  ],
});

pushLinearArea(scenes, scene_patches, {
  anchor_scene_id: 'chaos_sea_port',
  anchor_direction: 'west',
  region_name: '乱星近海群岛',
  chapter: '乱星近海群岛',
  base_x: 8,
  base_y: 98,
  rooms: [
    { scene_id: 'outer_isles_wharf', name: '群岛小埠', description: '近海群岛之间的小埠规模不大，却足够让修士补给、换货和招人出海。', room_type: '港埠', risk_level: '安全' },
    { scene_id: 'outer_isles_market', name: '珠市棚', description: '珠蚌、珊瑚、符纸和旧海图堆在一起，价钱每天都跟着潮水走。', room_type: '坊市', risk_level: '安全', shop_item_ids: ['moon_blessing_draught', 'storm_anchor_charm', 'sea_clear_pill'] },
    { scene_id: 'outer_isles_shell_beach', name: '贝滩', description: '细沙里满是碎壳，踩上去会发出咯吱咯吱的声音。', room_type: '海岸', risk_level: '历练' },
    { scene_id: 'outer_isles_palm_ridge', name: '风榈脊', description: '海风把树冠压得低低的，站在高处能一眼看到几条常走航线。', room_type: '海岛', risk_level: '历练' },
    { scene_id: 'outer_isles_storm_tree', name: '雷木坡', description: '这片坡地的树木总带着焦黑裂纹，像每个月都要挨上几回雷。', room_type: '海岛', risk_level: '历练' },
    { scene_id: 'outer_isles_lagoon', name: '月泻潟湖', description: '湖面在夜里会把月光揉得碎碎的，许多采珠人都信这里藏着好运。', room_type: '海域', risk_level: '历练' },
    { scene_id: 'outer_isles_coral_path', name: '珊瑚径', description: '锋利珊瑚从浅水一路顶出来，既像宝地也像陷阱。', room_type: '海域', risk_level: '历练' },
    { scene_id: 'outer_isles_black_reef', name: '黑礁外缘', description: '礁色发黑，浪头又急，是最容易爆发争抢和冲突的采集点。', room_type: '海域', risk_level: '冲突', pvp_enabled: true },
    { scene_id: 'outer_isles_moon_cove', name: '望月湾', description: '海湾弯成一轮半月，夜里看着极美，白天却常有妖物潜伏其间。', room_type: '海域', risk_level: '冲突', pvp_enabled: true },
    { scene_id: 'outer_isles_watch_altar', name: '听潮坛', description: '一座半旧石坛立在高处，岛民和散修都爱在这里看潮、看天、看人心。', room_type: '海岛', risk_level: '安全' },
  ],
});

pushLinearArea(scenes, scene_patches, {
  anchor_scene_id: 'chaos_sea_ship',
  anchor_direction: 'south',
  region_name: '外海深线',
  chapter: '结丹之门',
  base_x: 28,
  base_y: 106,
  rooms: [
    {
      scene_id: 'outer_sea_mid',
      name: '外海中层',
      description: '海雾渐沉，灵压已不是近海可比，潮下偶有青焰般的碎光一闪而过。',
      room_type: '海域',
      risk_level: '高危',
      pvp_enabled: true,
      landmark: '外海灵潮线',
      rumors: ['能在外海中层稳住船与神识的人，才算真正摸到了结丹前的海路门槛。'],
    },
    {
      scene_id: 'star_abyss',
      name: '星渊裂海',
      description: '深海灵流在此翻卷成裂，潮底像埋着碎星，许多凝婴灵物都只肯在这片黑水里现身。',
      room_type: '海域',
      risk_level: '高危',
      pvp_enabled: true,
      landmark: '星渊潮眼',
      rumors: ['若非已经摸到结丹后段的门槛，大多数人连星渊边上的第一道暗潮都过不去。'],
    },
  ],
});

pushLinearArea(scenes, scene_patches, {
  anchor_scene_id: 'xutian_inner_gate',
  anchor_direction: 'south',
  region_name: '虚天残区',
  chapter: '虚天残区',
  base_x: 78,
  base_y: 126,
  rooms: [
    { scene_id: 'xutian_void_rift', name: '裂隙口', description: '内殿玄门之后并非整齐殿宇，而是一道像被硬生生撕开的古禁裂口。', room_type: '秘境', risk_level: '高危' },
    { scene_id: 'xutian_ash_corridor', name: '灰廊', description: '长廊两侧不断有灰屑落下，像一整段空间都在缓慢剥离。', room_type: '秘境', risk_level: '高危' },
    { scene_id: 'xutian_shard_steps', name: '碎阶', description: '石阶断断续续浮在空中，脚下每一阶都带着轻微震颤。', room_type: '秘境', risk_level: '高危' },
    { scene_id: 'xutian_rune_garden', name: '残纹庭', description: '地面残纹交错成园，走错一步便会触到古禁余力。', room_type: '秘境', risk_level: '高危' },
    { scene_id: 'xutian_broken_stair', name: '断升台', description: '半截楼梯向上却又什么都接不上，只剩风从缝里反复灌进来。', room_type: '秘境', risk_level: '高危' },
    { scene_id: 'xutian_shadow_pool', name: '影池', description: '池水漆黑，像把周围一切光线都偷偷吃进去。', room_type: '秘境', risk_level: '高危' },
    { scene_id: 'xutian_silent_vault', name: '静库', description: '库室里安静得吓人，连自己的脚步声都像被什么东西吞了半截。', room_type: '秘境', risk_level: '高危' },
    { scene_id: 'xutian_crystal_bridge', name: '晶桥', description: '桥身裂着一道道细纹，桥下则是翻滚不定的幽暗虚流。', room_type: '秘境', risk_level: '高危', pvp_enabled: true },
    { scene_id: 'xutian_star_pit', name: '星坑', description: '坑底像埋着无数未熄的微光，又像有东西正盯着抬头的人。', room_type: '秘境', risk_level: '高危', pvp_enabled: true },
    { scene_id: 'xutian_endless_wall', name: '无尽壁', description: '一道看不到尽头的古壁横在最深处，许多残纹在其表面像活物般缓慢游走。', room_type: '秘境', risk_level: '高危' },
  ],
});

pushLinearArea(scenes, scene_patches, {
  anchor_scene_id: 'xutian_star_platform',
  anchor_direction: 'south',
  region_name: '虚天后脉',
  chapter: '古修残环',
  base_x: 62,
  base_y: 126,
  rooms: [
    {
      scene_id: 'core_flame_vein',
      name: '丹火灵脉',
      description: '地火暗涌，许多断裂槽线仍在吐出灼热灵息，是稳固金丹丹火最危险也最有效的地方。',
      room_type: '秘境',
      risk_level: '高危',
      landmark: '丹火裂槽',
      rumors: ['古修丹火不认侥幸，敢在这里取材的人，通常已经准备拿更高境界去赌命。'],
    },
    {
      scene_id: 'ancient_ruin_ring',
      name: '古修残环',
      description: '一圈残殿围着旧日禁制慢慢沉在暗光里，中央像还压着某种没散尽的古修意志。',
      room_type: '秘境',
      risk_level: '高危',
      pvp_enabled: true,
      landmark: '残环主殿',
      rumors: ['越靠近残环中央，越会觉得这里留下的并非死物，而是仍在挑选后来人的某种旧规矩。'],
    },
  ],
});

const npcs = [
  npc('xu_wanderer', '许游方', 'wanderer_camp', '在草棚间教散修怎么先活下去，再谈修行。', '许游方捧着药碗道：山路不长，胆要慢慢练，先把脚下这口气护住。', ['wanderer_dewleaf_task'], { role: 'mentor', description: '典型的散修前辈，不富不强，却很懂怎么让新人少死几回。' }),
  npc('herb_wife_lan', '蓝药娘', 'wanderer_creek', '一边洗药一边认草，看谁都先看手是不是稳。', '蓝药娘抬眼道：药不是贵就有用，认不准的人连黄精和毒草都分不清。', ['wanderer_resin_task'], { role: 'doctor', description: '散修草药线的引路人。' }),
  npc('temple_keeper_qiu', '丘守残', 'wanderer_old_temple', '守着半座残庙和一堆旧纸，像在等什么人记起这里。', '丘守残把竹签翻了翻：山里旧路多，旧消息也多，值不值钱看你会不会听。', [], { role: 'story', description: '替散修玩家提供旧闻、年历与地点线索。' }),
  npc('firefield_hunter_song', '宋火猎', 'wanderer_firefield', '擅长在焦土地翻树脂和野兽踪迹。', '宋火猎笑道：这地看着烫，实际最藏货，手快的人总能翻出点东西。', [], { role: 'hunter', description: '焦土地带的在地猎人。' }),
  npc('watch_oldman_he', '何望山', 'wanderer_watch_tower', '喜欢在望台上看路，也爱点评谁像会活得久的人。', '何望山咂着嘴道：你能活多远，先看你愿不愿意多看两眼路。', [], { role: 'watcher', description: '给新散修提供路况和风声的人。' }),

  npc('escort_captain_shen', '沈镖头', 'escort_post', '看人先看肩，再看眼神，最后才看你有没有灵根。', '沈镖头把腰刀一拍：修士也得先把人活明白了，护得住货再谈别的。', ['escort_token_task'], { role: 'quest_giver', description: '官道护送线的起点。' }),
  npc('road_peddler_liu', '刘路贩', 'escort_wayside', '总能在最荒的路边卖出最刚需的东西。', '刘路贩低声道：匪徒怕狠人，也怕有准备的人。', [], { role: 'vendor', description: '官道中段的流动补给点。' }),
  npc('relay_clerk_zhou', '周驿吏', 'relay_station', '比谁都清楚哪条路最近出事，也知道谁真能扛事。', '周驿吏翻着簿子道：路引、封签、换马、赏格，都得记清。', ['escort_seal_task'], { role: 'quest_giver', description: '护送与驿站循环的中枢。' }),
  npc('inn_keeper_su', '苏栈娘', 'night_inn', '做的是住店生意，听的却都是夜里不该乱传的话。', '苏栈娘笑了笑：官道上最贵的从不是床位，是有人愿意替你望一夜。', [], { role: 'host', description: '夜宿和护送线的人情节点。' }),
  npc('pass_guard_chen', '陈隘卒', 'yue_watch_pass', '守着越境望隘，对陌生修士总有三分怀疑。', '陈隘卒闷声道：再往前路更远，肯回头的人反而少。', [], { role: 'guard', description: '官道尽头的守关人。' }),

  npc('loose_master_wen', '温散人', 'loose_camp_square', '没有宗门，却教过许多人怎么在坊市和荒野之间活下来。', '温散人端着茶碗：散修最怕的不是穷，是不知道自己下一步该去哪。', ['loose_rumor_task'], { role: 'mentor', description: '散修棚市的公共导师。' }),
  npc('market_broker_hu', '胡牙人', 'loose_market_lane', '手里永远有两种价，一种给熟人，一种给不会问的人。', '胡牙人嘿嘿笑道：消息、货、旧图、旧账，我这儿都有一点。', [], { role: 'broker', description: '散修坊市里的中介节点。' }),
  npc('herb_tutor_qing', '青药师', 'loose_medicine_tent', '喜欢把便宜药材的用法讲得比贵药还细。', '青药师指着药锅：识得草木的人，穷也能活得久。', ['loose_stone_task'], { role: 'doctor', description: '太南散修坊的炼药导师。' }),
  npc('diviner_meng', '孟卜师', 'loose_diviner_mat', '摊开草席就能坐半天看人，不急着说话，也不急着赚钱。', '孟卜师低声道：你以为要找的是人，其实常常是路。', [], { role: 'diviner', description: '负责谣言、图鉴和识物线索的角色。' }),
  npc('guest_scribe_fan', '范记名', 'loose_guest_hall', '拿着一叠借宿簿和竹签，谁来谁走都记得比本人还清楚。', '范记名埋头记账：棚里每天都有人来，也每天都有人再也不回来。', [], { role: 'scribe', description: '散修公共区域的记录者。' }),

  npc('spirit_beast_steward', '灵兽山管事', 'spirit_beast_outer_gate', '正在筛选能不能吃苦、能不能先把小差事做稳的新弟子。', '灵兽山管事淡淡道：先把草料和虫槽照看明白，再谈你有没有资格进山。', ['spirit_feed_task'], { role: 'sect_master', sect_offer_id: 'spirit_beast_mountain', description: '灵兽山外门的正式引荐人。' }),
  npc('bug_trader_tao', '陶虫商', 'spirit_beast_broker_lane', '对虫壳、饵丸和小型兽材的行情最为熟稔。', '陶虫商压着嗓子：灵兽不只吃草，也吃钱。', [], { role: 'vendor', description: '灵兽山外门街市的商人。' }),
  npc('beast_feeder_zhou', '周饲兽', 'spirit_beast_beast_pen', '整天和草料、药丸、脏爪印打交道，说话却出奇耐心。', '周饲兽拍了拍栏杆：先把它们喂稳，别总想着拿它们显威风。', ['spirit_bug_task'], { role: 'sect_worker', description: '灵兽山饲养线的关键导师。' }),
  npc('insect_master_qin', '秦虫师', 'spirit_beast_insect_garden', '看灵虫的眼神比看人还认真。', '秦虫师皱眉道：别拿凡人看虫的眼光来看灵虫，那只会让你吃亏。', [], { role: 'teacher', description: '灵虫与采集线的专业导师。' }),
  npc('ridge_keeper_pei', '裴岭守', 'spirit_beast_herb_ridge', '负责看护饲草岭，最烦别人踩坏地面幼苗。', '裴岭守提醒你：草料不值钱，但断档了谁都得挨骂。', [], { role: 'guard', description: '灵兽山基础事务的看护者。' }),
  npc('marsh_scout_luo', '罗沼探', 'spirit_beast_worm_marsh', '长期在虫泽边巡看，身上总带着驱虫味。', '罗沼探抬脚抖了抖泥：虫泽最烦人的不是虫，是你一慌就会踩错地。', [], { role: 'scout', description: '虫泽风险与资源情报提供者。' }),
  npc('outer_deacon_du', '杜执事', 'spirit_beast_hall', '把外山差事排得极满，却也最清楚谁值得往上提。', '杜执事翻着册子：外门弟子先学会扛事，内山才会看你。', [], { role: 'deacon', description: '灵兽山外山事务总管。' }),

  npc('backbay_fisher_wu', '吴老渔', 'harbor_backbay', '一眼就看得出谁是真下过海的人，谁只是站在岸边做梦。', '吴老渔咧嘴道：海里给你的东西多，拿回去的也多。', ['harbor_shell_task'], { role: 'fisher', description: '外港海猎循环的起点。' }),
  npc('salt_house_keeper_lin', '林盐婆', 'harbor_salt_house', '她卖盐、收壳、记账，也最懂后湾哪些东西能换钱。', '林盐婆掂着盐壳道：别看这东西不起眼，炼药的人可离不了。', [], { role: 'vendor', description: '后湾海货与基础炼制的商人。' }),
  npc('net_master_peng', '彭网师', 'harbor_net_field', '补网的手法极快，眼神也总盯着海边有没有好货上岸。', '彭网师边系边说：海货是抢时间的，慢一步就什么都没了。', ['harbor_chart_task'], { role: 'quest_giver', description: '外港采集和海图任务的中间人。' }),
  npc('lamp_guard_xie', '谢灯守', 'harbor_lamp_tower', '靠看灯色和潮势混饭吃，顺便替人挑日子出湾。', '谢灯守望着海面：风要起来前，灯焰会先抖。', [], { role: 'watcher', description: '负责提示潮势与风向。' }),
  npc('hidden_diver_yao', '姚潜者', 'harbor_hidden_cove', '不喜欢在明处说话，倒很喜欢在暗湾谈交易和路线。', '姚潜者把声音压得很低：敢下暗湾，就别怕见到不该见的人。', [], { role: 'broker', description: '外港灰色交易与冲突区的接口人物。' }),

  npc('island_broker_shi', '施岛牙', 'outer_isles_wharf', '熟悉群岛间每条小航线和每个小埠的脾气。', '施岛牙笑道：近海群岛不大，可真懂路的人并不多。', ['outer_pearl_task'], { role: 'broker', description: '群岛路线与组队海猎的连接人。' }),
  npc('pearl_diver_lan', '蓝采珠', 'outer_isles_market', '她看珠子像看人一样，总能一眼看出成色和脾气。', '蓝采珠轻轻一抛珠壳：潮来潮去，最会说谎的是海面。', ['outer_coral_task'], { role: 'quest_giver', description: '群岛采珠与炼露线的核心人物。' }),
  npc('storm_scout_qi', '齐风候', 'outer_isles_storm_tree', '守着雷木坡观风看浪，脸上常带点被海风割出来的旧痕。', '齐风候抬头听风：有些风不是来吹船的，是来挑人的。', [], { role: 'scout', description: '风暴与海战节奏的提示者。' }),
  npc('reef_monk_yun', '云礁客', 'outer_isles_black_reef', '常坐在黑礁边看浪，像是在等合适的人来问路。', '云礁客道：黑礁边最值钱的从不是货，是你愿不愿意多等一阵浪。', [], { role: 'mentor', description: '黑礁冲突区的冷眼旁观者。' }),
  npc('altar_keeper_hua', '花守坛', 'outer_isles_watch_altar', '负责听潮坛香火与石面清理，对潮时极有执念。', '花守坛轻声道：看懂潮，就等于看懂一半近海人的命。', [], { role: 'keeper', description: '听潮坛与群岛时间节奏的守护者。' }),

  npc('rift_record_spirit', '记纹残灵', 'xutian_rune_garden', '像在无休止地记录残纹变化，见到来人也只会先看手里的碎片。', '记纹残灵空声道：把裂纹看清，把碎片归位，才有资格继续往里。', ['void_crystal_task'], { role: 'spirit', description: '虚天残区炼制与残纹任务的关键引导者。' }),
  npc('shard_collector_yi', '拾屑傀', 'xutian_shard_steps', '动作机械却格外专注，像被设定成了只能做这一件事。', '拾屑傀断断续续道：碎片……归档……不得遗失……', ['void_rune_task'], { role: 'puppet', description: '残区碎片回收的傀儡执行者。' }),
  npc('silent_vault_guide', '静库引灵', 'xutian_silent_vault', '连声音都像被收走了大半，只能用极轻的语气提醒闯入者。', '静库引灵低低道：越安静的地方，往往越危险。', [], { role: 'spirit', description: '静库区域的低声警告者。' }),
  npc('wall_listener_qiu', '听壁残识', 'xutian_endless_wall', '像有一部分意识还贴在古壁之内，只剩极少部分向外发声。', '听壁残识缓缓道：无尽壁之后不是路，是更多选择。', [], { role: 'spirit', description: '虚天残区深处的旧识回响。' }),
];

const monsters = [
  monster('ash_fox', '灰尾荒狐', 'wanderer_brush', 118, 22, 8, 22, 58, 'fox_tail_grass', 1, { description: '总爱在灌丛边绕着人转，真正动嘴时却比看上去更狠。' }),
  monster('cliff_ape', '裂石猿', 'wanderer_stone_slope', 136, 28, 12, 28, 74, 'stone_bark', 1, { description: '常抱着石块砸人，是山路上最讨厌的麻烦之一。' }),
  monster('firefield_mole', '焦土鼹', 'wanderer_firefield', 128, 24, 10, 24, 68, 'amber_resin', 1, { description: '喜欢在焦土地里乱钻，把本来不多的树脂翻得满地都是。', element: '火' }),
  monster('night_bat_pack', '夜蝠', 'wanderer_fox_den', 122, 23, 9, 24, 66, 'night_bat_wing', 1, { description: '天色越暗越兴奋，最爱贴着头顶乱扑。', kind: '奇虫', element: '风' }),
  monster('cold_pool_frog', '寒潭鼓蛙', 'wanderer_cold_pool', 130, 25, 10, 26, 70, 'clear_marrow_water', 1, { description: '叫声刺耳，弹跳极快，总能从最湿滑的位置扑出来。', element: '冰' }),

  monster('road_bandit', '截路匪', 'bandit_gully', 150, 30, 12, 34, 88, 'bandit_token', 1, { description: '依着地形和人多取胜，是官道护送线绕不开的老麻烦。' }),
  monster('dust_hound', '黄尘獒', 'escort_road', 142, 28, 12, 30, 82, 'road_mugwort', 1, { description: '总跟着货队和血味跑，耐力出奇地好。' }),
  monster('vulture_spirit', '秃岭鹫', 'broken_bridge', 138, 27, 11, 28, 80, 'white_gull_feather', 1, { description: '会盯着摔倒的人盘旋，眼神阴得让人发寒。', element: '风' }),
  monster('ditch_ghoul', '沟边腐魇', 'border_field', 156, 31, 13, 36, 92, 'bridge_moss', 1, { description: '像是荒地和烂泥缠成的人影，最擅长拖人脚步。', element: '蚀' }),
  monster('pass_lurker', '隘口伏客', 'yue_watch_pass', 162, 32, 14, 40, 96, 'caravan_seal', 1, { description: '躲在关隘死角的旧匪，动作比荒野妖物更像人。' }),

  monster('rogue_puppet', '废傀', 'loose_training_ground', 158, 32, 14, 38, 96, 'spirit_ink', 1, { description: '散修们拿来练手的旧傀儡，偶尔会因灵线紊乱变得异常凶。' }),
  monster('stone_lizard', '石脊蜥', 'loose_stone_forest', 164, 33, 14, 40, 100, 'ink_shell', 1, { description: '喜欢在怪石背阴处伏着，等人从身边经过再突然蹿出。' }),
  monster('pool_bug', '暗池蜉', 'loose_hidden_pool', 148, 30, 12, 36, 94, 'moon_shell', 1, { description: '浮在水面时像漂叶，真正近身后才露出锋利口器。', kind: '奇虫', element: '毒' }),
  monster('bamboo_shadow', '竹影魅', 'loose_bamboo_stage', 170, 34, 14, 42, 106, 'bamboo_mark', 1, { description: '会借着竹影和风声混淆方向，让人越来越看不清对手。', element: '蚀' }),
  monster('camp_mink', '贼眼貂', 'loose_guest_hall', 150, 31, 12, 34, 90, 'shadow_weed', 1, { description: '专偷住客包裹的小兽，速度快得不像话。' }),

  monster('horn_beast_cub', '角犀幼兽', 'spirit_beast_taming_yard', 188, 36, 18, 54, 128, 'beast_bone', 1, { description: '看似还未长成，撞起来却一点都不含糊。' }),
  monster('ink_moth', '墨翅蛾', 'spirit_beast_insect_garden', 176, 34, 14, 46, 118, 'ink_moth_wing', 1, { description: '扑扇时会抖落细粉，一旦吸进去就很容易头晕。', kind: '奇虫', element: '毒' }),
  monster('marsh_centipede', '沼节蜈', 'spirit_beast_worm_marsh', 182, 35, 15, 48, 122, 'spirit_beast_feed', 1, { description: '在烂泥和枯草里钻得极快，最爱从脚边往上爬。', kind: '奇虫', element: '毒' }),
  monster('cliff_eagle', '崖栖鹰', 'spirit_beast_cliff_nest', 194, 37, 16, 56, 132, 'cliff_egg_shell', 1, { description: '专从高处俯冲，若不小心就会连人带包一起被掀翻。', element: '风' }),
  monster('bone_cave_mole', '骨洞掘鼠', 'spirit_beast_bone_cave', 186, 35, 16, 50, 124, 'beast_bone', 1, { description: '长期啃食骨渣和饲料袋，牙口硬得惊人。' }),
  monster('stream_turtle', '驭溪龟', 'spirit_beast_stream', 206, 34, 24, 60, 138, 'clear_marrow_water', 1, { description: '慢归慢，可真要打起来就像一块会砸人的大石头。', element: '水' }),

  monster('tide_crab', '潮壳蟹', 'harbor_tide_pool', 178, 34, 18, 44, 114, 'salt_shell', 1, { description: '退潮后最容易遇上的硬壳麻烦，脾气也和壳一样硬。', element: '水' }),
  monster('reef_eel', '礁鳗', 'harbor_reef_steps', 184, 36, 15, 48, 120, 'tide_sand', 1, { description: '贴着礁石乱窜，触到人时会带来一阵麻意。', element: '雷' }),
  monster('gull_demon', '白鸥妖', 'harbor_cliff_ladder', 170, 33, 13, 42, 108, 'white_gull_feather', 1, { description: '盘旋时叫声刺耳，最爱掀翻手里正抱着东西的人。', element: '风' }),
  monster('drift_manta', '漂鳍鲼', 'harbor_hidden_cove', 196, 38, 17, 54, 130, 'drift_fin', 1, { description: '借着暗湾海浪滑行，一扑过来像一整片湿布兜头盖下。', element: '水' }),
  monster('salt_leech', '盐沼蚀虫', 'harbor_salt_house', 166, 32, 12, 40, 104, 'sea_kelp_bundle', 1, { description: '在盐水和烂藻里活得极好，沾上就很难甩掉。', kind: '奇虫', element: '蚀' }),

  monster('shell_lobster', '裂壳虾', 'outer_isles_shell_beach', 202, 39, 18, 62, 144, 'moon_pearl', 1, { description: '壳裂得像石头，钳子却快得像刀。', element: '水' }),
  monster('storm_monkey', '风猴', 'outer_isles_storm_tree', 196, 38, 16, 58, 138, 'storm_coral', 1, { description: '总借着坡地和风势跳来跳去，根本不和人正面站住。', element: '风' }),
  monster('reef_sharkling', '黑礁幼鲨', 'outer_isles_black_reef', 212, 42, 18, 70, 154, 'black_reef_tooth', 1, { description: '虽然还不算真正可怕的大妖，却足够让采集的人手忙脚乱。', element: '水' }),
  monster('lagoon_bug', '潟湖月蜉', 'outer_isles_lagoon', 188, 36, 14, 54, 128, 'moon_salt', 1, { description: '月夜活跃得最厉害，翅光像碎月一样晃眼。', kind: '奇虫', element: '冰' }),
  monster('altar_jelly', '坛影水母', 'outer_isles_watch_altar', 204, 40, 16, 66, 148, 'sea_kelp_bundle', 1, { description: '半透明的灵体总在听潮坛附近飘来荡去，偶尔会突然贴近。', kind: '奇虫', element: '雷' }),
  monster('outer_sea_ray', '外海裂鲼', 'outer_sea_mid', 248, 48, 24, 112, 224, 'azure_flame_crystal', 1, { description: '它总借外海暗流突然翻身，鳍边像擦出一层青焰般的寒光。', kind: '海妖', element: '水' }),
  monster('star_abyss_eel', '星渊鳗皇', 'star_abyss', 304, 60, 28, 144, 276, 'star_sea_heart', 1, { description: '深海电光顺着它的骨脊一路闪灭，看久了连神识都会被那节律牵着走。', kind: '海妖', element: '雷' }),

  monster('ash_guard', '灰甲守影', 'xutian_ash_corridor', 236, 46, 24, 96, 196, 'void_crystal', 1, { description: '像由古殿落灰和残甲拼起来的守卫，脚步沉得让地面都发闷。', element: '蚀' }),
  monster('rune_mite', '纹蚀', 'xutian_rune_garden', 220, 43, 20, 88, 186, 'rift_rune', 1, { description: '专吃残纹边缘的灵屑，小归小，却总成群结队。', kind: '奇虫', element: '蚀' }),
  monster('shadow_eye', '影目', 'xutian_shadow_pool', 228, 44, 20, 92, 190, 'shadow_weed', 1, { description: '池面下像有眼睛在慢慢转动，被盯久了连心神都会发凉。', element: '毒' }),
  monster('crystal_hound', '晶裂犬', 'xutian_crystal_bridge', 244, 48, 24, 102, 202, 'star_shard', 1, { description: '踩过晶桥时会拖出一串刺耳刮擦声，让人根本不敢分心。', element: '冰' }),
  monster('bridge_wisp', '桥灵火', 'xutian_star_pit', 230, 45, 18, 94, 192, 'void_dust', 1, { description: '飘忽不定的一团灵火，总爱在人刚准备稳住时贴上来。', element: '火' }),
  monster('pit_spider', '坑渊蛛', 'xutian_endless_wall', 252, 49, 24, 108, 208, 'ash_silk', 1, { description: '在无尽壁附近结网，像是在替古禁收走最后一点活气。', kind: '奇虫', element: '毒' }),
  monster('flame_vein_wisp', '丹火流萤', 'core_flame_vein', 262, 52, 22, 118, 232, 'purple_core_sand', 1, { description: '它像被地火吹起来的一簇活焰，扑近时会把经脉都烫得微微发麻。', kind: '异灵', element: '火' }),
  monster('ruin_ring_guard', '残环守影', 'ancient_ruin_ring', 278, 56, 26, 132, 248, 'soul_warming_jade', 1, { description: '像古修残念与石甲捏在一起的守卫，只要踏进残环中央便会慢慢朝你合围。', element: '蚀' }),
];

const resource_nodes = [
  resourceNode('wanderer_dewleaf_patch', '露叶草簇', 'wanderer_creek', '溪边最容易找到的一小簇露叶草。', 'dewleaf_herb', 1),
  resourceNode('wanderer_stone_bark_tree', '裂皮老树', 'wanderer_stone_slope', '树皮坚硬却带着药性，砍取时要花点力气。', 'stone_bark', 1),
  resourceNode('wanderer_fox_grass', '狐尾草窝', 'wanderer_brush', '灌木阴影里常能翻到一把狐尾草。', 'fox_tail_grass', 1),
  resourceNode('wanderer_cold_spring', '寒泉', 'wanderer_cold_pool', '冷得刺骨的山泉，取水时连手腕都要跟着发麻。', 'clear_marrow_water', 1),

  resourceNode('escort_mugwort_field', '路艾地', 'escort_road', '官道边生命力最顽强的一片路艾。', 'road_mugwort', 1),
  resourceNode('escort_bridge_moss', '桥石苔痕', 'broken_bridge', '只有旧桥最潮的地方才长得出来。', 'bridge_moss', 1),
  resourceNode('escort_ore_sieve', '铁砂浅沟', 'relay_station', '驿站旁挖出的浅沟里偶尔能筛到一点铁砂。', 'relay_ore', 1),
  resourceNode('escort_field_saltweed', '荒田盐草', 'border_field', '盐碱地里顽强生长的杂草，药性却意外不差。', 'road_mugwort', 1),

  resourceNode('loose_ink_basin', '调墨盆', 'loose_medicine_tent', '许多散修调符、记图都会用到的灵墨。', 'spirit_ink', 1),
  resourceNode('loose_shadow_weed_patch', '影草角', 'loose_stone_forest', '石林最背阴的角落总会长出这一小丛影草。', 'shadow_weed', 1),
  resourceNode('loose_pool_shells', '池边月壳', 'loose_hidden_pool', '暗池边湿滑石缝里嵌着几枚薄壳。', 'moon_shell', 1),
  resourceNode('loose_bamboo_slips', '竹签堆', 'loose_bamboo_stage', '竹台边常会散落着写过记号的旧竹签。', 'bamboo_mark', 1),

  resourceNode('spirit_beast_forage', '饲草地', 'spirit_beast_herb_ridge', '灵兽山外门每天都要收割一遍的基础饲草。', 'spirit_beast_feed', 1),
  resourceNode('spirit_beast_bones', '骨片堆', 'spirit_beast_bone_cave', '晒得发白的骨片堆，是炼制和喂养都用得上的材料。', 'beast_bone', 1),
  resourceNode('spirit_beast_moth_wings', '蛾翼残叶', 'spirit_beast_insect_garden', '灵虫圃边常能捡到沾着粉的蛾翼。', 'ink_moth_wing', 1),
  resourceNode('spirit_beast_cliff_shell', '崖卵壳', 'spirit_beast_cliff_nest', '高崖弃巢里常残留着坚韧卵壳。', 'cliff_egg_shell', 1),
  resourceNode('spirit_beast_stream_water', '驭兽溪清泉', 'spirit_beast_stream', '给灵兽饮用的山溪水格外清冽。', 'clear_marrow_water', 1),

  resourceNode('harbor_salt_pile', '盐壳堆', 'harbor_salt_house', '晒盐棚下最常见的一堆粗盐壳。', 'salt_shell', 1),
  resourceNode('harbor_tide_sand', '潮砂带', 'harbor_reef_steps', '潮起潮落之间会留下一片湿润灵砂。', 'tide_sand', 1),
  resourceNode('harbor_gull_feather', '鸥翎窝', 'harbor_cliff_ladder', '海崖缝里塞满了白鸥掉落的羽毛。', 'white_gull_feather', 1),
  resourceNode('harbor_kelp_bundle', '海灵藻', 'harbor_backbay', '后湾浅水里时常缠上一束束海灵藻。', 'sea_kelp_bundle', 1),

  resourceNode('outer_pearl_bed', '珠蚌浅床', 'outer_isles_shell_beach', '退潮后才能摸到的一小片珠蚌床。', 'moon_pearl', 1),
  resourceNode('outer_coral_branch', '风暴珊瑚枝', 'outer_isles_coral_path', '红得发暗的珊瑚枝被海水磨得很滑。', 'storm_coral', 1),
  resourceNode('outer_black_tooth', '黑礁齿痕', 'outer_isles_black_reef', '黑礁边常能翻到猎物或海妖遗落的硬齿。', 'black_reef_tooth', 1),
  resourceNode('outer_moon_salt', '月盐滩', 'outer_isles_moon_cove', '月夜后海风一干，最容易留下细细一层月盐。', 'moon_salt', 1),
  resourceNode('outer_tide_kelp', '潮藻槽', 'outer_isles_watch_altar', '听潮坛下方的一片水槽里总能摸到海藻。', 'sea_kelp_bundle', 1),
  resourceNode('outer_sea_crystal_reef', '青焰礁髓', 'outer_sea_mid', '外海中层偶有礁髓在潮下亮起青焰般的碎光，正适合作为稳丹主材。', 'azure_flame_crystal', 1),
  resourceNode('star_abyss_tide_eye', '星心潮眼', 'star_abyss', '只有潮眼最短暂张开的一刻，才来得及摸出那一枚真正像活着的心珀。', 'star_sea_heart', 1),

  resourceNode('void_crystal_sand', '裂隙晶砂', 'xutian_void_rift', '裂隙边不断掉落的细小晶砂。', 'void_crystal', 1),
  resourceNode('void_rune_shards', '残纹碎片', 'xutian_rune_garden', '庭中每一道残纹边缘都可能剥下细碎片。', 'rift_rune', 1),
  resourceNode('void_star_shard', '星屑裂片', 'xutian_star_pit', '从坑底微光中剥离出来的小片星屑。', 'star_shard', 1),
  resourceNode('void_ash_silk', '灰丝网', 'xutian_endless_wall', '贴着古壁结出的细密灰丝，拉扯时总像会发出低鸣。', 'ash_silk', 1),
  resourceNode('core_flame_sandfall', '丹砂涌脉', 'core_flame_vein', '断裂火脉里不断有细砂般的丹火残屑涌上来，捞取时最怕火息反扑。', 'purple_core_sand', 1),
  resourceNode('ruin_ring_jade_seat', '养魂玉座', 'ancient_ruin_ring', '残环偏殿的玉座里仍残留温养神魂的旧意，偶尔能剥下几片古玉。', 'soul_warming_jade', 1),
];

const ground_loots = [
  groundLoot('wanderer_old_slip', 'wanderer_old_temple', 'bamboo_mark', 1, '残庙角落压着一枚旧竹签。'),
  groundLoot('escort_stolen_seal', 'granary_yard', 'caravan_seal', 1, '粮场角落落着一枚被踩脏的封签。'),
  groundLoot('loose_guest_note', 'loose_guest_hall', 'spirit_ink', 1, '木铺下塞着一小包调好的灵墨。'),
  groundLoot('spirit_beast_feed_sack', 'spirit_beast_beast_pen', 'spirit_beast_feed', 1, '兽栏边滚着一袋散开的草料。'),
  groundLoot('harbor_drift_fin_loot', 'harbor_drift_shore', 'drift_fin', 1, '漂木间卡着一片薄薄的妖鱼鳍。'),
  groundLoot('outer_altar_pearl', 'outer_isles_watch_altar', 'moon_pearl', 1, '石坛凹槽里躺着一枚被潮水冲亮的珠子。'),
  groundLoot('void_vault_dust', 'xutian_silent_vault', 'void_dust', 1, '静库角落里积着一小团异常细密的灰。'),
  groundLoot('outer_sea_route_trace', 'outer_sea_mid', 'storm_route_chart', 1, '一片被海水反复打湿的航纹残页卡在礁缝里，仍能看出深海方向的旧注记。'),
  groundLoot('ruin_ring_cache_token', 'ancient_ruin_ring', 'treasure_cache_token', 1, '残环主殿石台下压着一枚古库令牌，像是替后来人留下的一次试探。'),
];

const hazards = [
  hazard('wanderer_fire_ash', 'wanderer_firefield', '焦土余灼', '翻挖焦土地时稍不留神就会被余热灼伤。', { hp_cost: 10, resist_key: 'resist_fire' }),
  hazard('escort_gully_ambush', 'bandit_gully', '伏路杀机', '沟壑地形太适合伏击，神念一散就容易吃亏。', { hp_cost: 8, sta_cost: 10, resist_key: 'resist_pierce' }),
  hazard('loose_stone_echo', 'loose_stone_forest', '乱石回响', '石林里的回响会让人对方向和距离产生短暂误判。', { sen_cost: 10, resist_key: 'resist_wind' }),
  hazard('spirit_beast_marsh_gas', 'spirit_beast_worm_marsh', '虫泽湿瘴', '虫泽里总浮着一层令人不舒服的潮湿腥气。', { hp_cost: 12, sta_cost: 10, resist_key: 'resist_poison' }),
  hazard('spirit_beast_wild_roar', 'spirit_beast_inner_path', '兽压余波', '偶尔从内山深处传来的兽吼会让人心神一震。', { sen_cost: 12, resist_key: 'resist_wind' }),
  hazard('harbor_dark_tide', 'harbor_hidden_cove', '暗湾回潮', '暗湾里的暗流不像表面那样平静，失神就会吃一嘴苦水。', { hp_cost: 10, mana_cost: 10, resist_key: 'resist_water' }),
  hazard('outer_black_reef_surge', 'outer_isles_black_reef', '黑礁浪涌', '浪头会突然更换方向，逼得人不得不临时改步。', { hp_cost: 12, mana_cost: 12, resist_key: 'resist_water' }),
  hazard('outer_sea_pressure', 'outer_sea_mid', '外海灵压', '外海中层的灵压会顺着潮路一阵阵压来，稍不留神便连呼吸都乱掉。', { mana_cost: 14, sta_cost: 12, resist_key: 'resist_water' }),
  hazard('star_abyss_backflow', 'star_abyss', '星渊回潮', '潮底灵流会突然倒卷，像要把站得太深的人整个拖回深渊里。', { hp_cost: 16, mana_cost: 16, resist_key: 'resist_water' }),
  hazard('void_rift_pressure', 'xutian_void_rift', '裂隙灵压', '裂隙口的灵压会持续把人往后推，也持续磨掉护体。', { hp_cost: 14, mana_cost: 14, resist_key: 'resist_corrosion' }),
  hazard('void_wall_whisper', 'xutian_endless_wall', '古壁低鸣', '无尽壁上传来的低鸣会一点点侵蚀心神。', { sen_cost: 16, mana_cost: 10, resist_key: 'resist_ice' }),
  hazard('core_flame_burst', 'core_flame_vein', '丹火逆涌', '一旦踩错火脉节律，地火便会沿残槽猛地回喷，把经脉都烫得发颤。', { hp_cost: 14, mana_cost: 12, resist_key: 'resist_fire' }),
  hazard('ruin_ring_whisper', 'ancient_ruin_ring', '残环回啸', '残环里回荡的旧日咒声会一点点啃咬神识，让人分不清哪里是出口。', { sen_cost: 18, mana_cost: 8, resist_key: 'resist_corrosion' }),
];

const quests = [
  quest('wanderer_dewleaf_task', '露叶试手', '许游方想看看你会不会做最基础的采药活，把一株露叶草带回去再说。', 'xu_wanderer', 'xu_wanderer', 'dewleaf_herb', 1, 38, 68, 'wanderer_powder', 1, { chapter: '散修启途' }),
  quest('wanderer_resin_task', '焦土取脂', '蓝药娘需要一点黄琥脂稳住药性，若你敢去焦土地翻出来，她会承你这份情。', 'herb_wife_lan', 'herb_wife_lan', 'amber_resin', 1, 46, 78, 'traveler_boots', 1, { chapter: '散修启途' }),
  quest('escort_token_task', '官道清匪', '沈镖头要你带回一枚匪徒腰牌，先证明你敢在官道上正面扛事。', 'escort_captain_shen', 'escort_captain_shen', 'bandit_token', 1, 60, 104, 'escort_pill', 1, { chapter: '越京官道' }),
  quest('escort_seal_task', '封签送驿', '周驿吏让你找回失落的货队封签，好让一支临时货队今晚还能继续启程。', 'relay_clerk_zhou', 'relay_clerk_zhou', 'caravan_seal', 1, 72, 116, 'hunting_bow', 1, { chapter: '越京官道' }),
  quest('loose_rumor_task', '棚市风声', '温散人让你替他收一枚竹签记号回来，看看今天棚里到底又传开了什么风声。', 'loose_master_wen', 'loose_master_wen', 'bamboo_mark', 1, 54, 96, 'identify_manual', 1, { chapter: '太南散修坊' }),
  quest('loose_stone_task', '石林识物', '青药师要一片月壳做演示药引，顺便看看你在乱石林里能不能稳得住手。', 'herb_tutor_qing', 'herb_tutor_qing', 'moon_shell', 1, 66, 108, 'loose_market_talisman', 1, { chapter: '太南散修坊' }),
  quest('spirit_feed_task', '外门草料差', '灵兽山管事要你先补上一袋草料，做成了才有资格继续谈入门。', 'spirit_beast_steward', 'spirit_beast_steward', 'spirit_beast_feed', 1, 88, 146, 'spirit_beast_manual', 1, { reward_sect_id: 'spirit_beast_mountain', chapter: '灵兽山外岭' }),
  quest('spirit_bug_task', '灵虫粉翅', '周饲兽需要几片墨蛾翅调药，若你能带回，他愿教你更省事的喂养法。', 'beast_feeder_zhou', 'beast_feeder_zhou', 'ink_moth_wing', 1, 84, 138, 'beast_whistle', 1, { chapter: '灵兽山外岭' }),
  quest('harbor_shell_task', '后湾盐壳', '吴老渔缺一把能磨药的盐壳，带回来就算你真开始懂后湾活路了。', 'backbay_fisher_wu', 'backbay_fisher_wu', 'salt_shell', 1, 82, 132, 'reef_lantern', 1, { chapter: '天南外港' }),
  quest('harbor_chart_task', '后湾海图', '彭网师想把后湾几处潮路记进图里，你先替他带回一片漂鳍做标记样本。', 'net_master_peng', 'net_master_peng', 'drift_fin', 1, 90, 146, 'sea_hunt_chart', 1, { chapter: '天南外港' }),
  quest('outer_pearl_task', '群岛采珠', '施岛牙想看看你有没有近海手艺，先去替他摸回一枚望月珠。', 'island_broker_shi', 'island_broker_shi', 'moon_pearl', 1, 102, 168, 'moon_blessing_draught', 1, { chapter: '乱星近海群岛' }),
  quest('outer_coral_task', '风暴珊瑚', '蓝采珠正缺一截能稳药性的风暴珊瑚，她愿拿出真正有用的避风货来换。', 'pearl_diver_lan', 'pearl_diver_lan', 'storm_coral', 1, 110, 176, 'storm_anchor_charm', 1, { chapter: '乱星近海群岛' }),
  quest('void_crystal_task', '裂隙晶砂', '记纹残灵只认裂隙晶砂，能带回来的人才算真正看见了这里的危险。', 'rift_record_spirit', 'rift_record_spirit', 'void_crystal', 1, 138, 220, 'void_guard_charm', 1, { chapter: '虚天残区' }),
  quest('void_rune_task', '残纹归位', '拾屑傀要你带回一枚残纹碎片，像是还在执行许多年前就定好的命令。', 'shard_collector_yi', 'shard_collector_yi', 'rift_rune', 1, 146, 232, 'riftbreaking_note', 1, { chapter: '虚天残区' }),
];

export default {
  backgrounds,
  scene_patches,
  scenes,
  npcs,
  monsters,
  resource_nodes,
  ground_loots,
  hazards,
  items,
  skills,
  spells,
  recipes,
  formations,
  quests,
  manual_codex_entries,
};
