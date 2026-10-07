# 变更记录 · 可配置的存档位置

| 项 | 值 |
|---|---|
| 日期 | 2026-10-07 |
| 提交 | `772307c` — Add configurable save-data location |
| 类型 | 新功能 |
| 影响 | 存档 |
| 涉及文件 | `SexyAppFramework/Common.h`、`Common.cpp`、`SexyAppFramework/SexyAppBase.cpp`、`main.cpp`、`assets/savedata.ini`、`README.md` |

用户手册在 [04-存档配置](../04-存档配置.md)，这里讲**实现**。

## 背景

原版（以及上游工程）把存档**写死**在 `%PROGRAMDATA%\PopCap Games\PlantsVsZombies\`，
即"全机器共享"位置。带来的问题：

- 多用户共用一台机器时存档互相覆盖（`ProgramData` 是共享目录）
- 绿色/便携版无法把存档带在 U 盘上
- 多个独立 mod 实例无法各用一套存档
- 受限账户下 `ProgramData` 可能根本不可写

目标是改成由**随游戏分发的静态配置文件**决定，风格参考《Minecraft》。

## 设计决策

| 决策 | 取值 | 理由 |
|---|---|---|
| 配置文件名 | `savedata.ini` | 自描述；契合"调试/控制静态配置文件"的定位 |
| 位置 | **exe 旁边** | 随程序分发；`portable` / `instance` 模式本身就依赖这个位置 |
| 分发方式 | 放进 `assets/` | PostBuildEvent 已有 `robocopy assets → bin`，**无需改构建脚本** |
| 格式 | 极简 INI | 人类可读可改，不引入 XML/JSON 依赖 |
| 默认模式 | `user`（`%APPDATA%`） | 现代惯例（对标 `%APPDATA%\.minecraft`），并天然避开 ProgramData 的权限/完整性问题 |
| 兜底 | 未知/缺失 → `user` | 删掉配置文件也不能让游戏挂掉 |
| `path` 语义 | **根目录**，游戏在其下建 `userdata\` | 与其它四种模式行为一致、可预测 |
| 存档子目录 | 仍是 `userdata\` | 沿用游戏原有结构，改动面最小 |

## 改动

### a. `Common.h` / `Common.cpp` — 新增解析与路径设施

```cpp
typedef std::map<SexyString, SexyString> IniValueMap;

bool       LoadIniFile(const SexyString& theFileName, IniValueMap* theValues);
SexyString GetExeDirectory();                    // exe 所在目录，无尾斜杠
void       SetLaunchDirectory(const SexyString& theDir);
SexyString GetLaunchDirectory();                 // 启动时的工作目录
SexyString ResolveSaveDataFolder(const SexyString& theCompanyName,
                                 const SexyString& theProductName,
                                 const SexyString& theCommonAppData,
                                 const SexyString& theRoamingAppData);
```

**INI 解析要点**（`LoadIniFile`）：

- 支持 UTF-8 BOM（记事本另存为 UTF-8 不会出问题）
- `#` / `;` 注释；忽略 `[section]`
- 键名 `StringToLower` → **大小写不敏感**
- **注释只在其前是空白时才算注释** —— 所以路径里可以安全出现 `#` / `;`
  （例如 `path = D:\my#saves`）：

  ```cpp
  for (size_t aPos = aValue.find_first_of("#;"); aPos != std::string::npos;
       aPos = aValue.find_first_of("#;", aPos + 1))
  {
      if (aPos > 0 && (aValue[aPos - 1] == ' ' || aValue[aPos - 1] == '\t'))
      { aValue = Trim(aValue.substr(0, aPos)); break; }
  }
  ```

**模式分派**（`ResolveSaveDataFolder`，简化）：

```cpp
if      (mode == "portable") aFolder = GetExeDirectory();
else if (mode == "instance") aFolder = GetLaunchDirectory();
else if (mode == "static" && !path.empty())
    aFolder = isAbsolute(path) ? path : GetExeDirectory() + "\\" + path;
else if (mode == "legacy")   { base = commonAppData; needsSuffix = true; }
else                         { base = roamingAppData; needsSuffix = true; }  // user + 兜底
```

- `needsSuffix` 表示还要拼上 `<Company>\<Product>`
- `base` 为空（例如 `SHGetFolderPath` 失败）时退回 exe 目录，
  避免拼出 `\PopCap Games\PlantsVsZombies` 这种根路径
- `mode = static` 但 `path` 为空 → 落到最后的 `else`，即回退 `user`
  （**不会**写入一个空的相对路径）

### b. `SexyAppBase.cpp` — 替换写死路径

```cpp
SexyString aDataPath = ResolveSaveDataFolder(mFullCompanyName, mProdName,
                                            aCommonAppData, aRoamingAppData);
SetAppDataFolder(aDataPath);
MkDir(aDataPath);                        // 启动即创建根目录
MkDir(aDataPath + _S("userdata"));       // 以及 userdata，便于确认落点
```

**顺带修掉同一段代码里的两个隐患**（这两个都是真实 bug，不是清洁工作）：

1. **`SexyChar aPath[MAX_PATH];` 未初始化就读** ——
   若 `SHGetFolderPath` 失败，会拿栈上的垃圾数据去拼路径。已改为 `= { 0 }`。
2. **`SHGetFolderPathFunc aFunc` 在 `if` 内部被重复声明**：

   ```cpp
   // 修改前 —— 内层声明遮蔽了外层，外层 aFunc 永远是 NULL
   SHGetFolderPathFunc aFunc = (SHGetFolderPathFunc)GetSHGetFolderPath(_S("shell32.dll"), &aMod);
   if (aFunc == NULL || aMod == NULL)
       SHGetFolderPathFunc aFunc = (SHGetFolderPathFunc)GetSHGetFolderPath(_S("shfolder.dll"), &aMod);
   ```

   导致 `shfolder.dll` 的回退分支**从未生效**。已改为赋值。

另外把原来被注释掉的 `//MkDir(aDataPath);` 正式启用（改为无条件创建）。

### c. `main.cpp` — 捕获启动工作目录

```cpp
{
    char aLaunchDir[MAX_PATH] = { 0 };
    if (GetCurrentDirectoryA(MAX_PATH, aLaunchDir) != 0)
        Sexy::SetLaunchDirectory(aLaunchDir);
}
```

**顺序是关键**：`SexyAppBase::Init()` 后面会执行 `sexychdir(mChangeDirTo)`
改变工作目录，在那之后再取 `GetCurrentDirectory` 拿到的就是被改过的目录了。
所以必须在 `Init()` 之前捕获。

### d. `main.cpp` — 合并重复的 `GetExeDirectory`

`main.cpp` 原本自己有一份文件级的 `static std::string GetExeDirectory()`，
与新增的 `Sexy::GetExeDirectory()` 在 `using namespace Sexy;` 下产生重载歧义：

```
main.cpp(55,23): error C2668: "GetExeDirectory": 对重载函数的调用不明确
```

已删掉 `main.cpp` 里那份（连带不再需要 `<shlwapi.h>`），统一用共享实现。

### e. `assets/savedata.ini` — 默认配置

带完整注释的模板，随构建复制到 exe 旁边。默认 `mode = user`。

## 验证

**7/7 实测通过。** 每个用例都是：清理候选目录 → 写配置 → **实际运行游戏** →
检查哪个目录出现了 `userdata\`。

> 之所以能用"目录是否出现"作为判据，是因为本次改动让游戏在**启动时就创建**
> 所选根目录和 `userdata\`。

| 用例 | `savedata.ini` | 期望目录 | 结果 |
|---|---|---|---|
| `user` | `mode = user` | `%APPDATA%\PopCap Games\PlantsVsZombies\userdata` | ✅ PASS |
| `portable` | `mode = portable` | `<bin>\userdata` | ✅ PASS |
| `instance` | `mode = instance`（以 `C:\...\pvzinst` 为工作目录启动） | `C:\...\pvzinst\userdata` | ✅ PASS |
| `static` | `mode = static` + `path = C:\...\pvzstatic` | `C:\...\pvzstatic\userdata` | ✅ PASS |
| `legacy` | `mode = legacy` | `%PROGRAMDATA%\PopCap Games\PlantsVsZombies\userdata` | ✅ PASS |
| 未知模式 | `mode = nonsense` | 回退到 user | ✅ PASS |
| 无配置文件 | （删除文件） | 回退到 user | ✅ PASS |

复现（以 portable 为例）：

```powershell
$bin = ".\build\ReleaseGOTY_x64\bin"
Remove-Item "$bin\userdata" -Recurse -Force -ErrorAction SilentlyContinue
Set-Content "$bin\savedata.ini" "mode = portable" -Encoding ASCII
Start-Process "$bin\PlantsVsZombies.exe" -WorkingDirectory $bin
Start-Sleep 6
Stop-Process -Name PlantsVsZombies -Force
Test-Path "$bin\userdata"        # 期望 True
```

ReleaseGOTY|x64 与 DebugGOTY|x64 重新构建均 **0 error / 0 C4819**。

## 回退

```powershell
git revert 772307c
```

回退后：

- 存档位置恢复为写死的 `%PROGRAMDATA%\PopCap Games\PlantsVsZombies\`
- 该目录**不再**在启动时自动创建
- `main.cpp` 的启动目录捕获、`GetExeDirectory` 合并一并撤销

> 注意：`assets/savedata.ini` 会被删除，但它对回退后的版本没有副作用
> （新版本不读它）。

## 相关

- 用户手册（五种模式的详细用法） → [04-存档配置](../04-存档配置.md)
- 存档系统整体（文件格式、注册表、启动选档流程） →
  [02-开发参考 §4 存档系统](../02-开发参考.md#4-存档系统)
- 这次改动的来由 —— 一次"每启动都是全新档"的排查 →
  [04 · 存档不保存问题的排查实录](2026-10-07-04-save-persistence-forensics.md)
