export interface WorldMapNode {
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

export const worldMapNodes: WorldMapNode[] = [
  {
    "id": "qixuan_hall",
    "name": "七玄门议事堂",
    "region": "七玄门",
    "x": 14,
    "y": 9
  },
  {
    "id": "qixuan_square",
    "name": "七玄门外场",
    "region": "七玄门",
    "x": 14,
    "y": 18
  },
  {
    "id": "qixuan_dormitory",
    "name": "外门弟子舍",
    "region": "七玄门",
    "x": 6,
    "y": 18
  },
  {
    "id": "qixuan_backslope",
    "name": "后山缓坡",
    "region": "七玄门",
    "x": 14,
    "y": 28
  },
  {
    "id": "qixuan_stream",
    "name": "洗剑溪",
    "region": "七玄门",
    "x": 6,
    "y": 28
  },
  {
    "id": "qixuan_medicine_garden",
    "name": "药圃小院",
    "region": "七玄门",
    "x": 22,
    "y": 28
  },
  {
    "id": "jiayuan_east_gate",
    "name": "嘉元城东门",
    "region": "嘉元城",
    "x": 28,
    "y": 18
  },
  {
    "id": "jiayuan_market",
    "name": "嘉元城集市",
    "region": "嘉元城",
    "x": 36,
    "y": 18
  },
  {
    "id": "mofu_gate",
    "name": "墨府大门",
    "region": "墨府",
    "x": 44,
    "y": 18
  },
  {
    "id": "mofu_front_hall",
    "name": "墨府前厅",
    "region": "墨府",
    "x": 52,
    "y": 18
  },
  {
    "id": "mofu_courtyard",
    "name": "墨府内院",
    "region": "墨府",
    "x": 52,
    "y": 28
  },
  {
    "id": "bamboo_forest",
    "name": "青竹林",
    "region": "嘉元城郊",
    "x": 60,
    "y": 24
  },
  {
    "id": "herb_slope",
    "name": "黄精坡",
    "region": "嘉元城郊",
    "x": 68,
    "y": 24
  },
  {
    "id": "backhill_cave",
    "name": "后山洞窟",
    "region": "后山",
    "x": 76,
    "y": 24
  },
  {
    "id": "tainan_gate",
    "name": "太南谷入口",
    "region": "太南谷",
    "x": 36,
    "y": 36
  },
  {
    "id": "tainan_fair",
    "name": "太南小会",
    "region": "太南谷",
    "x": 44,
    "y": 36
  },
  {
    "id": "xin_house",
    "name": "辛如音小筑",
    "region": "太南谷",
    "x": 52,
    "y": 30
  },
  {
    "id": "talisman_street",
    "name": "符箓长街",
    "region": "太南谷",
    "x": 52,
    "y": 36
  },
  {
    "id": "array_lane",
    "name": "阵旗巷",
    "region": "太南谷",
    "x": 52,
    "y": 44
  },
  {
    "id": "mountain_path",
    "name": "山隙古道",
    "region": "太南谷",
    "x": 68,
    "y": 36
  },
  {
    "id": "huangfeng_outpost",
    "name": "黄枫谷外营",
    "region": "黄枫谷",
    "x": 24,
    "y": 48
  },
  {
    "id": "huangfeng_hall",
    "name": "黄枫谷偏殿",
    "region": "黄枫谷",
    "x": 24,
    "y": 58
  },
  {
    "id": "huangfeng_medicine_terrace",
    "name": "药梯台",
    "region": "黄枫谷",
    "x": 34,
    "y": 48
  },
  {
    "id": "huangfeng_scripture",
    "name": "藏经石廊",
    "region": "黄枫谷",
    "x": 34,
    "y": 58
  },
  {
    "id": "huangfeng_foothill",
    "name": "枫岭山麓",
    "region": "黄枫谷外山",
    "x": 14,
    "y": 48
  },
  {
    "id": "huangfeng_cloud_bridge",
    "name": "云桥峰口",
    "region": "黄枫谷",
    "x": 44,
    "y": 58
  },
  {
    "id": "blood_gate",
    "name": "血禁石门",
    "region": "血色禁地",
    "x": 24,
    "y": 72
  },
  {
    "id": "blood_forbidden_outer",
    "name": "血色禁地外围",
    "region": "血色禁地",
    "x": 34,
    "y": 72
  },
  {
    "id": "blood_swamp",
    "name": "血雾沼泽",
    "region": "血色禁地",
    "x": 44,
    "y": 72
  },
  {
    "id": "blood_orchid_vale",
    "name": "血兰谷",
    "region": "血色禁地",
    "x": 34,
    "y": 82
  },
  {
    "id": "spirit_beast_altar",
    "name": "灵兽祭坛",
    "region": "血色禁地",
    "x": 50,
    "y": 76
  },
  {
    "id": "blood_ruins",
    "name": "血禁残垣",
    "region": "血色禁地",
    "x": 50,
    "y": 86
  },
  {
    "id": "tiannan_harbor",
    "name": "天南港",
    "region": "天南海岸",
    "x": 14,
    "y": 84
  },
  {
    "id": "tiannan_market",
    "name": "海商坊市",
    "region": "天南海岸",
    "x": 26,
    "y": 84
  },
  {
    "id": "tiannan_dock",
    "name": "远航码头",
    "region": "天南海岸",
    "x": 38,
    "y": 84
  },
  {
    "id": "sea_wind_tower",
    "name": "望海风塔",
    "region": "天南海岸",
    "x": 48,
    "y": 80
  },
  {
    "id": "smuggler_alley",
    "name": "暗潮小巷",
    "region": "天南海岸",
    "x": 48,
    "y": 90
  },
  {
    "id": "chaos_sea_port",
    "name": "乱星海近港",
    "region": "乱星海",
    "x": 16,
    "y": 98
  },
  {
    "id": "chaos_sea_ship",
    "name": "灵帆海船",
    "region": "乱星海",
    "x": 28,
    "y": 98
  },
  {
    "id": "chaos_sea_isle",
    "name": "残碑孤岛",
    "region": "乱星海",
    "x": 40,
    "y": 98
  },
  {
    "id": "reef_shore",
    "name": "礁影浅滩",
    "region": "乱星海",
    "x": 52,
    "y": 98
  },
  {
    "id": "demon_fish_nest",
    "name": "妖鱼巢湾",
    "region": "乱星海",
    "x": 40,
    "y": 108
  },
  {
    "id": "storm_route",
    "name": "风暴航道",
    "region": "乱星海",
    "x": 56,
    "y": 108
  },
  {
    "id": "xutian_hall",
    "name": "虚天殿外殿",
    "region": "虚天殿",
    "x": 68,
    "y": 108
  },
  {
    "id": "xutian_corridor",
    "name": "虚天回廊",
    "region": "虚天殿",
    "x": 78,
    "y": 108
  },
  {
    "id": "xutian_pill_room",
    "name": "丹房遗室",
    "region": "虚天殿",
    "x": 88,
    "y": 108
  },
  {
    "id": "xutian_treasure_chamber",
    "name": "秘藏偏库",
    "region": "虚天殿",
    "x": 88,
    "y": 118
  },
  {
    "id": "xutian_star_platform",
    "name": "星纹祭台",
    "region": "虚天殿",
    "x": 68,
    "y": 118
  },
  {
    "id": "xutian_inner_gate",
    "name": "内殿玄门",
    "region": "虚天殿",
    "x": 78,
    "y": 118
  }
]

export const worldMapEdges: WorldMapEdge[] = [
  {
    "from": "qixuan_hall",
    "to": "qixuan_square"
  },
  {
    "from": "qixuan_square",
    "to": "jiayuan_east_gate"
  },
  {
    "from": "qixuan_square",
    "to": "qixuan_dormitory"
  },
  {
    "from": "qixuan_square",
    "to": "qixuan_backslope"
  },
  {
    "from": "qixuan_dormitory",
    "to": "qixuan_stream"
  },
  {
    "from": "qixuan_backslope",
    "to": "qixuan_medicine_garden"
  },
  {
    "from": "qixuan_backslope",
    "to": "jiayuan_east_gate"
  },
  {
    "from": "qixuan_stream",
    "to": "qixuan_backslope"
  },
  {
    "from": "qixuan_medicine_garden",
    "to": "jiayuan_market"
  },
  {
    "from": "jiayuan_east_gate",
    "to": "jiayuan_market"
  },
  {
    "from": "jiayuan_east_gate",
    "to": "mofu_gate"
  },
  {
    "from": "jiayuan_market",
    "to": "mofu_gate"
  },
  {
    "from": "jiayuan_market",
    "to": "tainan_gate"
  },
  {
    "from": "mofu_gate",
    "to": "mofu_front_hall"
  },
  {
    "from": "mofu_gate",
    "to": "mofu_courtyard"
  },
  {
    "from": "mofu_front_hall",
    "to": "mofu_courtyard"
  },
  {
    "from": "mofu_courtyard",
    "to": "bamboo_forest"
  },
  {
    "from": "bamboo_forest",
    "to": "herb_slope"
  },
  {
    "from": "bamboo_forest",
    "to": "tainan_gate"
  },
  {
    "from": "herb_slope",
    "to": "backhill_cave"
  },
  {
    "from": "backhill_cave",
    "to": "mountain_path"
  },
  {
    "from": "tainan_gate",
    "to": "xin_house"
  },
  {
    "from": "tainan_gate",
    "to": "huangfeng_outpost"
  },
  {
    "from": "tainan_fair",
    "to": "tainan_gate"
  },
  {
    "from": "tainan_fair",
    "to": "talisman_street"
  },
  {
    "from": "tainan_fair",
    "to": "array_lane"
  },
  {
    "from": "xin_house",
    "to": "talisman_street"
  },
  {
    "from": "talisman_street",
    "to": "array_lane"
  },
  {
    "from": "array_lane",
    "to": "mountain_path"
  },
  {
    "from": "array_lane",
    "to": "huangfeng_cloud_bridge"
  },
  {
    "from": "mountain_path",
    "to": "huangfeng_outpost"
  },
  {
    "from": "huangfeng_outpost",
    "to": "huangfeng_hall"
  },
  {
    "from": "huangfeng_outpost",
    "to": "huangfeng_foothill"
  },
  {
    "from": "huangfeng_outpost",
    "to": "huangfeng_medicine_terrace"
  },
  {
    "from": "huangfeng_hall",
    "to": "huangfeng_scripture"
  },
  {
    "from": "huangfeng_hall",
    "to": "blood_gate"
  },
  {
    "from": "huangfeng_medicine_terrace",
    "to": "huangfeng_scripture"
  },
  {
    "from": "huangfeng_scripture",
    "to": "huangfeng_cloud_bridge"
  },
  {
    "from": "huangfeng_foothill",
    "to": "tiannan_harbor"
  },
  {
    "from": "huangfeng_cloud_bridge",
    "to": "blood_swamp"
  },
  {
    "from": "blood_gate",
    "to": "blood_forbidden_outer"
  },
  {
    "from": "blood_gate",
    "to": "tiannan_harbor"
  },
  {
    "from": "blood_forbidden_outer",
    "to": "blood_swamp"
  },
  {
    "from": "blood_forbidden_outer",
    "to": "blood_orchid_vale"
  },
  {
    "from": "blood_swamp",
    "to": "spirit_beast_altar"
  },
  {
    "from": "blood_orchid_vale",
    "to": "spirit_beast_altar"
  },
  {
    "from": "spirit_beast_altar",
    "to": "blood_ruins"
  },
  {
    "from": "blood_ruins",
    "to": "tiannan_market"
  },
  {
    "from": "tiannan_harbor",
    "to": "tiannan_market"
  },
  {
    "from": "tiannan_harbor",
    "to": "chaos_sea_port"
  },
  {
    "from": "tiannan_market",
    "to": "tiannan_dock"
  },
  {
    "from": "tiannan_market",
    "to": "chaos_sea_ship"
  },
  {
    "from": "tiannan_dock",
    "to": "sea_wind_tower"
  },
  {
    "from": "tiannan_dock",
    "to": "chaos_sea_ship"
  },
  {
    "from": "sea_wind_tower",
    "to": "smuggler_alley"
  },
  {
    "from": "smuggler_alley",
    "to": "tiannan_dock"
  },
  {
    "from": "smuggler_alley",
    "to": "chaos_sea_port"
  },
  {
    "from": "chaos_sea_port",
    "to": "chaos_sea_ship"
  },
  {
    "from": "chaos_sea_port",
    "to": "reef_shore"
  },
  {
    "from": "chaos_sea_ship",
    "to": "chaos_sea_isle"
  },
  {
    "from": "chaos_sea_isle",
    "to": "reef_shore"
  },
  {
    "from": "chaos_sea_isle",
    "to": "demon_fish_nest"
  },
  {
    "from": "reef_shore",
    "to": "storm_route"
  },
  {
    "from": "demon_fish_nest",
    "to": "storm_route"
  },
  {
    "from": "storm_route",
    "to": "xutian_hall"
  },
  {
    "from": "xutian_hall",
    "to": "xutian_corridor"
  },
  {
    "from": "xutian_hall",
    "to": "xutian_star_platform"
  },
  {
    "from": "xutian_corridor",
    "to": "xutian_pill_room"
  },
  {
    "from": "xutian_corridor",
    "to": "xutian_inner_gate"
  },
  {
    "from": "xutian_pill_room",
    "to": "xutian_treasure_chamber"
  },
  {
    "from": "xutian_treasure_chamber",
    "to": "xutian_inner_gate"
  },
  {
    "from": "xutian_star_platform",
    "to": "xutian_inner_gate"
  }
]
