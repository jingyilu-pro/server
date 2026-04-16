const topic = (topic_id, title, summary, body_lines, extras = {}) => ({
  topic_id,
  title,
  summary,
  body_lines,
  keywords: extras.keywords ?? [],
  related_commands: extras.related_commands ?? [],
  inline_commands: extras.inline_commands ?? [],
  category: extras.category ?? '基础',
});

export default {
  help_topics: [
    topic(
      'newbie',
      '新手入世',
      '从开号到站稳脚跟，先学会看、听、问、走。',
      [
        '你现在玩的是共享世界修仙 MUD，不是单线剧情闯关。',
        '先做三件事：look 看场景、listen 听风声、talk/ask 找人问路。',
        '看不懂时先输入 help commands 或 help work。',
        '初期最稳的节奏是：看 -> 问 -> 接差事 -> 跑一圈 -> 回来交事。',
      ],
      {
        keywords: ['新手', '入门', '上手'],
        related_commands: ['look', 'listen', 'talk', 'ask', 'board', 'work', 'journal'],
        inline_commands: ['look', 'listen', 'help commands', 'board', 'work'],
      },
    ),
    topic(
      'commands',
      '常用命令',
      '高频命令以观察、问讯、事务、修炼四组为核心。',
      [
        '观察：look / here / listen / map / rumor / who',
        '问讯：talk <人物> / ask <人物> about rumor / inspect <目标>',
        '事务：board / work / duty / tasks / journal / claim',
        '修炼：score / hp / skills / spells / practice <技能> / meditate / breakthrough',
        '社交：say / chat / tell / reply / emote / team',
        '行动：go <方向> / fight <目标> / cast <法术> <目标> / loot <物品> / harvest <资源>',
      ],
      {
        keywords: ['命令', 'commands', '指令'],
        related_commands: ['help', 'commands', 'look', 'talk', 'board', 'work', 'score'],
        inline_commands: ['look', 'talk 厉飞雨', 'ask 厉飞雨 about rumor', 'board', 'work'],
      },
    ),
    topic(
      'channels',
      '频道与说话',
      '世界、当地、队伍、门派和私聊分开走，别把所有话都扔到一个口子里。',
      [
        'say：只让同场景听见，适合眼前交谈。',
        'chat world：发到世界频道，适合问路、喊人、收消息。',
        'chat team：发到队伍频道，适合组队协作。',
        'tell <玩家> <内容>：私聊。',
        'reply <内容>：回复最近联系你的玩家。',
      ],
      {
        keywords: ['频道', '聊天', 'chat', 'say', 'tell'],
        related_commands: ['say', 'chat', 'tell', 'reply', 'team'],
        inline_commands: ['chat world 有人在太南谷吗', 'say 在下初来乍到', 'tell hanli 道友可在'],
        category: '社交',
      },
    ),
    topic(
      'work',
      '工作与营生',
      '工作是稳定收入线，不一定华丽，但能让你稳稳活到筑基准备期。',
      [
        '先用 work 看当前场景可做的工作。',
        '工作通常绑定一个发起人、一段路和一类产出。',
        '低风险工作适合积累灵石、药材、熟练度和人情。',
        '如果 board 更像公开委托，那么 work 更像你此刻手边就能开的营生。',
      ],
      {
        keywords: ['工作', '营生', '活路'],
        related_commands: ['work', 'board', 'travel', 'journal'],
        inline_commands: ['work', 'board', 'travel'],
        category: '循环',
      },
    ),
    topic(
      'freequest',
      '公众任务',
      '公众任务靠风声和板子驱动，不是只有主线才算任务。',
      [
        '先找有风声的人 talk，然后 ask <人物> about rumor。',
        '有些任务会直接挂在 board 上，有些则藏在 NPC 话里。',
        '公众任务讲究一轮一轮跑：问讯 -> 接线 -> 执行 -> 提交 -> 听后续风声。',
      ],
      {
        keywords: ['公众任务', '委托', '风声'],
        related_commands: ['ask', 'board', 'journal', 'tasks'],
        inline_commands: ['ask 墨府总管 about rumor', 'board', 'journal'],
        category: '循环',
      },
    ),
    topic(
      'board',
      '留言板',
      '有板子的地方不仅能看差事，也能看房间里留下的帖子。',
      [
        'board：看当前房间的公开委托和近帖。',
        'read <编号>：读一条帖子。',
        'post <题目>=<正文>：在当前板子留言。',
        'discard <编号>：把某条帖子从你自己的视野里收起。',
      ],
      {
        keywords: ['留言板', 'board', 'read', 'post'],
        related_commands: ['board', 'read', 'post', 'discard'],
        inline_commands: ['board', 'post 收药=后湾收一批盐壳与海灵藻', 'read 1'],
        category: '社交',
      },
    ),
    topic(
      'rank',
      '排行与头衔',
      '排行榜不是摆设，它会告诉你这个服现在最值钱的路数是什么。',
      [
        'rank：查看当前默认榜单。',
        'rank wealth / rank alchemy / rank travel：查看指定榜。',
        '首席榜只取各门派当前最强一人，偏身份荣誉。',
        '财富榜、丹道榜、游历榜更适合观察服内生态。',
      ],
      {
        keywords: ['榜', '排行', 'rank'],
        related_commands: ['rank', 'family', 'duty'],
        inline_commands: ['rank', 'rank wealth', 'rank travel'],
        category: '成长',
      },
    ),
    topic(
      'sects',
      '散修与门派',
      '这服不是强制门派服，散修也能长期玩。',
      [
        '散修线更自由，靠坊市、风声、采药、跑商、海猎慢慢起势。',
        '七玄门偏凡俗起步与江湖过渡。',
        '黄枫谷偏药园、法修和宗门规矩。',
        '灵兽山偏灵兽、灵虫、采药和照料工作。',
      ],
      {
        keywords: ['门派', '散修', '宗门'],
        related_commands: ['family', 'duty', 'join', 'contribute'],
        inline_commands: ['family', 'duty', 'join huangfeng_valley'],
        category: '身份',
      },
    ),
    topic(
      'map_tiannan',
      '天南舆图',
      '天南一线是从凡俗走向宗门和海路的主交通带。',
      [
        '七玄门与嘉元城适合起步和熟悉命令。',
        '太南谷适合换消息、接风声、学阵法。',
        '黄枫谷和灵兽山开始把你推向正式身份线。',
        '血禁和天南港则把你往更高风险的世界推过去。',
      ],
      {
        keywords: ['天南', '地图', '舆图'],
        related_commands: ['map', 'travel', 'rumor'],
        inline_commands: ['map', 'travel', 'rumor'],
        category: '地理',
      },
    ),
    topic(
      'map_chaos_sea',
      '乱星海舆图',
      '乱星海不是一张图走到底，而是要学会认潮、认礁、认风暴。',
      [
        '先从天南港和近港开始，别一上来就往深海闯。',
        '礁影浅滩和群岛小埠是海猎、采珠与跑货的基础盘。',
        '风暴航道和残碑孤岛则开始把人拉向虚天殿线。',
      ],
      {
        keywords: ['乱星海', '海路', '航道'],
        related_commands: ['map', 'travel', 'work', 'board'],
        inline_commands: ['travel', 'work', 'board'],
        category: '地理',
      },
    ),
  ],
};
