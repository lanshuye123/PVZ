# 变更记录 · 存档不保存问题的排查实录

| 项 | 值 |
|---|---|
| 日期 | 2026-10-07 |
| 提交 | — （无代码改动，本文是方法论记录） |
| 类型 | 记录 |
| 影响 | 存档 / 环境 |
| 涉及文件 | 无（排查期间临时插桩，已全部还原） |

> **这不是代码改动。** 它是 [03 · 可配置的存档位置](2026-10-07-03-configurable-save-data.md)
> 的直接来由，也是一次值得复用的排查范例，所以并入本批。

## 症状

**每次启动游戏都是全新档** —— 新建用户、玩一会儿、退出、再启动，进度全没了。

## 为什么静态读代码得不出结论

先做的是代码审查，逐项检查后**每一条结论都是"没问题"**：

| 检查项 | 结论 |
|---|---|
| `gAppDataFolder` 解析逻辑 | ✅ 正确 |
| 读写路径是否一致 | ✅ 一致（`userdata/users.dat`、`userdata/user%d.dat`） |
| PAK 虚拟文件系统会不会挡住读取 | ✅ 不会 —— `p_fopen` 在 `PakInterface.cpp:212` **确实**回退到真实文件系统 `fopen` |
| 存档目录的 ACL | ✅ 正常（沙箱 ACL 诊断脚本判定 `NOT_THIS_CLASS`，调用者拥有 `FullControl`，owner 就是当前用户） |
| 磁盘上是否已有存档 | ✅ 找不到任何 `userdata\` 目录（说明**写入从未成功**，而不是读不到） |
| 游戏是否降低了自身完整性 | ✅ 没有（全库 grep 完整性相关 API，命中的只有我自己插的探针） |

**以上全部正确，但存档依然不保存。**

静态分析的盲区在于 **运行时的进程上下文** —— 文件权限、令牌完整性这类东西，
代码里看不出来。

## 有效手法：分层探针

往关键路径插临时日志（写到一个固定文件），逐层缩小范围。
每层只回答一个问题：

| 层 | 探针内容 | 得到的结论 |
|---|---|---|
| 1 | `ResolveSaveDataFolder` 的返回值 | 路径解析**正确**：`C:\ProgramData\PopCap Games\PlantsVsZombies\` |
| 2 | `ProfileMgr::Load` 的完整路径 + 读取结果 | 读路径**正确**，只是文件不存在 |
| 3 | `ProfileMgr::Save` 是否被调用 | 未被调用（无 GUI 输入时不触发，符合预期） |
| 4 | 直接调 `CreateDirectoryA` 试写 | **失败** —— `lasterr=5`（ACCESS_DENIED） |
| 5 | 在 `MkDir` 内部逐步 `_mkdir` 并记录 `errno` | 父目录 `errno=17`(EEXIST) 正常，**最后一级 `errno=13`(EACCES)** |
| 6 | `GetTokenInformation(TokenIntegrityLevel)` 读自身完整性 | **`RID=4096`（Low）** ← 决定性证据 |

第 6 层是关键。拿到它之后，前面所有矛盾一次性解释通了。

第 5 层的原始输出：

```
MkDir: input='C:\ProgramData\PopCap Games\PlantsVsZombies\userdata' len=52
MkDir:   step _mkdir('C:')                                          -> -1 errno=17
MkDir:   step _mkdir('C:\ProgramData')                              -> -1 errno=17
MkDir:   step _mkdir('C:\ProgramData\PopCap Games')                 -> -1 errno=17
MkDir:   step _mkdir('C:\ProgramData\PopCap Games\PlantsVsZombies') -> -1 errno=17
MkDir:   final _mkdir('C:\ProgramData\PopCap Games\PlantsVsZombies\userdata') -> -1 errno=13
```

（`errno=17` = EEXIST，父目录本来就存在，正常；只有最后一级失败。）

第 4/6 层的对照输出：

```
Probe: integrity RID=4096 (4096=Low 8192=Medium 12288=High)
Probe: CreateDirectoryA('C:\ProgramData\PopCap Games\PlantsVsZombies\userdata')=0 lasterr=5
Probe: CreateDirectoryA('...\stabledecompile\build\_logs\userdata')=1 lasterr=0
Probe: CreateDirectoryA('C:\Users\...\AppData\Local\Temp\pvz_ud')=0 lasterr=5
```

## 结论

**这不是游戏代码的问题。**

1. 可执行文件继承了构建环境的 **Low 完整性标签**
   （`icacls` 显示 `Mandatory Label\Low Mandatory Level:(I)(NW)`）；
2. Windows 按可执行文件自身的完整性标签决定进程的完整性级别 →
   游戏以 **Low 完整性** 运行；
3. Windows 的**强制完整性控制（MIC）禁止"向上写"** —— Low 进程不能写入
   Medium 完整性的位置，包括 `%PROGRAMDATA%`、`%APPDATA%`、`%TEMP%`；
4. 于是 `MkDir` 建不出 `userdata\`，`fopen(..., "w+b")` 报 `ENOENT`；
5. 每次启动都读不到档案 → **表现为全新档**。

反证（同一份 exe，只是去掉标签）：

| 做法 | 运行时完整性 | 建目录结果 |
|---|---|---|
| 工作区里的 exe（带 Low 标签） | `RID=4096` Low | `lasterr=5` 失败 |
| 复制到工作区外（无标签） | `RID=8192` Medium | `lasterr=0` 成功 |
| 就地 `icacls /setintegritylevel Medium` | `RID=8192` Medium | `lasterr=0` 成功 |

## 两个容易踩的坑

### 坑一：`errno` 要配合 `GetLastError()` 一起看

`_mkdir` 返回的 `errno=13`(EACCES) 只是 Win32 错误 `5`(ACCESS_DENIED) 的映射，
信息量更少。直接用 Win32 API 能拿到原始错误码：

```cpp
SetLastError(0);
BOOL r = CreateDirectoryA(dir, NULL);
DWORD e = GetLastError();   // 5=拒绝访问  3=路径不存在  183=已存在
```

### 坑二：「能写 A 不能写 B」是 MIC 的典型特征

一开始以为只有 `%PROGRAMDATA%` 有问题。实测 Low 进程对
`C:\ProgramData\...`、`%APPDATA%\...\Temp\...` 都是 `lasterr=5`，
**只有工作区（同样带 Low 标签）能写**。

见到"某些目录能写、另一些不能"这种**按目录分界**的权限现象，
就应该直接怀疑完整性级别，而不是去逐目录查 ACL。

## 这次排查对代码的正面影响

因为"目录有没有被创建"是最省事的判据，[03](2026-10-07-03-configurable-save-data.md)
特意让游戏**启动时就创建**所选根目录和 `userdata\`。
以后再遇到"存档不保存"，看一眼目录在不在就能分流，
**不必再插一遍探针**。

## 相关

- 症状速查与完整诊断脚本 →
  [05-故障排查 · 存档完全无法固化](../05-故障排查.md#症状存档完全无法固化每次启动都是全新的)
- 由此催生的功能 → [03 · 可配置的存档位置](2026-10-07-03-configurable-save-data.md)
- 用户手册 → [04-存档配置](../04-存档配置.md)
