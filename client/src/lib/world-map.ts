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
  { id: 'qixuan_hall', name: '七玄门议事堂', region: '七玄门', x: 14, y: 10 },
  { id: 'qixuan_square', name: '七玄门外场', region: '七玄门', x: 14, y: 22 },
  { id: 'jiayuan_market', name: '嘉元城集市', region: '嘉元城', x: 32, y: 22 },
  { id: 'bamboo_forest', name: '青竹林', region: '嘉元城郊', x: 50, y: 18 },
  { id: 'backhill_cave', name: '后山洞窟', region: '后山', x: 66, y: 18 },
  { id: 'tainan_fair', name: '太南小会', region: '太南谷', x: 30, y: 40 },
  { id: 'yanyue_peak', name: '掩月峰前坪', region: '掩月宗', x: 30, y: 58 },
  { id: 'huangfeng_outpost', name: '黄枫谷外营', region: '黄枫谷', x: 14, y: 40 },
  { id: 'huangfeng_hall', name: '黄枫谷偏殿', region: '黄枫谷', x: 14, y: 56 },
  { id: 'huangfeng_foothill', name: '枫岭山麓', region: '黄枫谷外山', x: 4, y: 40 },
  { id: 'blood_gate', name: '血禁石门', region: '血色禁地', x: 16, y: 72 },
  { id: 'blood_forbidden', name: '血色禁地外围', region: '血色禁地', x: 34, y: 72 },
  { id: 'tiannan_harbor', name: '天南港', region: '天南海岸', x: 4, y: 72 },
  { id: 'chaos_sea_port', name: '乱星海近港', region: '乱星海', x: 4, y: 86 },
  { id: 'chaos_sea_isle', name: '残碑孤岛', region: '乱星海', x: 20, y: 86 },
  { id: 'xutian_hall', name: '虚天殿外殿', region: '虚天殿', x: 20, y: 96 },
]

export const worldMapEdges: WorldMapEdge[] = [
  { from: 'qixuan_hall', to: 'qixuan_square' },
  { from: 'qixuan_square', to: 'jiayuan_market' },
  { from: 'jiayuan_market', to: 'bamboo_forest' },
  { from: 'bamboo_forest', to: 'backhill_cave' },
  { from: 'jiayuan_market', to: 'tainan_fair' },
  { from: 'tainan_fair', to: 'yanyue_peak' },
  { from: 'tainan_fair', to: 'huangfeng_outpost' },
  { from: 'huangfeng_outpost', to: 'huangfeng_hall' },
  { from: 'huangfeng_outpost', to: 'huangfeng_foothill' },
  { from: 'huangfeng_hall', to: 'blood_gate' },
  { from: 'blood_gate', to: 'blood_forbidden' },
  { from: 'blood_gate', to: 'tiannan_harbor' },
  { from: 'tiannan_harbor', to: 'chaos_sea_port' },
  { from: 'chaos_sea_port', to: 'chaos_sea_isle' },
  { from: 'chaos_sea_isle', to: 'xutian_hall' },
]
