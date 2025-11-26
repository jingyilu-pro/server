整合一些服务器常用的库，之后基于这些库搭建一个游戏服务器

# 方式1：递归克隆（推荐）
git clone --recursive https://github.com/user/main-repo.git

mkdir build
cmake ..
make install

# 方式2：先克隆主项目，再初始化子模块
git clone https://github.com/user/main-repo.git
cd main-repo
git submodule init
git submodule update


# 更新所有子模块到最新提交
git submodule update --remote

# 更新特定子模块
git submodule update --remote libs/lib1

# 拉取所有子模块的更新
git submodule foreach git pull

# 如果子模块本身也包含子模块
git submodule update --init --recursive

# 添加子模块并指定深度为1
git submodule add --depth 1 https://github.com/user/repo.git

# 指定深度和路径
git submodule add --depth 1 https://github.com/user/repo.git libs/repo

# 指定深度和分支
git submodule add --depth 1 -b main https://github.com/user/repo.git