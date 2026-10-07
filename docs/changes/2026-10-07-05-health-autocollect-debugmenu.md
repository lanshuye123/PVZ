# 变更记录 · 血量数字显示 + 自动拾取 + 调试模式菜单

| 项 | 值 |
|---|---|
| 日期 | 2026-10-07 |
| 提交 | `efa9c14` |
| 类型 | 新功能 |
| 影响 | 游戏逻辑（渲染 / 掉落物 / 卡槽）、游戏设置界面、调试界面 |
| 涉及文件 | `GameConstants.h`、`LawnApp.h`、`LawnApp.cpp`、`ConstEnums.h`、`SexyAppFramework/KeyCodes.h`、`SexyAppFramework/SexyAppBase.vcxproj`、`Lawn/Board.h`、`Lawn/Board.cpp`、`Lawn/Plant.cpp`、`Lawn/Zombie.cpp`、`Sexy.TodLib/TodCommon.h`、`Sexy.TodLib/TodCommon.cpp`、`Sexy.TodLib/TodDebug.cpp`、`Lawn/Widget/NewOptionsDialog.cpp`、`Lawn/Widget/MoreSettingsDialog.h`、`Lawn/Widget/MoreSettingsDialog.cpp`、`Lawn/Widget/DebugMenuDialog.h`、`Lawn/Widget/DebugMenuDialog.cpp`、`Lawn/Widget/LawnFeatureLabels.h` |

## 背景与症状

需要给这个反编译工程补三块玩家向功能：

1. **血量显示**：改成数字，植物用绿色（如 `300/300`），僵尸用红色；
   有防具的僵尸要**分开显示本体血量和防具血量**。
   工程里原有的 `_HAS_HEALTHBAR_TOGGLE`（TAB 键）画的是两条 50×5 的彩条，
   没有具体数值，也分不出"哪条是本体、哪条是防具"。
2. **自动拾取**：阳光自动拾取、金币自动拾取，两个**独立**开关，放在设置里。
3. **调试模式菜单**：按 D 键打开，里面放血量显示、无限阳光、无种植 CD 三个开关。

## 根因 / 设计要点

### a. 血量条与血量数字的取舍

原实现（`Zombie.cpp` 约 7342 行、`Plant.cpp` 约 5187 行）画的是矩形条：

```cpp
g->SetColor(Color(255, 75, 75));            // 底：红
g->FillRect(HEALTH_POSX, HEALTH_POSY, 50, 5);
g->SetColor(Color(255, 255, 75));           // 前：黄
float HPpercent = (mBodyHealth + mHelmHealth - HP_OFFSET) / (float)(...);
g->FillRect(HEALTH_POSX, HEALTH_POSY, (int)(50 * HPpercent), 5);
```

`mBodyHealth` / `mHelmHealth` 之类的字段一直都在（`Zombie.h` 139–147 行），
缺的只是"把它们画成数字"。因此本次**只换绘制方式，不动战斗数值**。

### b. 僵尸"本体 / 防具"怎么分

代码里的三个血量字段语义不同，不能简单相加：

| 字段 | 含义 | 例 |
|---|---|---|
| `mBodyHealth` / `mBodyMaxHealth` | 本体 | 普通僵尸 270 |
| `mHelmHealth` / `mHelmMaxHealth` | 头盔类防具 | 路障、铁桶、报纸、足球头盔 |
| `mFlyingHealth` / `mFlyingMaxHealth` | 飞行防具 | 气球 |

所以按"有没有 `mHelmMaxHealth`"决定是否画两行：
防具那行在上、本体那行在下，颜色统一红色（`255, 40, 40`）。

> 注意原血条把 `mBodyHealth + mHelmHealth` 合成一条，并把 `HP_OFFSET = mBodyMaxHealth / 3`
> 算进百分比（对应"僵尸掉头之前还能打掉三分之一血量"的机制）。
> 数字显示**不套用这个 offset** —— 玩家看到的就是实实在在的剩余血量。

### c. 自动拾取为什么复用 `Coin::Collect()`

手动拾取的入口是 `Coin::MouseDown()`：`PlayCollectSound()` + `Collect()`。
`Collect()` 内部自己处理阳光进账、金币进账、成就、粒子、音效，
所以自动拾取只要走同一个入口，行为就与手点**逐字节一致**，
不需要再碰 `mSunMoney` / `mPlayerInfo->AddCoins`。

收集时机选"任何位置"而不是"落地之后"：阳光从天上掉下来时就在下落路径上收走，
不会出现"快落地时忽然消失"的观感。`COIN_USABLE_SEED_PACKET`（需要玩家点选落点）
按类型天然被排除在 `IsSun()` / `IsMoney()` 之外。

### d. 无限阳光 / 无种植 CD 为什么是"每帧拉满"

与其去改 `Board::TakeSunMoney()` / `GetCurrentPlantCost()` 的判定，
不如每帧把状态恢复到满：

```cpp
if (mApp->mFeatures.mInfiniteSun)   mSunMoney = 9990;   // AddSunMoney 原本的上限
if (mApp->mFeatures.mNoPlantCooldown) { /* 卡槽计数清零 + mActive = true */ }
```

好处是**松手即恢复**，不需要写任何"关闭时回滚"的代码，也不会永久破坏存档/关卡状态。
`9990` 是 `Board::AddSunMoney()` 里本来就用的上限，不是新魔数。

### e. 设置入口：启用原本被注释掉的"More Settings"

`NewOptionsDialog` 里有一组按钮是注释状态：

```cpp
/* mGameplayButton = MakeButton(..., _S("[GAMEPLAY_SETTINGS_BUTTON]")); ... */
```

而 `MoreSettingsDialog`（600×450，两页）第 2 页是空的，`LawnApp::DoMoreSettingsDialog()`
和 `DIALOG_MORESETTINGS` 都已存在但无人调用。本次把这条链路接通：

```
选项（ESC/空格）→ [More Settings] 按钮 → MoreSettingsDialog 第 2 页 = 游戏设置
```

按钮放在图鉴上方 y=198（图鉴在 241，两者不重叠），改完实测不遮挡任何现有控件。

### f. 调试菜单为什么不用 ImGui

工程虽然链了 Dear ImGui，但只在粒子编辑器里用，而且 `LawnApp::DrawDirtyStuff()`
**每帧都调用 `ParticleScreen::ImGuiDraw()`** —— 在 Debug 配置下 `ImGui::NewFrame()`
撞上 `IM_ASSERT(g.WithinFrameScope == false)` 会直接断言失败。

而调试菜单需要在**主游戏里**能开，不能只跟粒子编辑器绑定。
所以改用工程自己的 `LawnDialog` + `Checkbox`，零新增依赖，输入/绘制全部走既有管线。

### g. 两处开关为什么各自独立

设置里的"自动拾取"和调试菜单里的三个作弊**不共用变量**，而是两组：

| | 存放 | 语义 |
|---|---|---|
| 玩家设置 | `mFeatures.mAutoCollectSun` / `mAutoCollectCoins` | 改一次一直有效 |
| 调试作弊 | `mFeatures.mShowHealthText` / `mInfiniteSun` / `mNoPlantCooldown` | 允许一局一开 |

这样设置里关掉"无限阳光"之后，调试菜单里临时开的无限阳光不会被一起关掉，反之亦然。

## 改动

### 1. 功能总开关（`GameConstants.h`）

```cpp
#define _HAS_FEATURE_MENU
```

注释掉它就等于整组功能不参与编译，不留运行时开销。

### 2. 运行时开关（`LawnApp.h`）

```cpp
struct LawnFeatureSettings
{
    bool mAutoCollectSun = false;   // 玩家设置
    bool mAutoCollectCoins = false;
    bool mShowHealthText = false;   // 调试作弊
    bool mInfiniteSun = false;
    bool mNoPlantCooldown = false;
};
```

### 3. 血量数字（`Plant.cpp` / `Zombie.cpp` / `TodCommon.cpp`）

新增 `TodDrawHealthText()`，坐标按**对象局部坐标**给（函数内部补 `Graphics` 的平移量）。
两个对象类都只用这一个函数，方便将来换字体时**只改一处**。

植物（绿色，`Color(0, 255, 0)`）：

```cpp
TodDrawHealthText(g, StrFormat(_S("%d/%d"), mPlantHealth, mPlantMaxHealth),
    12 + 25, HEALTH_Y - 2, mBoard->mDebugFont, Color(0, 255, 0));
```

僵尸（红色，`Color(255, 40, 40)`），有防具时两行：

```cpp
if (mHelmMaxHealth > 0 || mHelmHealth > 0) {
    TodDrawHealthText(g, StrFormat(_S("%d/%d"), mHelmHealth, mHelmMaxHealth), ...);
    TodDrawHealthText(g, StrFormat(_S("%d/%d"), mBodyHealth, mBodyMaxHealth), ...);
} else {
    TodDrawHealthText(g, StrFormat(_S("%d/%d"), mBodyHealth, mBodyMaxHealth), ...);
}
```

原有的血条分支**完整保留**在 `else` 里，`mShowHealthBar` 仍然可用（TAB 键切换）。

### 4. 自动拾取 + 作弊（`Board.cpp`）

新增 `Board::UpdateAutoCollect()` 与 `Board::UpdateFeatureCheats()`，
在 `Board::UpdateGame()` 里、原版 `mMainCounter++` 之前调用：

```cpp
UpdateFeatureCheats();                                        // 无限阳光 / 无 CD
if (mApp->mGameScene == GameScenes::SCENE_PLAYING)
    UpdateAutoCollect();                                      // 只在真正对局里跑
```

### 5. 调试菜单（新增 `Lawn/Widget/DebugMenuDialog.{h,cpp}`）

- D 键在 `LawnApp::UpdateAppStep()` 的 `SDL_EVENT_KEY_DOWN` 里拦下。
  **不**走 widget 的 `KeyDown`：这个对话框不是模态的，没有焦点 widget，
  键盘事件到不了它（`WidgetManager::KeyDown` 只转发给 `mFocusWidget`）。
- 只在没按 Ctrl/Alt/Win 时触发，避免和已有的 Ctrl+Alt+D 抢。
- 关闭走 `LawnApp::KillDebugMenu()`，并且在 `DoBackToMain()` 里也收一次，
  防止回主菜单后菜单横在主界面上挡鼠标。

### 6. 文字集中管理（新增 `Lawn/Widget/LawnFeatureLabels.h`）

游戏内字体是 `main.pak` 里的 8 位点阵 `ImageFont`，`CharList` 只定义了 ASCII，
画不出中文，因此标签目前是英文。但**所有显示文字都集中在 `LawnFeatureLabels.h`**：

```cpp
#define LAWN_LABEL_SHOW_HEALTH   _S("Show Health Text")
```

以后接入外挂 Unicode 字体做 i18n 时，只需要把这一组换成
`TodStringTranslate(_S("[XXX]"))`，界面代码一行都不用动。

### 7. 其它

- `ConstEnums.h`：新增 `DIALOG_DEBUGMENU`。
- `KeyCodes.h`：新增 `constexpr KeyCode KEYCODE_D = (KeyCode)'D';`，避免散落魔法值。
- `MoreSettingsDialog`：新增"游戏"页两个复选框，并在构造/显示时 `SyncSettingsFromApp()`。
- `SexyAppBase.vcxproj`：登记新文件。

## 验证

编译（8 个配置里抽两个，覆盖 `_DEBUG` 与 `NDEBUG` 两条分支）：

```powershell
$msbuild = "D:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe"
& $msbuild PlantsVsZombies.sln /t:Rebuild /p:Configuration=DebugGOTY   /p:Platform=x64 /m /nologo /nodeReuse:false > b1.log 2>&1
& $msbuild PlantsVsZombies.sln /p:Configuration=ReleaseGOTY /p:Platform=x64 /m /nologo /nodeReuse:false > b2.log 2>&1
$enc = [Text.Encoding]::GetEncoding(936)
foreach ($f in 'b1.log','b2.log') {
    $t = [IO.File]::ReadAllText($f, $enc)
    "$f  errors = " + ([regex]::Matches($t, ': error ')).Count
}
# 期望：两个都是 0
```

实测：**DebugGOTY|x64（全量 Rebuild）与 ReleaseGOTY|x64 均为 errors = 0**。

调试菜单实测（截屏逐步核对）：按一次 D 弹出、标题完整、三行复选框与文字对齐；
再按一次 D 整块消失，前后两帧像素完全一致。

## 第一版的两个 bug（已修）

第一版交付后实测暴露了问题，记在这里避免以后重犯。

### bug 1：血量数字完全看不到

**根因**：`Graphics::DrawString()` 和 `Graphics::FillRect()` 对平移的处理**不一致**。

- `FillRect` / `DrawRect` / `DrawImage` 都会把 `g->mTransX / mTransY` 加进目标坐标；
- `Graphics::DrawString()`（`Graphics.cpp:683`）拿到坐标后**原样**交给 `Font::DrawString()`，
  **完全不理会 mTransX/mTransY**。

而血量条是在 `GameObject::BeginDraw()` 已经 `Translate(mX, mY)` 之后的坐标系里画的
（`GameObject.cpp:22`），所以那条 `FillRect(12, HEALTH_Y, 50, 5)` 的 12 是**对象局部坐标**。
第一版把同样的局部坐标交给 `TodDrawString`，字就被画到屏幕左上角（甚至画到板外）去了。

**修法**：`TodDrawHealthText()` 里显式把平移量补回去，改用 `g->SetFont() + g->DrawString()`：

```cpp
const int aDrawX = thePosX + g->mTransX;
const int aDrawY = thePosY + g->mTransY;
g->DrawString(aFinalString, aDrawX, aDrawY - theFont->GetDescent());
```

这样文字和 `FillRect` 画出来的血量条处在同一个坐标系里。

### bug 2：调试菜单错位、且关不掉

三个原因是叠在一起的：

1. **行距硬算错位** —— 第一版照抄 `MoreSettingsDialog` 里 `aStartY - 12` 那套写法，
   那套只对"首行"成立，往下叠加每加一行都要重新对一次。改成固定行距 42 从正文区顶部往下排。
2. **宽度拍数字** —— 拍了个 470，游戏窗口只有 640x480（比 800x600 小），右边被裁掉。
   改成宽度 = 最长标签实测宽度 + 各段边距（`GetPreferredWidth()`）。
3. **高度没有为内容让位** —— 高度用的是 `CalcSize` 的结果（对话框贴图的**最小高度**），
   三行 + 底部提示放不下，第三行 "No Plant Cooldown" 被下边框切掉一半。
   改成自己算（`GetPreferredHeight()`）：正文区顶 + 三行 + 提示 + 下边距。

**位置**也**不再用 `CenterDialog`** —— 它会把对话框推到右边被裁掉。直接贴左上角 `(24, 48)`，
任何画布尺寸都完整可见。

修完实测：标题 "DEBUG MENU" 完整、三行复选框与文字对齐、底部 "Press D to close" 在内；
按 D 开关，开→关→开 的像素签名与第一次开完全一致（可重复）。

### bug 3：More Settings 看不到翻页按钮、也进不了第二页

**根因是个很隐蔽的坑**：`MakeButton()` 只设了 `mHeight = 33`，
而 `LawnStoneButton::SetLabel()`（`GameButton.cpp:277`）**也不设宽度** ——
于是 `mPage1` / `mPage2` 的 `mWidth` 一直是 **0**。

而 `MoreSettingsDialog::Resize` 又是照抄原版写的：

```cpp
mPage1->Resize(40, ..., mPage1->mWidth, 46);          // ← 宽度就是 0
mPage2->Resize(..., mPage2->mWidth, 46);              // ← 也是 0
```

0 宽的控件既画不出来、也命不中 → "看不到翻页按钮，怎么点都进不去第二页"。

**修法**（`MoreSettingsDialog`）：

1. **显式给翻页按钮宽度**（94×36），横向居中排在最后一排控件下方、
   底部 CLOSE 按钮之上；
2. 翻页改成**绝对**选页：按钮 1 → 第 1 页，按钮 2 → 第 2 页
   （原来是"上一页/下一页"的相对翻页）；
3. `ChangePage` 对越界入参做保护，并且**同页也重排一次**，
   避免从第一页切过来时第二页那两个复选框还停在 `(0,0)`；
4. 键盘兜底：对话框自己的 `KeyDown` 收 **1 / 2 / 左右方向键**，
   `LawnApp::UpdateAppStep` 在 `DIALOG_MORESETTINGS` 打开时**再兜一层** ——
   因为对话框不一定拿得到键盘焦点，不能只依赖 `KeyDown`。

### bug 4：More Settings 在菜单里的位置、以及 Close 按钮

- **位置**：要求 "More Settings 压在 Credits 上方"。原版按钮堆栈是从 y=241 起的 3 行
  （241 / 284 / 327），底部 OK 在 381。
  改成主界面那套把堆栈整体上移一行到 **198** 起，"更多设置"落在堆栈**最后一行**，
  于是它正好在 Credits 正下方、不与任何按钮重叠；底部 OK 跟着下移到 391。
  游戏内那套没有 Credits，堆栈仍从 241 起铺 4 行。
- **Close 按钮错位**：按钮行距统一成常量 `BUTTON_STEP = 43`，
  底部区域不再被挤，Close 的落点回到 `LawnDialog` 原本算出来的位置。

## 顺带修掉的一个上游 bug（会直接崩）

排查过程中发现：**任何 `fopen("log.txt")` 失败的 Debug 构建都会在启动瞬间崩溃**。

`TodDebug.cpp` 的 `TodLogString()` 原本是这样：

```cpp
FILE* f = fopen(gLogFileName, "a");
if (f == nullptr) { OutputDebugString(...); printf(...); }   // ← 只提示，没有 return
if (fwrite(theMsg, strlen(theMsg), 1, f) != 1) { ... }        // ← f == nullptr，直接崩
```

实测报错：

```
Debug Assertion Failed!
File: minkernel\crts\ucrt\src\appcrt\stdio\fwrite.cpp
Line: 35
Expression: stream != nullptr
```

**两处改动**：

1. `TodLogString()` 在 `f == nullptr` 时 `return`（真正的 bug）；
2. `TodAssertInitForApp()` 不再用 `GetAppDataFolder()` 拼绝对路径，日志改为
   **程序工作目录下的相对路径 `log.txt`** —— 不写死任何盘符，存档无论配成
   `user` 还是 `portable`，日志都跟 exe 在一起，方便现场取。

> 这个 bug 与本批三个功能无关，是独立的启动崩溃；但因为会掩盖一切后续验证，顺手修了。

## 游戏内手工观察点

| 步骤 | 期望 |
|---|---|
| 主界面 → [OPTIONS] | 按钮自上而下：More Settings → Credits → OK，互不重叠 |
| 点 [More Settings] | 打开设置，标题右侧 `1/2`，下方有两个翻页按钮 `1` `2` |
| 点 "2"，或按键盘 **2** / **→** | 切到第 2 页，出现 Auto Collect Sun / Auto Collect Coins |
| 点 "1"，或按键盘 **1** / **←** | 切回第 1 页 |
| 勾上 Auto Collect Sun | 天上掉下来的阳光自动进账，不需要点 |
| 勾上 Auto Collect Coins | 僵尸/砖块掉的金币自动进账 |
| 游戏中按 D | 弹出 DEBUG MENU，**完整显示不被裁**，三行对齐，底部有 "Press D to close" |
| 勾 Show Health Text | **满血也显示**：植物头顶绿色 `当前/最大`；僵尸头顶红色数字，路障/铁桶僵尸多一行防具血量 |
| 勾 Infinite Sun | 阳光数字立刻变 9990 并锁住 |
| 勾 No Plant Cooldown | 所有卡槽立刻变成可用；关掉后按原 `mRefreshTime` 重新计时 |
| 再按 D | 菜单整块消失 |

> **沙箱环境注意**：如果是在 DSH 这类受限会话里构建，`build/*/bin/PlantsVsZombies.exe`
> 可能是**上一次普通权限构建**留下的 Medium 完整性标签文件，而受限会话是 Low 完整性，
> 链接器会报 `LNK1168: 无法打开 ... 进行写入`。这是**环境/权限问题，不是代码问题**：
> 删掉那个 exe 再链接即可。

## 回退

`git revert <hash>`。

手工回退要点：

1. 注释掉 `GameConstants.h` 的 `#define _HAS_FEATURE_MENU` —— 这一步就能让全部新逻辑
   退出编译（所有改动都在 `#ifdef _HAS_FEATURE_MENU` 里），界面与旧行为完全恢复；
2. 若要去掉界面入口，把 `NewOptionsDialog.cpp` 里的 `mGameplayButton` 那几段
   重新注释掉；
3. 若要连文件一起删：`Lawn/Widget/DebugMenuDialog.{h,cpp}`、`Lawn/Widget/LawnFeatureLabels.h`，
   并从 `SexyAppBase.vcxproj` 移除对应条目。

回退后果：血量回到"只有彩条、没有数字"，自动拾取与调试菜单消失，其余功能不受影响。

## 相关

- 功能开关总表 → [02-开发参考 §5 功能开关](../02-开发参考.md#5-功能开关)
- 源文件编码约定（新增文件必须 UTF-8） → [02-开发参考 §6](../02-开发参考.md#6-源文件编码约定)
- 上游 `MoreSettingsDialog` 一直是**未接通**状态，本次首次启用其第 2 页
