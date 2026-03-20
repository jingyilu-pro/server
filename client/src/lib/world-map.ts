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
  },
  {
    "id": "wanderer_trail",
    "name": "散修古道",
    "region": "七玄外野",
    "x": -2,
    "y": 28
  },
  {
    "id": "wanderer_camp",
    "name": "落脚草棚",
    "region": "七玄外野",
    "x": -10,
    "y": 28
  },
  {
    "id": "wanderer_creek",
    "name": "乱石溪湾",
    "region": "七玄外野",
    "x": -18,
    "y": 28
  },
  {
    "id": "wanderer_brush",
    "name": "狐草灌丛",
    "region": "七玄外野",
    "x": -26,
    "y": 28
  },
  {
    "id": "wanderer_stone_slope",
    "name": "乱石坡",
    "region": "七玄外野",
    "x": -34,
    "y": 28
  },
  {
    "id": "wanderer_old_temple",
    "name": "残庙旧址",
    "region": "七玄外野",
    "x": -42,
    "y": 28
  },
  {
    "id": "wanderer_firefield",
    "name": "焦土地",
    "region": "七玄外野",
    "x": -50,
    "y": 28
  },
  {
    "id": "wanderer_fox_den",
    "name": "狐穴坡",
    "region": "七玄外野",
    "x": -58,
    "y": 28
  },
  {
    "id": "wanderer_watch_tower",
    "name": "断木望台",
    "region": "七玄外野",
    "x": -66,
    "y": 28
  },
  {
    "id": "wanderer_cold_pool",
    "name": "寒潭边",
    "region": "七玄外野",
    "x": -74,
    "y": 28
  },
  {
    "id": "escort_post",
    "name": "东门驿棚",
    "region": "越京官道",
    "x": 28,
    "y": 8
  },
  {
    "id": "escort_road",
    "name": "碎石官道",
    "region": "越京官道",
    "x": 28,
    "y": 0
  },
  {
    "id": "escort_wayside",
    "name": "路旁荒亭",
    "region": "越京官道",
    "x": 28,
    "y": -8
  },
  {
    "id": "bandit_gully",
    "name": "截路沟",
    "region": "越京官道",
    "x": 28,
    "y": -16
  },
  {
    "id": "broken_bridge",
    "name": "断桥坎",
    "region": "越京官道",
    "x": 28,
    "y": -24
  },
  {
    "id": "relay_station",
    "name": "换马驿",
    "region": "越京官道",
    "x": 28,
    "y": -32
  },
  {
    "id": "granary_yard",
    "name": "粮场外院",
    "region": "越京官道",
    "x": 28,
    "y": -40
  },
  {
    "id": "night_inn",
    "name": "夜宿破栈",
    "region": "越京官道",
    "x": 28,
    "y": -48
  },
  {
    "id": "border_field",
    "name": "边田荒地",
    "region": "越京官道",
    "x": 28,
    "y": -56
  },
  {
    "id": "yue_watch_pass",
    "name": "越境望隘",
    "region": "越京官道",
    "x": 28,
    "y": -64
  },
  {
    "id": "loose_camp_gate",
    "name": "散修地棚口",
    "region": "太南散修坊",
    "x": 42,
    "y": 42
  },
  {
    "id": "loose_camp_square",
    "name": "散修棚市",
    "region": "太南散修坊",
    "x": 48,
    "y": 48
  },
  {
    "id": "loose_market_lane",
    "name": "杂货棚巷",
    "region": "太南散修坊",
    "x": 54,
    "y": 54
  },
  {
    "id": "loose_medicine_tent",
    "name": "药帐",
    "region": "太南散修坊",
    "x": 60,
    "y": 60
  },
  {
    "id": "loose_diviner_mat",
    "name": "卜席角",
    "region": "太南散修坊",
    "x": 66,
    "y": 66
  },
  {
    "id": "loose_training_ground",
    "name": "试手空地",
    "region": "太南散修坊",
    "x": 72,
    "y": 72
  },
  {
    "id": "loose_stone_forest",
    "name": "乱石林",
    "region": "太南散修坊",
    "x": 78,
    "y": 78
  },
  {
    "id": "loose_hidden_pool",
    "name": "暗池",
    "region": "太南散修坊",
    "x": 84,
    "y": 84
  },
  {
    "id": "loose_guest_hall",
    "name": "借宿大棚",
    "region": "太南散修坊",
    "x": 90,
    "y": 90
  },
  {
    "id": "loose_bamboo_stage",
    "name": "竹台",
    "region": "太南散修坊",
    "x": 96,
    "y": 96
  },
  {
    "id": "spirit_beast_outer_gate",
    "name": "灵兽山外门",
    "region": "灵兽山外岭",
    "x": 4,
    "y": 48
  },
  {
    "id": "spirit_beast_broker_lane",
    "name": "兽材街",
    "region": "灵兽山外岭",
    "x": -4,
    "y": 48
  },
  {
    "id": "spirit_beast_beast_pen",
    "name": "外山兽栏",
    "region": "灵兽山外岭",
    "x": -12,
    "y": 48
  },
  {
    "id": "spirit_beast_insect_garden",
    "name": "灵虫圃",
    "region": "灵兽山外岭",
    "x": -20,
    "y": 48
  },
  {
    "id": "spirit_beast_herb_ridge",
    "name": "饲草岭",
    "region": "灵兽山外岭",
    "x": -28,
    "y": 48
  },
  {
    "id": "spirit_beast_worm_marsh",
    "name": "虫泽",
    "region": "灵兽山外岭",
    "x": -36,
    "y": 48
  },
  {
    "id": "spirit_beast_taming_yard",
    "name": "驯兽场",
    "region": "灵兽山外岭",
    "x": -44,
    "y": 48
  },
  {
    "id": "spirit_beast_inner_path",
    "name": "内山小径",
    "region": "灵兽山外岭",
    "x": -52,
    "y": 48
  },
  {
    "id": "spirit_beast_bone_cave",
    "name": "兽骨洞",
    "region": "灵兽山外岭",
    "x": -60,
    "y": 48
  },
  {
    "id": "spirit_beast_cliff_nest",
    "name": "崖巢",
    "region": "灵兽山外岭",
    "x": -68,
    "y": 48
  },
  {
    "id": "spirit_beast_stream",
    "name": "驭兽溪",
    "region": "灵兽山外岭",
    "x": -76,
    "y": 48
  },
  {
    "id": "spirit_beast_hall",
    "name": "外山执事堂",
    "region": "灵兽山外岭",
    "x": -84,
    "y": 48
  },
  {
    "id": "harbor_backbay",
    "name": "后湾口",
    "region": "天南外港",
    "x": 20,
    "y": 90
  },
  {
    "id": "harbor_fish_lane",
    "name": "鱼棚街",
    "region": "天南外港",
    "x": 26,
    "y": 96
  },
  {
    "id": "harbor_salt_house",
    "name": "晒盐棚",
    "region": "天南外港",
    "x": 32,
    "y": 102
  },
  {
    "id": "harbor_net_field",
    "name": "晒网场",
    "region": "天南外港",
    "x": 38,
    "y": 108
  },
  {
    "id": "harbor_cliff_ladder",
    "name": "崖梯",
    "region": "天南外港",
    "x": 44,
    "y": 114
  },
  {
    "id": "harbor_drift_shore",
    "name": "漂木滩",
    "region": "天南外港",
    "x": 50,
    "y": 120
  },
  {
    "id": "harbor_tide_pool",
    "name": "潮池",
    "region": "天南外港",
    "x": 56,
    "y": 126
  },
  {
    "id": "harbor_reef_steps",
    "name": "礁阶",
    "region": "天南外港",
    "x": 62,
    "y": 132
  },
  {
    "id": "harbor_lamp_tower",
    "name": "后湾灯塔",
    "region": "天南外港",
    "x": 68,
    "y": 138
  },
  {
    "id": "harbor_hidden_cove",
    "name": "暗湾",
    "region": "天南外港",
    "x": 74,
    "y": 144
  },
  {
    "id": "outer_isles_wharf",
    "name": "群岛小埠",
    "region": "乱星近海群岛",
    "x": 8,
    "y": 98
  },
  {
    "id": "outer_isles_market",
    "name": "珠市棚",
    "region": "乱星近海群岛",
    "x": 0,
    "y": 98
  },
  {
    "id": "outer_isles_shell_beach",
    "name": "贝滩",
    "region": "乱星近海群岛",
    "x": -8,
    "y": 98
  },
  {
    "id": "outer_isles_palm_ridge",
    "name": "风榈脊",
    "region": "乱星近海群岛",
    "x": -16,
    "y": 98
  },
  {
    "id": "outer_isles_storm_tree",
    "name": "雷木坡",
    "region": "乱星近海群岛",
    "x": -24,
    "y": 98
  },
  {
    "id": "outer_isles_lagoon",
    "name": "月泻潟湖",
    "region": "乱星近海群岛",
    "x": -32,
    "y": 98
  },
  {
    "id": "outer_isles_coral_path",
    "name": "珊瑚径",
    "region": "乱星近海群岛",
    "x": -40,
    "y": 98
  },
  {
    "id": "outer_isles_black_reef",
    "name": "黑礁外缘",
    "region": "乱星近海群岛",
    "x": -48,
    "y": 98
  },
  {
    "id": "outer_isles_moon_cove",
    "name": "望月湾",
    "region": "乱星近海群岛",
    "x": -56,
    "y": 98
  },
  {
    "id": "outer_isles_watch_altar",
    "name": "听潮坛",
    "region": "乱星近海群岛",
    "x": -64,
    "y": 98
  },
  {
    "id": "xutian_void_rift",
    "name": "裂隙口",
    "region": "虚天残区",
    "x": 78,
    "y": 126
  },
  {
    "id": "xutian_ash_corridor",
    "name": "灰廊",
    "region": "虚天残区",
    "x": 78,
    "y": 134
  },
  {
    "id": "xutian_shard_steps",
    "name": "碎阶",
    "region": "虚天残区",
    "x": 78,
    "y": 142
  },
  {
    "id": "xutian_rune_garden",
    "name": "残纹庭",
    "region": "虚天残区",
    "x": 78,
    "y": 150
  },
  {
    "id": "xutian_broken_stair",
    "name": "断升台",
    "region": "虚天残区",
    "x": 78,
    "y": 158
  },
  {
    "id": "xutian_shadow_pool",
    "name": "影池",
    "region": "虚天残区",
    "x": 78,
    "y": 166
  },
  {
    "id": "xutian_silent_vault",
    "name": "静库",
    "region": "虚天残区",
    "x": 78,
    "y": 174
  },
  {
    "id": "xutian_crystal_bridge",
    "name": "晶桥",
    "region": "虚天残区",
    "x": 78,
    "y": 182
  },
  {
    "id": "xutian_star_pit",
    "name": "星坑",
    "region": "虚天残区",
    "x": 78,
    "y": 190
  },
  {
    "id": "xutian_endless_wall",
    "name": "无尽壁",
    "region": "虚天残区",
    "x": 78,
    "y": 198
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
    "from": "qixuan_backslope",
    "to": "wanderer_trail"
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
    "from": "jiayuan_east_gate",
    "to": "escort_post"
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
    "from": "tainan_gate",
    "to": "loose_camp_gate"
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
    "from": "huangfeng_foothill",
    "to": "spirit_beast_outer_gate"
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
    "from": "tiannan_harbor",
    "to": "harbor_backbay"
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
    "from": "chaos_sea_port",
    "to": "outer_isles_wharf"
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
  },
  {
    "from": "xutian_inner_gate",
    "to": "xutian_void_rift"
  },
  {
    "from": "wanderer_trail",
    "to": "wanderer_camp"
  },
  {
    "from": "wanderer_camp",
    "to": "wanderer_creek"
  },
  {
    "from": "wanderer_creek",
    "to": "wanderer_brush"
  },
  {
    "from": "wanderer_brush",
    "to": "wanderer_stone_slope"
  },
  {
    "from": "wanderer_stone_slope",
    "to": "wanderer_old_temple"
  },
  {
    "from": "wanderer_old_temple",
    "to": "wanderer_firefield"
  },
  {
    "from": "wanderer_firefield",
    "to": "wanderer_fox_den"
  },
  {
    "from": "wanderer_fox_den",
    "to": "wanderer_watch_tower"
  },
  {
    "from": "wanderer_watch_tower",
    "to": "wanderer_cold_pool"
  },
  {
    "from": "escort_post",
    "to": "escort_road"
  },
  {
    "from": "escort_road",
    "to": "escort_wayside"
  },
  {
    "from": "escort_wayside",
    "to": "bandit_gully"
  },
  {
    "from": "bandit_gully",
    "to": "broken_bridge"
  },
  {
    "from": "broken_bridge",
    "to": "relay_station"
  },
  {
    "from": "relay_station",
    "to": "granary_yard"
  },
  {
    "from": "granary_yard",
    "to": "night_inn"
  },
  {
    "from": "night_inn",
    "to": "border_field"
  },
  {
    "from": "border_field",
    "to": "yue_watch_pass"
  },
  {
    "from": "loose_camp_gate",
    "to": "loose_camp_square"
  },
  {
    "from": "loose_camp_square",
    "to": "loose_market_lane"
  },
  {
    "from": "loose_market_lane",
    "to": "loose_medicine_tent"
  },
  {
    "from": "loose_medicine_tent",
    "to": "loose_diviner_mat"
  },
  {
    "from": "loose_diviner_mat",
    "to": "loose_training_ground"
  },
  {
    "from": "loose_training_ground",
    "to": "loose_stone_forest"
  },
  {
    "from": "loose_stone_forest",
    "to": "loose_hidden_pool"
  },
  {
    "from": "loose_hidden_pool",
    "to": "loose_guest_hall"
  },
  {
    "from": "loose_guest_hall",
    "to": "loose_bamboo_stage"
  },
  {
    "from": "spirit_beast_outer_gate",
    "to": "spirit_beast_broker_lane"
  },
  {
    "from": "spirit_beast_broker_lane",
    "to": "spirit_beast_beast_pen"
  },
  {
    "from": "spirit_beast_beast_pen",
    "to": "spirit_beast_insect_garden"
  },
  {
    "from": "spirit_beast_insect_garden",
    "to": "spirit_beast_herb_ridge"
  },
  {
    "from": "spirit_beast_herb_ridge",
    "to": "spirit_beast_worm_marsh"
  },
  {
    "from": "spirit_beast_worm_marsh",
    "to": "spirit_beast_taming_yard"
  },
  {
    "from": "spirit_beast_taming_yard",
    "to": "spirit_beast_inner_path"
  },
  {
    "from": "spirit_beast_inner_path",
    "to": "spirit_beast_bone_cave"
  },
  {
    "from": "spirit_beast_bone_cave",
    "to": "spirit_beast_cliff_nest"
  },
  {
    "from": "spirit_beast_cliff_nest",
    "to": "spirit_beast_stream"
  },
  {
    "from": "spirit_beast_stream",
    "to": "spirit_beast_hall"
  },
  {
    "from": "harbor_backbay",
    "to": "harbor_fish_lane"
  },
  {
    "from": "harbor_fish_lane",
    "to": "harbor_salt_house"
  },
  {
    "from": "harbor_salt_house",
    "to": "harbor_net_field"
  },
  {
    "from": "harbor_net_field",
    "to": "harbor_cliff_ladder"
  },
  {
    "from": "harbor_cliff_ladder",
    "to": "harbor_drift_shore"
  },
  {
    "from": "harbor_drift_shore",
    "to": "harbor_tide_pool"
  },
  {
    "from": "harbor_tide_pool",
    "to": "harbor_reef_steps"
  },
  {
    "from": "harbor_reef_steps",
    "to": "harbor_lamp_tower"
  },
  {
    "from": "harbor_lamp_tower",
    "to": "harbor_hidden_cove"
  },
  {
    "from": "outer_isles_wharf",
    "to": "outer_isles_market"
  },
  {
    "from": "outer_isles_market",
    "to": "outer_isles_shell_beach"
  },
  {
    "from": "outer_isles_shell_beach",
    "to": "outer_isles_palm_ridge"
  },
  {
    "from": "outer_isles_palm_ridge",
    "to": "outer_isles_storm_tree"
  },
  {
    "from": "outer_isles_storm_tree",
    "to": "outer_isles_lagoon"
  },
  {
    "from": "outer_isles_lagoon",
    "to": "outer_isles_coral_path"
  },
  {
    "from": "outer_isles_coral_path",
    "to": "outer_isles_black_reef"
  },
  {
    "from": "outer_isles_black_reef",
    "to": "outer_isles_moon_cove"
  },
  {
    "from": "outer_isles_moon_cove",
    "to": "outer_isles_watch_altar"
  },
  {
    "from": "xutian_void_rift",
    "to": "xutian_ash_corridor"
  },
  {
    "from": "xutian_ash_corridor",
    "to": "xutian_shard_steps"
  },
  {
    "from": "xutian_shard_steps",
    "to": "xutian_rune_garden"
  },
  {
    "from": "xutian_rune_garden",
    "to": "xutian_broken_stair"
  },
  {
    "from": "xutian_broken_stair",
    "to": "xutian_shadow_pool"
  },
  {
    "from": "xutian_shadow_pool",
    "to": "xutian_silent_vault"
  },
  {
    "from": "xutian_silent_vault",
    "to": "xutian_crystal_bridge"
  },
  {
    "from": "xutian_crystal_bridge",
    "to": "xutian_star_pit"
  },
  {
    "from": "xutian_star_pit",
    "to": "xutian_endless_wall"
  }
]
