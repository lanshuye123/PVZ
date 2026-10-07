# stabledecompile 文档

本目录是 **stabledecompile**（植物大战僵尸一代社区反编译工程）的中文文档。
所有内容以本仓库当前代码为准，与仓库根目录的 `README.md`（英文）互为补充；
若两者冲突，以代码和本目录为准。

## 阅读路线

| 文档 | 面向谁 | 内容 |
|---|---|---|
| [01-项目概述](01-项目概述.md) | 所有人 | 这个工程是什么、能做什么、技术栈现状、已知问题 |
| [02-开发参考](02-开发参考.md) | 改代码的人 | 代码分层、构建配置矩阵、渲染/资源/存档系统、编码约定与坑 |
| [03-构建指南](03-构建指南.md) | 想编译的人 | 从零到跑起来：环境、游戏资源、选配置、构建、产物 |
| [04-存档配置](04-存档配置.md) | 玩家 / 打包分发的人 | `savedata.ini` 五种存档模式（本仓库新增功能） |
| [05-故障排查](05-故障排查.md) | 所有人 | 症状 → 原因 → 解决，按报错信息索引 |
| [changes/](changes/README.md) | 想了解/回退历次改动的人 | **变更记录**：按时间顺序，含每项改动的根因、验证与回退 |

## 快速开始

```powershell
# 1. 把正版游戏目录放到仓库根目录，命名为 "Plants Vs. Zombies"
#    里面要有 main.pak 和 properties/

# 2. 编译（GOTY 版，x64）
$msbuild = "D:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe"
& $msbuild PlantsVsZombies.sln /p:Configuration=ReleaseGOTY /p:Platform=x64 /m

# 3. 运行
.\build\ReleaseGOTY_x64\bin\PlantsVsZombies.exe
```

存档位置的默认值与修改方式见 [04-存档配置](04-存档配置.md)。

## 文档约定

- 命令示例默认在仓库根目录执行，shell 为 PowerShell。
- 路径中的 `D:\Program Files\Microsoft Visual Studio\2022\Community\` 是本文档作者的
  VS2022 安装位置，请按自己的实际路径替换。
- 标记为 **（已验证）** 的结论都在本仓库的 Windows x64 + VS2022 环境下实测过；
  其余来自代码阅读或上游说明。
