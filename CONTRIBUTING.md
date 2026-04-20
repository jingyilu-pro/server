# Contributing

感谢你愿意改进这个仓库。

## 开始之前

- 先阅读 [README.md](README.md)、[INSTALL.md](INSTALL.md) 和 [CODE_STYLE.md](CODE_STYLE.md)。
- 涉及玩法、协议或内容结构的改动，优先补文档，再补实现。
- 大改动请先开 issue 或讨论，避免多人在同一方向上重复返工。

## 开发环境

- 克隆仓库后请同步子模块：

```bash
git clone --recursive https://github.com/jingyilu-pro/server
cd server
git submodule update --init --recursive
```

- 常用验证命令：

```bash
node scripts/build_mud_world.mjs
npm --prefix client test -- --run
npm --prefix client run build
wsl bash -lc "cd /mnt/c/Work/Projects/server && cmake --build build-wsl-main --target application mud_smoke -j10"
```

## 提交建议

- 保持提交聚焦，一次只解决一条明确问题。
- 提交说明优先写“为什么改”，而不是只写“改了什么”。
- 修改接口、文档或玩法闭环时，请同时补齐相应说明与验证。
- 不要把本地临时文件、构建产物、私有配置或执行笔记提交到仓库。

## Pull Request 期望

- 说明问题背景、改动范围和验证方式。
- 如果改动影响客户端、协议、世界数据或烟测，请在描述里明确写出。
- 涉及界面变化时，附截图或录屏。
- 涉及平衡性、运营节奏或玩法入口时，说明回退方式。

## 许可证约定

向本仓库提交代码、文档或其他受版权保护的内容，即表示你同意这些贡献在仓库中按 [MIT License](LICENSE) 分发。

第三方代码、子模块与外部资源仍遵循其各自许可证，不受本条覆盖。

另请注意：[OPEN_SOURCE_SCOPE.md](OPEN_SOURCE_SCOPE.md) 中明确排除的“凡人修仙”题材方案、策划、剧情资料与运营材料，不属于本仓库对外开源授权范畴；向这些范围提交内容，不当然意味着该内容会被作为 MIT 开源素材对外分发。
