#include "DebugMenuDialog.h"
#include "../../LawnApp.h"
#include "../../ConstEnums.h"
#include "../../GameConstants.h"
#include "../../Resources.h"
#include "../../Sexy.TodLib/TodCommon.h"
#include "../../SexyAppFramework/Checkbox.h"
#include "../../SexyAppFramework/Font.h"
#include "../../SexyAppFramework/WidgetManager.h"
#include "../LawnCommon.h"
#include "LawnFeatureLabels.h"

DebugMenuDialog::DebugMenuDialog(LawnApp* theApp) :
	LawnDialog(theApp, Dialogs::DIALOG_DEBUGMENU, false, LAWN_LABEL_DEBUG_MENU, _S(""), _S(""), Dialog::BUTTONS_NONE)
{
	mApp = theApp;
	mVerticalCenterText = false;

	mShowHealth = MakeNewCheckbox(DebugMenuDialog::DebugMenuDialog_ShowHealth, this, false);
	mInfiniteSun = MakeNewCheckbox(DebugMenuDialog::DebugMenuDialog_InfiniteSun, this, false);
	mNoPlantCooldown = MakeNewCheckbox(DebugMenuDialog::DebugMenuDialog_NoPlantCooldown, this, false);
	SyncFromApp();

	CalcSize(60, 60);

	// 尺寸和位置全自己定。
	//
	// 位置**不用** CenterDialog：这个对话框在 640x480 这种小窗口下会被推到右边被裁掉
	// （实测标题只剩 "DEBUG MEN"）。贴左上角反而更好按，任何画布尺寸都完整可见。
	//
	// 高度按内容算（GetPreferredHeight）：CalcSize 给的是"对话框贴图的最小高度"，
	// 三行复选框会顶出下边框 —— 实测第三行 "No Plant Cooldown" 被下边框切掉一半。
	Resize(24, 48, GetPreferredWidth(), GetPreferredHeight(0));
}

DebugMenuDialog::~DebugMenuDialog()
{
	delete mShowHealth;
	delete mInfiniteSun;
	delete mNoPlantCooldown;
}

// 让复选框与 LawnApp 里真正生效的开关保持一致
void DebugMenuDialog::SyncFromApp()
{
#ifdef _HAS_FEATURE_MENU
	mShowHealth->SetChecked(mApp->mFeatures.mShowHealthText, false);
	mInfiniteSun->SetChecked(mApp->mFeatures.mInfiniteSun, false);
	mNoPlantCooldown->SetChecked(mApp->mFeatures.mNoPlantCooldown, false);
#endif
}

void DebugMenuDialog::AddedToManager(WidgetManager* theWidgetManager)
{
	LawnDialog::AddedToManager(theWidgetManager);
	AddWidget(mShowHealth);
	AddWidget(mInfiniteSun);
	AddWidget(mNoPlantCooldown);
}

void DebugMenuDialog::RemovedFromManager(WidgetManager* theWidgetManager)
{
	LawnDialog::RemovedFromManager(theWidgetManager);
	RemoveWidget(mShowHealth);
	RemoveWidget(mInfiniteSun);
	RemoveWidget(mNoPlantCooldown);
}

// ====================================================================================================
// 尺寸与布局
//
// 背景：LawnDialog 的九宫格背景 + Dialog 的内容边距是两层叠加的，
// 自己拿 mContentInsets / mBackgroundInsets 逐行硬算很容易错位（第一版就是这么错的）。
//
// 现在：
//   · 宽度 = 最长标签的实测宽度 + 左边距 + 复选框宽 + 文字前间距 + 右边留白；
//   · 三行用固定行距 44（复选框 45 高，重叠 1px）从正文区顶部往下排；
//   · 文字横向只跟对话框左边距有关，不跟复选框走，改宽度不会再错位。
// ====================================================================================================
static const int DEBUG_MENU_ROW_HEIGHT = 42;      // 复选框 45 高，行距 42 让它们略微叠一点，视觉是连续一列
static const int DEBUG_MENU_FIRST_ROW = 14;
static const int DEBUG_MENU_CHECKBOX_SIZE = 46;
static const int DEBUG_MENU_CHECKBOX_HEIGHT = 42;
static const int DEBUG_MENU_HINT_GAP = 14;
static const int DEBUG_MENU_LEFT_PAD = 14;
static const int DEBUG_MENU_BOTTOM_PAD = 10;

int DebugMenuDialog::GetPreferredWidth()
{
	const int aLabelWidth = max(
		FONT_DWARVENTODCRAFT18->StringWidth(LAWN_LABEL_SHOW_HEALTH),
		max(FONT_DWARVENTODCRAFT18->StringWidth(LAWN_LABEL_INFINITE_SUN),
			FONT_DWARVENTODCRAFT18->StringWidth(LAWN_LABEL_NO_PLANT_COOLDOWN)));

	// 左边距 + 复选框 + 文字前间距 + 标签 + 右边留白
	return mBackgroundInsets.mLeft + mContentInsets.mLeft + 14
		+ DEBUG_MENU_CHECKBOX_SIZE + 8
		+ aLabelWidth + 24;
}

// 高度 = 正文区顶部 + 三行 + 底部提示 + 下边距。
// 必须自己算：CalcSize / LawnDialog::GetPreferredHeight 给的是对话框贴图的最小高度，
// 按那个高度三行复选框会顶出下边框（实测第三行标签被切掉一半）。
int DebugMenuDialog::GetPreferredHeight(int theWidth)
{
	const int aFirstRowY = GetContentStartY() + DEBUG_MENU_FIRST_ROW;
	const int aLastRowBottom = aFirstRowY + DEBUG_MENU_ROW_HEIGHT * 2 + DEBUG_MENU_CHECKBOX_HEIGHT;
	const int aHintBottom = aLastRowBottom + DEBUG_MENU_HINT_GAP + FONT_DWARVENTODCRAFT12->GetHeight();

	return aHintBottom + mContentInsets.mBottom + mBackgroundInsets.mBottom + DEBUG_MENU_BOTTOM_PAD;
}

void DebugMenuDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
	LawnDialog::Resize(theX, theY, theWidth, theHeight);

	if (mShowHealth == nullptr)
		return;

	const int aStartY = GetContentStartY();
	const int startX = mBackgroundInsets.mLeft + mContentInsets.mLeft + DEBUG_MENU_LEFT_PAD;

	mShowHealth->Resize(startX, aStartY + DEBUG_MENU_FIRST_ROW, DEBUG_MENU_CHECKBOX_SIZE, DEBUG_MENU_CHECKBOX_HEIGHT);
	mInfiniteSun->Resize(startX, mShowHealth->mY + DEBUG_MENU_ROW_HEIGHT, DEBUG_MENU_CHECKBOX_SIZE, DEBUG_MENU_CHECKBOX_HEIGHT);
	mNoPlantCooldown->Resize(startX, mInfiniteSun->mY + DEBUG_MENU_ROW_HEIGHT, DEBUG_MENU_CHECKBOX_SIZE, DEBUG_MENU_CHECKBOX_HEIGHT);
}

// 正文区顶部的 y（和 LawnDialog::Draw / MoreSettingsDialog::Draw 用的是同一套算法）
int DebugMenuDialog::GetContentStartY()
{
	int aStartY = mContentInsets.mTop + mBackgroundInsets.mTop + DIALOG_HEADER_OFFSET;
	if (mDialogHeader.size() > 0)
	{
		const int aOffsetY = aStartY - mHeaderFont->GetAscentPadding() + mHeaderFont->GetAscent();
		aStartY = aOffsetY - mHeaderFont->GetAscent() + mHeaderFont->GetHeight() + mSpaceAfterHeader;
	}
	return aStartY;
}

void DebugMenuDialog::Draw(Graphics* g)
{
	LawnDialog::Draw(g);

	if (mShowHealth == nullptr)
		return;

	const Color fontColor(107, 109, 145);

	// 文字横向只跟对话框左边距有关，不跟复选框宽度走，改宽度不会再错位
	const int aTextX = mBackgroundInsets.mLeft + mContentInsets.mLeft + 14 + DEBUG_MENU_CHECKBOX_SIZE + 8;

	TodDrawString(g, LAWN_LABEL_SHOW_HEALTH, aTextX, mShowHealth->mY + 24, FONT_DWARVENTODCRAFT18, fontColor, DS_ALIGN_LEFT);
	TodDrawString(g, LAWN_LABEL_INFINITE_SUN, aTextX, mInfiniteSun->mY + 24, FONT_DWARVENTODCRAFT18, fontColor, DS_ALIGN_LEFT);
	TodDrawString(g, LAWN_LABEL_NO_PLANT_COOLDOWN, aTextX, mNoPlantCooldown->mY + 24, FONT_DWARVENTODCRAFT18, fontColor, DS_ALIGN_LEFT);

	// 底部提示，水平居中在对话框里
	const int aHintCenterX = mWidth / 2;
	const int aHintY = mNoPlantCooldown->mY + 45 + 16;
	TodDrawString(g, LAWN_LABEL_DEBUG_HINT, aHintCenterX, aHintY, FONT_DWARVENTODCRAFT12, Color(140, 140, 170), DS_ALIGN_CENTER);
}

void DebugMenuDialog::CheckboxChecked(int theId, bool checked)
{
#ifdef _HAS_FEATURE_MENU
	switch (theId)
	{
	case DebugMenuDialog::DebugMenuDialog_ShowHealth:
		mApp->mFeatures.mShowHealthText = checked;
		break;

	case DebugMenuDialog::DebugMenuDialog_InfiniteSun:
		mApp->mFeatures.mInfiniteSun = checked;
		break;

	case DebugMenuDialog::DebugMenuDialog_NoPlantCooldown:
		mApp->mFeatures.mNoPlantCooldown = checked;
		break;
	}
#endif

	mApp->PlaySample(SOUND_BUTTONCLICK);
}

void DebugMenuDialog::KeyDown(KeyCode theKey)
{
	if (theKey == KEYCODE_D)
	{
		mApp->KillDebugMenu();
		return;
	}

	if (theKey == KeyCode::KEYCODE_ESCAPE)
	{
		mApp->KillDebugMenu();
		return;
	}

	LawnDialog::KeyDown(theKey);
}
