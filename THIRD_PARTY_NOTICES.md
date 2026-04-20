# Third-Party Notices

本仓库的项目自有代码按 [MIT License](LICENSE) 发布。

凡人修仙题材相关的方案、资料、设定与运营文档不属于本仓库的开源授权范围，详见 [OPEN_SOURCE_SCOPE.md](OPEN_SOURCE_SCOPE.md)。

仓库中的第三方子模块、依赖代码和外部资源不因主仓库采用 MIT 而自动改为 MIT。它们继续遵循各自上游项目的许可证与声明。

## Git Submodules

| Component | Path | Upstream | Bundled license entry |
| --- | --- | --- | --- |
| fmt | `libs/fmt` | `https://github.com/fmtlib/fmt` | `libs/fmt/LICENSE` |
| libevent | `libs/libevent` | `https://github.com/libevent/libevent` | `libs/libevent/LICENSE` |
| jansson | `libs/jansson` | `https://github.com/akheron/jansson` | `libs/jansson/LICENSE` |
| curl | `libs/curl` | `https://github.com/curl/curl` | `libs/curl/COPYING` |
| hiredis | `libs/hiredis` | `https://github.com/redis/hiredis` | `libs/hiredis/COPYING` |
| libjwt | `libs/libjwt` | `https://github.com/benmcollins/libjwt` | `libs/libjwt/LICENSE` |
| MariaDB Connector/C | `libs/mariadb-connector-c` | `https://github.com/mariadb-corporation/mariadb-connector-c` | `libs/mariadb-connector-c/COPYING.LIB` |
| protobuf | `libs/protobuf` | `https://github.com/protocolbuffers/protobuf` | `libs/protobuf/LICENSE` |
| concurrentqueue | `libs/concurrentqueue` | `https://github.com/cameron314/concurrentqueue` | `libs/concurrentqueue/LICENSE.md` |
| readerwriterqueue | `libs/readerwriterqueue` | `https://github.com/cameron314/readerwriterqueue` | `libs/readerwriterqueue/LICENSE.md` |
| spdlog | `libs/spdlog` | `https://github.com/gabime/spdlog` | `libs/spdlog/LICENSE` |
| jemalloc | `libs/jemalloc` | `https://github.com/jemalloc/jemalloc` | `libs/jemalloc/COPYING` |
| Mbed TLS | `libs/mbedtls` | `https://github.com/Mbed-TLS/mbedtls` | `libs/mbedtls/LICENSE` |
| recastnavigation | `libs/recastnavigation` | `https://github.com/recastnavigation/recastnavigation` | `libs/recastnavigation/License.txt` |
| tinyxml2 | `libs/tinyxml2` | `https://github.com/leethomason/tinyxml2` | `libs/tinyxml2/LICENSE.txt` |

## Additional Notes

- `client/` 的 Node 依赖在开发或构建时由包管理器安装，这些依赖的许可证以各自包内声明为准。
- 若第三方组件升级、替换或新增，请同步更新本文件。
- 如果你计划把本仓库重新打包、分发或商用，请在分发前复核所有第三方许可证义务。
