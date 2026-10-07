#ifndef __LAWNFEATURELABELS_H__
#define __LAWNFEATURELABELS_H__

// ====================================================================================================
// ★ Mod 功能界面的文字
//
// 为什么单独一个文件：
//   游戏内的字体是 main.pak 里的 8 位点阵字体（ImageFont），只能显示它 CharList 里列出的
//   那批 ASCII 字形，中文一律画不出来。所以这些标签目前只能是英文。
//
//   等将来接入“外挂字体”（Unicode TTF）之后，i18n 的全部工作量就是把下面这几组 _S("...")
//   换成 TodStringTranslate(_S("[XXX]")) 或直接写中文，界面代码一行都不用动。
//
//   所以：界面代码里不要内联写死任何显示文字，一律从这里取。
// ====================================================================================================

// 游戏设置 → “游戏”页
#define LAWN_LABEL_AUTO_COLLECT_SUN		_S("Auto Collect Sun")
#define LAWN_LABEL_AUTO_COLLECT_COINS	_S("Auto Collect Coins")

// 调试模式菜单
#define LAWN_LABEL_DEBUG_MENU			_S("DEBUG MENU")
#define LAWN_LABEL_DEBUG_HINT			_S("Press D to close")
#define LAWN_LABEL_SHOW_HEALTH			_S("Show Health Text")
#define LAWN_LABEL_INFINITE_SUN			_S("Infinite Sun")
#define LAWN_LABEL_NO_PLANT_COOLDOWN	_S("No Plant Cooldown")

// 按钮 / 页面
#define LAWN_LABEL_CLOSE				_S("CLOSE")
#define LAWN_LABEL_GAMEPLAY_PAGE		_S("Gameplay")

#endif // __LAWNFEATURELABELS_H__
