import fs from 'node:fs/promises';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

import worldMainline from '../doc/mud/source/world_mainline.mjs';
import charactersSects from '../doc/mud/source/characters_sects.mjs';
import creaturesResources from '../doc/mud/source/creatures_resources.mjs';
import itemsSystems from '../doc/mud/source/items_systems.mjs';
import pureMudExpansion from '../doc/mud/source/pure_mud_shared_world.mjs';
import mudHelpManual from '../doc/mud/source/mud_help_manual.mjs';
import mudJobsRumors from '../doc/mud/source/mud_jobs_rumors.mjs';
import mudTitlesFactions from '../doc/mud/source/mud_titles_factions.mjs';

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);
const repoRoot = path.resolve(__dirname, '..');

function ensureArray(value, label) {
  if (!Array.isArray(value)) {
    throw new Error(`${label} must be an array`);
  }
  return value;
}

function buildIdMap(list, key, label) {
  const map = new Map();
  for (const item of ensureArray(list, label)) {
    const id = String(item?.[key] ?? '');
    if (!id) {
      throw new Error(`${label} contains item without ${key}`);
    }
    if (map.has(id)) {
      throw new Error(`${label} contains duplicate id: ${id}`);
    }
    map.set(id, item);
  }
  return map;
}

function mustExist(map, id, label) {
  if (!id) {
    return;
  }
  if (!map.has(id)) {
    throw new Error(`missing ${label}: ${id}`);
  }
}

function pushGrouped(grouped, key, value) {
  if (!grouped.has(key)) {
    grouped.set(key, []);
  }
  grouped.get(key).push(value);
}

function uniqueStrings(values) {
  return [...new Set(values.filter(Boolean))];
}

function mergeArrays(...groups) {
  return groups.flatMap((group) => (Array.isArray(group) ? group : []));
}

function defaultRoomType(scene) {
  if (scene.room_type) {
    return scene.room_type;
  }
  if ((scene.scene_id ?? '').includes('market') || (scene.scene_id ?? '').includes('street')) {
    return '坊市';
  }
  if ((scene.scene_id ?? '').includes('dock') || (scene.scene_id ?? '').includes('port') || (scene.scene_id ?? '').includes('harbor')) {
    return '港埠';
  }
  if ((scene.scene_id ?? '').includes('hall') || (scene.scene_id ?? '').includes('gate') || (scene.scene_id ?? '').includes('scripture')) {
    return '宗门';
  }
  if ((scene.scene_id ?? '').includes('ship') || (scene.scene_id ?? '').includes('route') || (scene.scene_id ?? '').includes('reef')) {
    return '海域';
  }
  if ((scene.scene_id ?? '').includes('xutian') || (scene.scene_id ?? '').includes('blood')) {
    return '秘境';
  }
  if ((scene.scene_id ?? '').includes('forest') || (scene.scene_id ?? '').includes('slope') || (scene.scene_id ?? '').includes('marsh')) {
    return '野外';
  }
  return '房间';
}

function defaultRiskLevel(scene) {
  if (scene.risk_level) {
    return scene.risk_level;
  }
  const danger = (scene.monster_ids?.length ?? 0) + (scene.hazard_ids?.length ?? 0);
  if (scene.pvp_enabled) {
    return '冲突';
  }
  if (danger >= 3) {
    return '高危';
  }
  if (danger >= 1) {
    return '历练';
  }
  return '安全';
}

function defaultRumors(scene) {
  if (Array.isArray(scene.rumors) && scene.rumors.length > 0) {
    return scene.rumors;
  }
  const hints = [];
  if (scene.chapter) {
    hints.push(`此地常有人提起「${scene.chapter}」相关的旧闻。`);
  }
  if ((scene.monster_ids?.length ?? 0) > 0) {
    hints.push('附近修士常交换妖兽出没与掉落材料的消息。');
  }
  if ((scene.resource_node_ids?.length ?? 0) > 0) {
    hints.push('这里的采集点很受散修与药师关注。');
  }
  if (scene.pvp_enabled) {
    hints.push('此地偶有同道因资源与悬赏起争执。');
  }
  return hints;
}

function defaultRoomLayer(scene) {
  if (scene.room_layer) {
    return scene.room_layer;
  }

  const id = String(scene.scene_id ?? '');
  const region = String(scene.region_name ?? '');
  const safeIds = [
    'qixuan_',
    'jiayuan_',
    'mofu_',
    'tainan_',
    'xin_house',
    'talisman_street',
    'array_lane',
    'loose_',
    'huangfeng_outpost',
    'huangfeng_hall',
    'huangfeng_medicine_terrace',
    'huangfeng_scripture',
    'spirit_beast_outer_gate',
    'spirit_beast_beast_pen',
    'spirit_beast_',
    'tiannan_harbor',
    'harbor_backbay',
  ];
  if (safeIds.some((prefix) => id === prefix || id.startsWith(prefix)) || /七玄门|嘉元城|墨府|太南谷|黄枫谷|灵兽山|天南海岸/.test(region)) {
    return '新手安全圈';
  }

  if (
    id.startsWith('blood_') ||
    id.startsWith('chaos_sea_') ||
    id.startsWith('outer_isles_') ||
    id.startsWith('xutian_') ||
    id.startsWith('escort_') ||
    id.startsWith('wanderer_') ||
    id.startsWith('harbor_') ||
    id === 'storm_route' ||
    id === 'reef_shore' ||
    id === 'demon_fish_nest'
  ) {
    if (
      id.includes('deep') ||
      id.includes('void') ||
      id.includes('black_reef') ||
      id.includes('star_pit') ||
      id.includes('endless_wall') ||
      id.includes('crystal_bridge') ||
      id === 'storm_route' ||
      id === 'blood_swamp' ||
      id === 'blood_orchid_vale'
    ) {
      return '筑基冲刺圈';
    }
    return '成长历练圈';
  }

  return '成长历练圈';
}

function defaultLoopTags(scene) {
  if (Array.isArray(scene.loop_tags) && scene.loop_tags.length > 0) {
    return uniqueStrings(scene.loop_tags);
  }

  const tags = [];
  const id = String(scene.scene_id ?? '');
  const region = String(scene.region_name ?? '');

  if ((scene.resource_node_ids?.length ?? 0) > 0 || /药|坊|棚|谷|圃|台/.test(region + id)) {
    tags.push('采药炼丹');
  }
  if (/escort|relay|road|post|station|gate|market|harbor|wharf|dock/i.test(id)) {
    tags.push('护送跑商');
  }
  if ((scene.monster_ids?.length ?? 0) > 0 && !/chaos_sea|outer_isles|harbor|reef|port/i.test(id)) {
    tags.push('巡山悬赏');
  }
  if (/qixuan|huangfeng|spirit_beast/i.test(id) || /七玄门|黄枫谷|灵兽山/.test(region)) {
    tags.push('门派事务');
  }
  if (/chaos_sea|outer_isles|harbor|reef|port|storm/i.test(id) || /海/.test(region)) {
    tags.push('海猎采珠');
  }
  if (/blood|xutian|void|star|forbidden|ruin/i.test(id) || scene.risk_level === '高危' || scene.pvp_enabled) {
    tags.push('残区探禁');
  }

  if (tags.length === 0) {
    tags.push('采药炼丹');
  }
  return uniqueStrings(tags);
}

function normalizeSceneMetadata(scenes) {
  for (const scene of scenes) {
    scene.room_type = defaultRoomType(scene);
    scene.risk_level = defaultRiskLevel(scene);
    scene.landmark = scene.landmark ?? scene.name;
    scene.room_layer = defaultRoomLayer(scene);
    scene.pvp_enabled = Boolean(scene.pvp_enabled);
    scene.rumors = defaultRumors(scene);
    scene.loop_tags = defaultLoopTags(scene);
    scene.service_tags = uniqueStrings((scene.service_tags ?? []).map((item) => String(item ?? '').trim()));
    scene.rumor_topics = uniqueStrings((scene.rumor_topics ?? []).map((item) => String(item ?? '').trim()));
    scene.board_available = Boolean(scene.board_available);
    scene.mentor_ids = uniqueStrings((scene.mentor_ids ?? []).map((item) => String(item ?? '').trim()));
  }
}

function applyScenePatches(scenes, patches) {
  if (!Array.isArray(patches) || patches.length === 0) {
    return;
  }

  const sceneMap = buildIdMap(scenes, 'scene_id', 'scenes');
  for (const patch of patches) {
    const sceneId = String(patch?.scene_id ?? '');
    if (!sceneId) {
      throw new Error('scene_patches contains patch without scene_id');
    }

    const scene = sceneMap.get(sceneId);
    if (!scene) {
      throw new Error(`scene_patches references missing scene: ${sceneId}`);
    }

    if (patch.exits && typeof patch.exits === 'object') {
      scene.exits = {
        ...(scene.exits ?? {}),
        ...patch.exits,
      };
    }

    if (Array.isArray(patch.shop_item_ids)) {
      scene.shop_item_ids = uniqueStrings([...(scene.shop_item_ids ?? []), ...patch.shop_item_ids]);
    }

    if (typeof patch.chapter === 'string' && patch.chapter) {
      scene.chapter = patch.chapter;
    }
    if (typeof patch.room_type === 'string' && patch.room_type) {
      scene.room_type = patch.room_type;
    }
    if (typeof patch.risk_level === 'string' && patch.risk_level) {
      scene.risk_level = patch.risk_level;
    }
    if (typeof patch.landmark === 'string' && patch.landmark) {
      scene.landmark = patch.landmark;
    }
    if (typeof patch.room_layer === 'string' && patch.room_layer) {
      scene.room_layer = patch.room_layer;
    }
    if (typeof patch.pvp_enabled === 'boolean') {
      scene.pvp_enabled = patch.pvp_enabled;
    }
    if (Array.isArray(patch.rumors) && patch.rumors.length > 0) {
      scene.rumors = uniqueStrings([...(scene.rumors ?? []), ...patch.rumors]);
    }
    if (Array.isArray(patch.loop_tags) && patch.loop_tags.length > 0) {
      scene.loop_tags = uniqueStrings([...(scene.loop_tags ?? []), ...patch.loop_tags]);
    }
  }
}

function applySceneServices(scenes, services) {
  if (!Array.isArray(services) || services.length === 0) {
    return;
  }

  const sceneMap = buildIdMap(scenes, 'scene_id', 'scenes')
  for (const patch of services) {
    const sceneId = String(patch?.scene_id ?? '')
    if (!sceneId) {
      throw new Error('scene_services contains patch without scene_id')
    }

    const scene = sceneMap.get(sceneId)
    if (!scene) {
      throw new Error(`scene_services references missing scene: ${sceneId}`)
    }

    if (Array.isArray(patch.service_tags)) {
      scene.service_tags = uniqueStrings([...(scene.service_tags ?? []), ...patch.service_tags])
    }
    if (Array.isArray(patch.rumor_topics)) {
      scene.rumor_topics = uniqueStrings([...(scene.rumor_topics ?? []), ...patch.rumor_topics])
    }
    if (typeof patch.board_available === 'boolean') {
      scene.board_available = patch.board_available
    }
    if (Array.isArray(patch.mentor_ids)) {
      scene.mentor_ids = uniqueStrings([...(scene.mentor_ids ?? []), ...patch.mentor_ids])
    }
  }
}

function codexCategoryForItem(item) {
  const tags = new Set(item.tags ?? []);
  if (tags.has('herb') || tags.has('medicine')) {
    return '灵草丹药志';
  }
  if (tags.has('array') || tags.has('talisman') || item.item_type === 'treasure') {
    return '宝物阵法志';
  }
  if (item.item_type === 'manual') {
    return '功法技能志';
  }
  return '灵草丹药志';
}

function makeSummary(text, limit = 42) {
  if (!text) {
    return '';
  }
  return text.length <= limit ? text : `${text.slice(0, limit - 1)}…`;
}

function codexEntry(entry_id, category, title, summary, content, extras = {}) {
  return {
    entry_id,
    category,
    title,
    summary,
    content,
    related_scene_ids: extras.related_scene_ids ?? [],
    related_npc_ids: extras.related_npc_ids ?? [],
    related_monster_ids: extras.related_monster_ids ?? [],
    related_item_ids: extras.related_item_ids ?? [],
    related_sect_ids: extras.related_sect_ids ?? [],
    unlock_rules: extras.unlock_rules ?? [],
  };
}

function buildCodex({
  scenes,
  npcs,
  sects,
  monsters,
  items,
  skills,
  spells,
  recipes,
  treasures,
  formations,
  manualCodexEntries,
}) {
  const entries = [];

  for (const scene of scenes) {
    scene.codex_entry_id = `geo_${scene.scene_id}`;
    entries.push(
      codexEntry(
        scene.codex_entry_id,
        '地理志',
        scene.name,
        makeSummary(scene.description),
        `${scene.name}位于${scene.region_name}。${scene.description}`,
        {
          related_scene_ids: [scene.scene_id],
          unlock_rules: [{ trigger: 'enter_scene', target_id: scene.scene_id }],
        },
      ),
    );
  }

  for (const sect of sects) {
    sect.codex_entry_id = `sect_${sect.sect_id}`;
    const unlockRules = sect.join_npc_id
      ? [{ trigger: 'talk_npc', target_id: sect.join_npc_id }]
      : sect.join_scene_id
        ? [{ trigger: 'enter_scene', target_id: sect.join_scene_id }]
        : [];
    entries.push(
      codexEntry(
        sect.codex_entry_id,
        '宗门志',
        sect.name,
        makeSummary(sect.description),
        sect.description,
        {
          related_sect_ids: [sect.sect_id],
          related_scene_ids: sect.join_scene_id ? [sect.join_scene_id] : [],
          related_npc_ids: sect.join_npc_id ? [sect.join_npc_id] : [],
          unlock_rules: unlockRules,
        },
      ),
    );
  }

  for (const npc of npcs) {
    npc.codex_entry_id = `npc_${npc.npc_id}`;
    entries.push(
      codexEntry(
        npc.codex_entry_id,
        '人物志',
        npc.name,
        makeSummary(npc.description || npc.hint),
        `${npc.description || npc.hint}\n\n${npc.dialogue}`,
        {
          related_scene_ids: [npc.scene_id],
          related_npc_ids: [npc.npc_id],
          related_sect_ids: npc.sect_offer_id ? [npc.sect_offer_id] : [],
          unlock_rules: [{ trigger: 'talk_npc', target_id: npc.npc_id }],
        },
      ),
    );
  }

  for (const monster of monsters) {
    monster.codex_entry_id = `monster_${monster.monster_id}`;
    const category = monster.kind === '奇虫' ? '奇虫志' : '妖兽志';
    entries.push(
      codexEntry(
        monster.codex_entry_id,
        category,
        monster.name,
        makeSummary(monster.description),
        monster.description,
        {
          related_scene_ids: [monster.scene_id],
          related_monster_ids: [monster.monster_id],
          related_item_ids: monster.drop_item_id ? [monster.drop_item_id] : [],
          unlock_rules: [{ trigger: 'defeat_monster', target_id: monster.monster_id }],
        },
      ),
    );
  }

  for (const item of items) {
    item.codex_entry_id = `item_${item.item_id}`;
    entries.push(
      codexEntry(
        item.codex_entry_id,
        codexCategoryForItem(item),
        item.name,
        makeSummary(item.description),
        item.description,
        {
          related_item_ids: [item.item_id],
          unlock_rules: [{ trigger: 'obtain_item', target_id: item.item_id }],
        },
      ),
    );
  }

  for (const skill of skills) {
    skill.codex_entry_id = `skill_${skill.skill_id}`;
    entries.push(
      codexEntry(
        skill.codex_entry_id,
        '功法技能志',
        skill.name,
        makeSummary(skill.description),
        skill.description,
        {
          unlock_rules: [{ trigger: 'practice_skill', target_id: skill.skill_id }],
        },
      ),
    );
  }

  for (const spell of spells) {
    spell.codex_entry_id = `spell_${spell.spell_id}`;
    entries.push(
      codexEntry(
        spell.codex_entry_id,
        '法术志',
        spell.name,
        `${spell.element}系入门术法`,
        spell.description,
        {
          related_item_ids: spell.granted_by_item_id ? [spell.granted_by_item_id] : [],
          unlock_rules: [{ trigger: 'cast_spell', target_id: spell.spell_id }],
        },
      ),
    );
  }

  for (const recipe of recipes) {
    recipe.codex_entry_id = `recipe_${recipe.recipe_id}`;
    entries.push(
      codexEntry(
        recipe.codex_entry_id,
        '灵草丹药志',
        recipe.name,
        makeSummary(recipe.description),
        recipe.description,
        {
          related_scene_ids: recipe.station_scene_id ? [recipe.station_scene_id] : [],
          related_npc_ids: recipe.npc_id ? [recipe.npc_id] : [],
          related_item_ids: uniqueStrings([
            recipe.result_item_id,
            ...recipe.ingredient_items.map((ingredient) => ingredient.item_id),
          ]),
          unlock_rules: [{ trigger: 'brew_recipe', target_id: recipe.recipe_id }],
        },
      ),
    );
  }

  for (const treasure of treasures) {
    entries.push(
      codexEntry(
        `treasure_${treasure.treasure_id}`,
        '宝物阵法志',
        treasure.name,
        makeSummary(treasure.description),
        `${treasure.description}\n\n效果摘要：${treasure.effect_summary}`,
        {
          related_item_ids: [treasure.treasure_id],
          unlock_rules: [{ trigger: 'obtain_item', target_id: treasure.treasure_id }],
        },
      ),
    );
  }

  for (const formation of formations) {
    formation.codex_entry_id = `formation_${formation.formation_id}`;
    entries.push(
      codexEntry(
        formation.codex_entry_id,
        '宝物阵法志',
        formation.name,
        makeSummary(formation.description),
        `${formation.description}\n\n效果摘要：${formation.effect_summary}`,
        {
          related_scene_ids: formation.scene_id ? [formation.scene_id] : [],
          unlock_rules: formation.scene_id
            ? [{ trigger: 'enter_scene', target_id: formation.scene_id }]
            : [],
        },
      ),
    );
  }

  entries.push(...manualCodexEntries);

  return entries;
}

function buildSceneRelations(scenes, list, sourceIdKey, targetKey, label) {
  const grouped = new Map();
  for (const item of list) {
    const sceneId = String(item?.scene_id ?? '');
    const sourceId = String(item?.[sourceIdKey] ?? '');
    if (!sceneId) {
      throw new Error(`${label} contains item without scene_id`);
    }
    if (!sourceId) {
      throw new Error(`${label} contains item without ${sourceIdKey}`);
    }
    pushGrouped(grouped, sceneId, sourceId);
  }
  for (const scene of scenes) {
    scene[targetKey] = grouped.get(scene.scene_id) ?? [];
  }
}

function validateRefs({
  scenes,
  origins,
  backgrounds,
  items,
  sects,
  quests,
  npcs,
  monsters,
  resourceNodes,
  groundLoots,
  hazards,
  skills,
  spells,
  recipes,
  formations,
  manualCodexEntries,
  helpTopics,
  jobs,
  rumorSources,
  identityTracks,
}) {
  const sceneMap = buildIdMap(scenes, 'scene_id', 'scenes');
  const itemMap = buildIdMap(items, 'item_id', 'items');
  const sectMap = buildIdMap(sects, 'sect_id', 'sects');
  const npcMap = buildIdMap(npcs, 'npc_id', 'npcs');
  const monsterMap = buildIdMap(monsters, 'monster_id', 'monsters');
  const resourceNodeMap = buildIdMap(resourceNodes, 'node_id', 'resource_nodes');
  const groundLootMap = buildIdMap(groundLoots, 'loot_id', 'ground_loots');
  const hazardMap = buildIdMap(hazards, 'hazard_id', 'hazards');
  const skillMap = buildIdMap(skills, 'skill_id', 'skills');
  const spellMap = buildIdMap(spells, 'spell_id', 'spells');
  const recipeMap = buildIdMap(recipes, 'recipe_id', 'recipes');
  const originMap = buildIdMap(origins, 'origin_id', 'origins');
  const backgroundMap = buildIdMap(backgrounds, 'background_id', 'backgrounds');
  const formationMap = buildIdMap(formations, 'formation_id', 'formations');
  const questMap = buildIdMap(quests, 'quest_id', 'quests');
  const jobMap = buildIdMap(jobs, 'job_id', 'jobs');

  for (const scene of scenes) {
    for (const targetSceneId of Object.values(scene.exits ?? {})) {
      mustExist(sceneMap, targetSceneId, 'scene exit target');
    }
    for (const shopItemId of scene.shop_item_ids ?? []) {
      mustExist(itemMap, shopItemId, 'shop item');
    }
    for (const mentorId of scene.mentor_ids ?? []) {
      mustExist(npcMap, mentorId, 'scene mentor');
    }
  }

  for (const origin of origins) {
    mustExist(skillMap, origin.starter_skill_id, 'origin starter skill');
    for (const spellId of origin.starter_spell_ids ?? []) {
      mustExist(spellMap, spellId, 'origin starter spell');
    }
  }

  for (const background of backgrounds) {
    for (const starterItem of background.starter_inventory ?? []) {
      mustExist(itemMap, starterItem.item_id, 'background starter item');
    }
  }

  for (const sect of sects) {
    if (sect.join_scene_id) {
      mustExist(sceneMap, sect.join_scene_id, 'sect join scene');
    }
    if (sect.join_npc_id) {
      mustExist(npcMap, sect.join_npc_id, 'sect join npc');
    }
  }

  for (const npc of npcs) {
    mustExist(sceneMap, npc.scene_id, 'npc scene');
    if (npc.sect_offer_id) {
      mustExist(sectMap, npc.sect_offer_id, 'npc sect offer');
    }
    for (const questId of npc.quest_ids ?? []) {
      mustExist(questMap, questId, 'npc quest');
    }
  }

  for (const monster of monsters) {
    mustExist(sceneMap, monster.scene_id, 'monster scene');
    mustExist(itemMap, monster.drop_item_id, 'monster drop item');
  }

  for (const resourceNode of resourceNodes) {
    mustExist(sceneMap, resourceNode.scene_id, 'resource node scene');
    mustExist(itemMap, resourceNode.drop_item_id, 'resource node drop item');
    if (resourceNode.required_skill_id) {
      mustExist(skillMap, resourceNode.required_skill_id, 'resource node required skill');
    }
  }

  for (const loot of groundLoots) {
    mustExist(sceneMap, loot.scene_id, 'ground loot scene');
    mustExist(itemMap, loot.item_id, 'ground loot item');
  }

  for (const hazard of hazards) {
    mustExist(sceneMap, hazard.scene_id, 'hazard scene');
  }

  for (const quest of quests) {
    mustExist(npcMap, quest.issuer_npc_id, 'quest issuer');
    mustExist(npcMap, quest.submit_npc_id, 'quest submit npc');
    mustExist(itemMap, quest.required_item_id, 'quest required item');
    if (quest.reward_item_id) {
      mustExist(itemMap, quest.reward_item_id, 'quest reward item');
    }
    if (quest.reward_sect_id) {
      mustExist(sectMap, quest.reward_sect_id, 'quest reward sect');
    }
  }

  for (const spell of spells) {
    if (spell.granted_by_item_id) {
      mustExist(itemMap, spell.granted_by_item_id, 'spell grant item');
    }
  }

  for (const recipe of recipes) {
    mustExist(itemMap, recipe.result_item_id, 'recipe result item');
    if (recipe.station_scene_id) {
      mustExist(sceneMap, recipe.station_scene_id, 'recipe station scene');
    }
    if (recipe.npc_id) {
      mustExist(npcMap, recipe.npc_id, 'recipe npc');
    }
    if (recipe.required_skill_id) {
      mustExist(skillMap, recipe.required_skill_id, 'recipe required skill');
    }
    for (const ingredient of recipe.ingredient_items ?? []) {
      mustExist(itemMap, ingredient.item_id, 'recipe ingredient item');
    }
  }

  for (const formation of formations) {
    if (formation.scene_id) {
      mustExist(sceneMap, formation.scene_id, 'formation scene');
    }
  }

  for (const entry of manualCodexEntries) {
    for (const sceneId of entry.related_scene_ids ?? []) {
      mustExist(sceneMap, sceneId, 'codex related scene');
    }
    for (const npcId of entry.related_npc_ids ?? []) {
      mustExist(npcMap, npcId, 'codex related npc');
    }
    for (const monsterId of entry.related_monster_ids ?? []) {
      mustExist(monsterMap, monsterId, 'codex related monster');
    }
    for (const itemId of entry.related_item_ids ?? []) {
      mustExist(itemMap, itemId, 'codex related item');
    }
    for (const sectId of entry.related_sect_ids ?? []) {
      mustExist(sectMap, sectId, 'codex related sect');
    }
  }

  for (const job of jobs) {
    mustExist(sceneMap, job.scene_id, 'job scene');
    if (job.issuer_npc_id) {
      mustExist(npcMap, job.issuer_npc_id, 'job issuer');
    }
    if (job.submit_npc_id) {
      mustExist(npcMap, job.submit_npc_id, 'job submit npc');
    }
    if (job.related_quest_id) {
      mustExist(questMap, job.related_quest_id, 'job related quest');
    }
  }

  for (const rumorSource of rumorSources) {
    mustExist(sceneMap, rumorSource.scene_id, 'rumor scene');
    if (rumorSource.npc_id) {
      mustExist(npcMap, rumorSource.npc_id, 'rumor npc');
    }
    for (const jobId of rumorSource.job_ids ?? []) {
      mustExist(jobMap, jobId, 'rumor job');
    }
    for (const questId of rumorSource.quest_ids ?? []) {
      mustExist(questMap, questId, 'rumor quest');
    }
  }

  for (const track of identityTracks) {
    for (const mentorId of track.mentor_ids ?? []) {
      mustExist(npcMap, mentorId, 'identity mentor');
    }
  }

  for (const topic of helpTopics) {
    for (const relatedCommand of topic.related_commands ?? []) {
      if (!String(relatedCommand ?? '').trim()) {
        throw new Error(`help topic ${topic.topic_id} contains empty related command`);
      }
    }
  }

  return {
    sceneMap,
    itemMap,
    sectMap,
    npcMap,
    monsterMap,
    resourceNodeMap,
    groundLootMap,
    hazardMap,
    skillMap,
    spellMap,
    recipeMap,
    originMap,
    backgroundMap,
    formationMap,
    questMap,
    jobMap,
  };
}

function attachSceneCodex(scenes, codexEntries) {
  for (const scene of scenes) {
    scene.codex_entry_ids = uniqueStrings([
      scene.codex_entry_id,
      ...codexEntries
        .filter((entry) => (entry.related_scene_ids ?? []).includes(scene.scene_id))
        .map((entry) => entry.entry_id),
    ]);
  }
}

function buildMapPayload(scenes) {
  const nodes = scenes
    .filter((scene) => Number.isFinite(scene.map_x) && Number.isFinite(scene.map_y))
    .map((scene) => ({
      id: scene.scene_id,
      name: scene.name,
      region: scene.region_name,
      x: scene.map_x,
      y: scene.map_y,
    }));

  const edgeSet = new Set();
  const edges = [];
  for (const scene of scenes) {
    for (const targetId of Object.values(scene.exits ?? {})) {
      const key = [scene.scene_id, targetId].sort().join(':');
      if (edgeSet.has(key)) {
        continue;
      }
      edgeSet.add(key);
      edges.push({ from: scene.scene_id, to: targetId });
    }
  }
  return { nodes, edges };
}

function validateP0Coverage({ scenes, helpTopics, jobs, rumorSources, identityTracks }) {
  if ((helpTopics ?? []).length < 8) {
    throw new Error(`P0 validation failed: expected at least 8 help topics, got ${helpTopics.length}`);
  }

  const requiredTrackIds = ['loose_cultivator', 'qixuan_gate', 'huangfeng_valley', 'spirit_beast_mountain'];
  const presentTrackIds = new Set((identityTracks ?? []).map((track) => String(track.track_id ?? '').trim()));
  for (const trackId of requiredTrackIds) {
    if (!presentTrackIds.has(trackId)) {
      throw new Error(`P0 validation failed: missing identity track ${trackId}`);
    }
  }

  const workSceneIds = new Set(
    (jobs ?? [])
      .filter((job) => String(job.scene_id ?? '').trim())
      .map((job) => String(job.scene_id ?? '').trim()),
  );
  if (workSceneIds.size < 7) {
    throw new Error(`P0 validation failed: expected jobs in at least 7 scenes, got ${workSceneIds.size}`);
  }

  const rumorSceneIds = new Set((rumorSources ?? []).map((source) => String(source.scene_id ?? '').trim()).filter(Boolean));
  const requiredRumorScenes = [
    ['嘉元城', 'jiayuan_market'],
    ['太南谷', 'tainan_fair'],
    ['天南港', 'tiannan_harbor'],
  ];
  for (const [label, sceneId] of requiredRumorScenes) {
    if (!rumorSceneIds.has(sceneId)) {
      throw new Error(`P0 validation failed: missing rumor coverage for ${label} (${sceneId})`);
    }
  }
}

function validateNascentSoulSkeleton({ defaults, items, helpTopics }) {
  const realmNames = Array.isArray(defaults?.realm_names) ? defaults.realm_names : [];
  if (!realmNames.includes('元婴初期')) {
    throw new Error('Nascent soul validation failed: defaults.realm_names is missing 元婴初期');
  }

  const itemIds = new Set((items ?? []).map((item) => String(item.item_id ?? '').trim()).filter(Boolean));
  for (const itemId of ['gold_core_pill', 'nascent_soul_pill']) {
    if (!itemIds.has(itemId)) {
      throw new Error(`Nascent soul validation failed: missing item ${itemId}`);
    }
  }

  const helpTopicIds = new Set((helpTopics ?? []).map((topic) => String(topic.topic_id ?? '').trim()).filter(Boolean));
  for (const topicId of ['core_dan', 'nascent_soul']) {
    if (!helpTopicIds.has(topicId)) {
      throw new Error(`Nascent soul validation failed: missing help topic ${topicId}`);
    }
  }
}

function validateNascentSoulHelpCoverage({ helpTopics }) {
  const topicMap = buildIdMap(helpTopics, 'topic_id', 'nascent_soul_help_topics');
  const coreDanBody = ensureArray(topicMap.get('core_dan')?.body_lines ?? [], 'core_dan.body_lines').join('\n');
  const nascentSoulBody = ensureArray(topicMap.get('nascent_soul')?.body_lines ?? [], 'nascent_soul.body_lines').join('\n');

  for (const keyword of ['外海见闻', '结丹之门', '结丹灵丸', '青焰晶髓', '紫丹灵砂']) {
    if (!coreDanBody.includes(keyword)) {
      throw new Error(`Nascent soul validation failed: core_dan help is missing ${keyword}`);
    }
  }

  for (const keyword of ['凝婴前夜', '凝婴灵丹', '星海心珀', '养魂古玉', '世界见闻']) {
    if (!nascentSoulBody.includes(keyword)) {
      throw new Error(`Nascent soul validation failed: nascent_soul help is missing ${keyword}`);
    }
  }
}

function validateNascentSoulMainline({ scenes, quests }) {
  const sceneMap = buildIdMap(scenes, 'scene_id', 'nascent_soul_scenes');
  const questMap = buildIdMap(quests, 'quest_id', 'nascent_soul_quests');

  for (const sceneId of ['outer_sea_mid', 'core_flame_vein', 'ancient_ruin_ring', 'star_abyss']) {
    if (!sceneMap.has(sceneId)) {
      throw new Error(`Nascent soul validation failed: missing scene ${sceneId}`);
    }
  }

  for (const questId of ['outer_sea_trail', 'gold_core_gate', 'core_ruin_heart', 'nascent_soul_gate']) {
    if (!questMap.has(questId)) {
      throw new Error(`Nascent soul validation failed: missing quest ${questId}`);
    }
  }
}

function renderWorldMapTs(mapPayload) {
  return `export interface WorldMapNode {
  id: string
  name: string
  region: string
  x: number
  y: number
}

export interface WorldMapEdge {
  from: string
  to: string
}

export const directionLabelMap: Record<string, string> = {
  north: '北',
  south: '南',
  east: '东',
  west: '西',
  up: '上',
  down: '下',
}

export const worldMapNodes: WorldMapNode[] = ${JSON.stringify(mapPayload.nodes, null, 2)}

export const worldMapEdges: WorldMapEdge[] = ${JSON.stringify(mapPayload.edges, null, 2)}
`;
}

async function main() {
  const scenes = mergeArrays(
    structuredClone(worldMainline.scenes),
    structuredClone(pureMudExpansion.scenes ?? []),
  );
  const defaults = structuredClone(worldMainline.defaults);
  const origins = structuredClone(worldMainline.origins);
  const backgrounds = structuredClone(pureMudExpansion.backgrounds ?? []);
  const quests = mergeArrays(
    structuredClone(worldMainline.quests),
    structuredClone(pureMudExpansion.quests ?? []),
  );
  const manualCodexEntries = mergeArrays(
    structuredClone(worldMainline.manual_codex_entries ?? []),
    structuredClone(pureMudExpansion.manual_codex_entries ?? []),
  );
  const sects = mergeArrays(
    structuredClone(charactersSects.sects),
    structuredClone(pureMudExpansion.sects ?? []),
  );
  const npcs = mergeArrays(
    structuredClone(charactersSects.npcs),
    structuredClone(pureMudExpansion.npcs ?? []),
  );
  const monsters = mergeArrays(
    structuredClone(creaturesResources.monsters),
    structuredClone(pureMudExpansion.monsters ?? []),
  );
  const resourceNodes = mergeArrays(
    structuredClone(creaturesResources.resource_nodes),
    structuredClone(pureMudExpansion.resource_nodes ?? []),
  );
  const groundLoots = mergeArrays(
    structuredClone(creaturesResources.ground_loots),
    structuredClone(pureMudExpansion.ground_loots ?? []),
  );
  const hazards = mergeArrays(
    structuredClone(creaturesResources.hazards),
    structuredClone(pureMudExpansion.hazards ?? []),
  );
  const items = mergeArrays(
    structuredClone(itemsSystems.items),
    structuredClone(pureMudExpansion.items ?? []),
  );
  const skills = mergeArrays(
    structuredClone(itemsSystems.skills),
    structuredClone(pureMudExpansion.skills ?? []),
  );
  const spells = mergeArrays(
    structuredClone(itemsSystems.spells),
    structuredClone(pureMudExpansion.spells ?? []),
  );
  const recipes = mergeArrays(
    structuredClone(itemsSystems.recipes),
    structuredClone(pureMudExpansion.recipes ?? []),
  );
  const treasures = mergeArrays(
    structuredClone(itemsSystems.treasures),
    structuredClone(pureMudExpansion.treasures ?? []),
  );
  const formations = mergeArrays(
    structuredClone(itemsSystems.formations),
    structuredClone(pureMudExpansion.formations ?? []),
  );
  const attributeDefaults = structuredClone(itemsSystems.attribute_defaults);
  const helpTopics = structuredClone(mudHelpManual.help_topics ?? []);
  const jobs = structuredClone(mudJobsRumors.jobs ?? []);
  const rumorSources = structuredClone(mudJobsRumors.rumor_sources ?? []);
  const identityTracks = structuredClone(mudTitlesFactions.identity_tracks ?? []);

  applyScenePatches(scenes, structuredClone(pureMudExpansion.scene_patches ?? []));
  applySceneServices(scenes, structuredClone(mudTitlesFactions.scene_services ?? []));
  normalizeSceneMetadata(scenes);

  validateRefs({
    scenes,
    origins,
    backgrounds,
    items,
    sects,
    quests,
    npcs,
    monsters,
    resourceNodes,
    groundLoots,
    hazards,
    skills,
    spells,
    recipes,
    formations,
    manualCodexEntries,
    helpTopics,
    jobs,
    rumorSources,
    identityTracks,
  });
  validateP0Coverage({
    scenes,
    helpTopics,
    jobs,
    rumorSources,
    identityTracks,
  });
  validateNascentSoulSkeleton({
    defaults,
    items,
    helpTopics,
  });
  validateNascentSoulHelpCoverage({
    helpTopics,
  });
  validateNascentSoulMainline({
    scenes,
    quests,
  });

  buildSceneRelations(scenes, npcs, 'npc_id', 'npc_ids', 'npcs');
  buildSceneRelations(scenes, monsters, 'monster_id', 'monster_ids', 'monsters');
  buildSceneRelations(scenes, resourceNodes, 'node_id', 'resource_node_ids', 'resource_nodes');
  buildSceneRelations(scenes, groundLoots, 'loot_id', 'ground_loot_ids', 'ground_loots');
  buildSceneRelations(scenes, hazards, 'hazard_id', 'hazard_ids', 'hazards');

  const codexEntries = buildCodex({
    scenes,
    npcs,
    sects,
    monsters,
    items,
    skills,
    spells,
    recipes,
    treasures,
    formations,
    manualCodexEntries,
  });

  attachSceneCodex(scenes, codexEntries);

  const output = {
    generated_at: new Date().toISOString(),
    defaults,
    attribute_defaults: attributeDefaults,
    origins,
    backgrounds,
    items,
    sects,
    quests,
    npcs,
    monsters,
    scenes,
    skills,
    spells,
    recipes,
    treasures,
    formations,
    help_topics: helpTopics,
    jobs,
    identity_tracks: identityTracks,
    rumor_sources: rumorSources,
    resource_nodes: resourceNodes,
    ground_loots: groundLoots,
    hazards,
    codex_entries: codexEntries,
  };

  const worldDataPath = path.join(repoRoot, 'doc', 'mud', 'world_data.json');
  await fs.mkdir(path.dirname(worldDataPath), { recursive: true });
  await fs.writeFile(worldDataPath, `${JSON.stringify(output, null, 2)}\n`, 'utf8');

  const mapPayload = buildMapPayload(scenes);
  const worldMapTsPath = path.join(repoRoot, 'client', 'src', 'lib', 'world-map.ts');
  await fs.writeFile(worldMapTsPath, renderWorldMapTs(mapPayload), 'utf8');

  console.log(
    JSON.stringify(
      {
        scenes: scenes.length,
        npcs: npcs.length,
        monsters: monsters.length,
        quests: quests.length,
        items: items.length,
        codex_entries: codexEntries.length,
      },
      null,
      2,
    ),
  );
}

main().catch((error) => {
  console.error(error instanceof Error ? error.message : String(error));
  process.exitCode = 1;
});
