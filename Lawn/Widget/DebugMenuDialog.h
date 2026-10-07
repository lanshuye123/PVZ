#ifndef __DEBUGMENUDIALOG_H__
#define __DEBUGMENUDIALOG_H__

#include "../../SexyAppFramework/CheckboxListener.h"
#include "LawnDialog.h"

class LawnApp;
namespace Sexy
{
	class Checkbox;
};

// ====================================================================================================
// ★ 调试模式菜单
//
//   · 游戏中按 D 键打开 / 关闭（LawnApp::ToggleDebugMenu）；
//   · 用 WidgetManager 的焦点键盘事件收 D 键，所以必须先点一下窗口；
//   · 三个开关直接写 LawnApp::mFeatures，和游戏设置页里那两个互不覆盖：
//     设置页管 mAutoCollectSun / mAutoCollectCoins，这里管血量、无限阳光、无 CD。
// ====================================================================================================
class DebugMenuDialog : public LawnDialog, public Sexy::CheckboxListener
{
private:
	enum
	{
		DebugMenuDialog_ShowHealth = 20100,
		DebugMenuDialog_InfiniteSun,
		DebugMenuDialog_NoPlantCooldown,
	};

public:
	LawnApp*			mApp;
	Sexy::Checkbox*		mShowHealth;
	Sexy::Checkbox*		mInfiniteSun;
	Sexy::Checkbox*		mNoPlantCooldown;

public:
	DebugMenuDialog(LawnApp* theApp);
	virtual ~DebugMenuDialog();

	virtual void		AddedToManager(Sexy::WidgetManager* theWidgetManager);
	virtual void		RemovedFromManager(Sexy::WidgetManager* theWidgetManager);
	virtual void		Resize(int theX, int theY, int theWidth, int theHeight);
	virtual int			GetPreferredHeight(int theWidth);
	virtual void		Draw(Sexy::Graphics* g);
	virtual void		CheckboxChecked(int theId, bool checked);
	virtual void		KeyDown(Sexy::KeyCode theKey);

	int					GetContentStartY();
	int					GetPreferredWidth();
	void				SyncFromApp();
};

#endif // __DEBUGMENUDIALOG_H__
