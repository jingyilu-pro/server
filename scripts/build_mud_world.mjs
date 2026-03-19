import fs from 'node:fs/promises';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

import worldMainline from '../doc/mud/source/world_mainline.mjs';
import charactersSects from '../doc/mud/source/characters_sects.mjs';
import creaturesResources from '../doc/mud/source/creatures_resources.mjs';
import itemsSystems from '../doc/mud/source/items_systems.mjs';

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
  const formationMap = buildIdMap(formations, 'formation_id', 'formations');

  for (const scene of scenes) {
    for (const targetSceneId of Object.values(scene.exits ?? {})) {
      mustExist(sceneMap, targetSceneId, 'scene exit target');
    }
    for (const shopItemId of scene.shop_item_ids ?? []) {
      mustExist(itemMap, shopItemId, 'shop item');
    }
  }

  for (const origin of origins) {
    mustExist(skillMap, origin.starter_skill_id, 'origin starter skill');
    for (const spellId of origin.starter_spell_ids ?? []) {
      mustExist(spellMap, spellId, 'origin starter spell');
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
      mustExist(buildIdMap(quests, 'quest_id', 'quests'), questId, 'npc quest');
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
    formationMap,
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
  const scenes = structuredClone(worldMainline.scenes);
  const defaults = structuredClone(worldMainline.defaults);
  const origins = structuredClone(worldMainline.origins);
  const quests = structuredClone(worldMainline.quests);
  const manualCodexEntries = structuredClone(worldMainline.manual_codex_entries ?? []);
  const sects = structuredClone(charactersSects.sects);
  const npcs = structuredClone(charactersSects.npcs);
  const monsters = structuredClone(creaturesResources.monsters);
  const resourceNodes = structuredClone(creaturesResources.resource_nodes);
  const groundLoots = structuredClone(creaturesResources.ground_loots);
  const hazards = structuredClone(creaturesResources.hazards);
  const items = structuredClone(itemsSystems.items);
  const skills = structuredClone(itemsSystems.skills);
  const spells = structuredClone(itemsSystems.spells);
  const recipes = structuredClone(itemsSystems.recipes);
  const treasures = structuredClone(itemsSystems.treasures);
  const formations = structuredClone(itemsSystems.formations);
  const attributeDefaults = structuredClone(itemsSystems.attribute_defaults);

  validateRefs({
    scenes,
    origins,
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
