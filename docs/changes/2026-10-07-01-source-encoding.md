# 变更记录 · 修复源码编码导致的编译失败

| 项 | 值 |
|---|---|
| 日期 | 2026-10-07 |
| 提交 | `e2636d3` — Fix compilation under non-UTF-8 system locales |
| 类型 | 修复（构建阻断） |
| 影响 | 构建 |
| 涉及文件 | `SexyAppFramework/SexyAppBase.vcxproj`、7 个源文件、`.gitignore` |

## 背景与症状

在中文 Windows（ANSI 代码页 **CP936**）上首次构建，出现两类**硬错误**，完全无法编译：

```
LawnApp.cpp(1900,91):   error C2001: 常量中有换行符
LawnApp.cpp(1900,1):    error C1057: 宏扩展中遇到意外的文件结束

EditWidget.cpp(271,46): error C3873: "0xf8f5": 不允许将此字符作为标识符的第一个字符
EditWidget.cpp(271,46): error C3688: 文本后缀""无效
EditWidget.cpp(271,101):error C2001: 常量中有换行符
EditWidget.cpp(271,101):error C2015: 常量中的字符太多
EditWidget.cpp(271,101):error C2059: 语法错误:"常数"
EditWidget.cpp(272,25): error C2143: 语法错误: 缺少")"
```

外加 **1158 条** `warning C4819: 该文件包含不能在当前代码页(936)中表示的字符`。

关键是：**同样的代码在英文/西欧系统（CP1252）上能编过** —— 说明问题不在代码逻辑，
而在"源码字符集依赖编译机器区域设置"。

## 根因

扫描全部 1066 个源文件，得到三种编码混用：

| 编码 | 数量 | 后果 |
|---|---|---|
| 纯 ASCII | 990 | 无关紧要 |
| UTF-8（无 BOM） | 63 | 在 CP936 下被按 GBK 解析 |
| UTF-8 with BOM | 6 | MSVC 能正确识别 |
| **其他编码** | **7** | 4 个 CP936 + 3 个 CP1252 |

而项目**没有指定源码字符集**，MSVC 于是按系统 ANSI 代码页解析。

### 证据 1：`LawnApp.cpp:1900` 的收尾引号被吞

```cpp
LawnDialog* aComfirmDialog = (LawnDialog*)DoDialog(
    Dialogs::DIALOG_STORE_PURCHASE, true, _S("买下这个物品？"), ...);
```

`？` 是 U+FF1F，UTF-8 字节为 `EF BC 9F`。按 CP936 解析时：

1. `EF BC` 被当作一个双字节汉字；
2. 剩下的 `9F` 是 CP936 的**首字节**，于是与后面的 `"`（0x22）配成第二个汉字 ——
   **字符串字面量的收尾引号被吃掉**。

编译器于是把后面整段代码都当成字符串内容，一路读到行尾才报 `C2001`，宏展开随之
失衡报 `C1057`。

### 证据 2：`EditWidget.cpp:271` 的裸 Latin-1 字节

源码原始字节（用 hexdump 确认）：

```
offset 6295:  4C 27 C0 27 29   →  L ' À(0xC0) ' )
offset 6346:  4C 27 FF 27 29   →  L ' ÿ(0xFF) ' )
```

即：

```cpp
(((unsigned int)theChar >= (unsigned int)(L'\xC0')) &&
 ((unsigned int)theChar <= (unsigned int)(L'\xFF')))
```

这是**真实代码**（用 0xC0–0xFF 做 Latin-1 范围判断），不是注释。在 CP936 下这两个
字节被当成 DBCS 首字节，于是字面量彻底崩坏，连锁报出 `C3873`/`C3688`/`C2001`/
`C2015`/`C2059`/`C2143`。

> 这一条特别值得注意：**它不是注释里的编码问题，改注释没用。**

## 改动

### a. 8 个配置统一指定源码字符集

`SexyAppFramework/SexyAppBase.vcxproj` 的**全部 8 个** `ClCompile` 配置：

```xml
<AdditionalOptions>/source-charset:utf-8 /wd4996 %(AdditionalOptions)</AdditionalOptions>
```

只加在 `ClCompile` 上。另有 2 处 `Link` 的 `/INCREMENTAL`，**未改动**。

### b. 7 个非 UTF-8 文件按各自真实编码转成 UTF-8 with BOM

**没有一刀切**，每个文件都先确认了真实编码：

| 文件 | 原编码 | 判据（原始字节 → 解码结果） |
|---|---|---|
| `Lawn/Cutscene.h` | CP936 | `A1 BE CD C6 CF FA B4 F7 B7 F2 B5 BD BC C6 CA B1 A1 BF` → 「推销戴夫倒计时」 |
| `Lawn/System/DataSync.h` | CP936 | `CE B4 D5 D2 B5 BD` → 「未找到」 |
| `Lawn/Widget/ContinueDialog.cpp` | CP936 | `A1 A2` → 「、」 |
| `Lawn/Widget/NewOptionsDialog.cpp` | CP936 | `A1 A2` → 「、」 |
| `SexyAppFramework/EditWidget.cpp` | CP1252 | `C0` / `FF` → `À` / `ÿ` |
| `include/dx8sdk/edevdefs.h` | CP1252 | `92` → `’`（device's） |
| `include/dx8sdk/xprtdefs.h` | CP1252 | `92` → `’`（device's） |

**为什么用 BOM 而不是无 BOM 的 UTF-8**：带 BOM 时 MSVC **优先按 BOM** 判断编码，
与系统代码页无关。即使以后有人误删 `/source-charset:utf-8`，这几个文件也不会退化。

**为什么用 `/source-charset:utf-8` 而不是 `/utf-8`**：后者会同时把**执行字符集**
改成 UTF-8，而游戏里大量 narrow 字面量默认期望系统 ANSI（CP936）。
只改源码字符集能保持原有落地行为。

### c. `.gitignore` 增加 `/_encoding_backup/`

排查期间保留了 7 个原文件的字节级备份（**这是唯一保存原始编码的地方**），
放在仓库根但不入库。

## 验证

```powershell
$msbuild = "D:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe"
& $msbuild PlantsVsZombies.sln /p:Configuration=ReleaseGOTY /p:Platform=x64 /m `
    "/flp:logfile=build.log;verbosity=normal"

# 统计日志（日志是 GBK 编码，需指定代码页）
$enc = [System.Text.Encoding]::GetEncoding(936)
$t = [IO.File]::ReadAllText("build.log", $enc)
"errors : " + ([regex]::Matches($t, ': error ')).Count    # 期望 0
"C4819  : " + ([regex]::Matches($t, 'C4819')).Count       # 期望 0（此前 1158）
"C4566  : " + ([regex]::Matches($t, 'C4566')).Count       # 期望 0（无字面量字符丢失）
```

实测结果：**errors = 0，C4819 = 0，C4566 = 0**；
ReleaseGOTY|x64 与 DebugGOTY|x64 均 `已成功生成`。

`C4566` 为 0 这一点很重要 —— 它证明**没有任何字符从字符串字面量里丢失**，
中文仍按 CP936 正确落进可执行文件。

## 回退

```powershell
git revert e2636d3
```

手工回退要点：

1. 把 `_encoding_backup\` 下的 7 个文件复制回原位（恢复原始字节）；
2. 移除 8 处 `/source-charset:utf-8`。

**回退后果**：在 CP936 等非 UTF-8 区域设置的中文系统上将**无法编译**。

## 相关

- 背景与自查脚本 → [02-开发参考 §6 源文件编码约定](../02-开发参考.md#6-源文件编码约定)
- 报错速查 → [05-故障排查 · C2001/C1057](../05-故障排查.md#症状error-c2001-常量中有换行符--c1057-宏扩展中遇到意外的文件结束--c3873--c3688)
- 换成其它第三方头文件（如升级 `include/dx8sdk/`）时，**记得重复第 b 步的编码转换**
