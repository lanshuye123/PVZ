#include "MoreSettingsDialog.h"
#include "../../LawnApp.h"
#include "../../ConstEnums.h"
#include "../../GameConstants.h"
#include "../../SexyAppFramework/Checkbox.h"
#include "../../SexyAppFramework/Font.h"
#include "../../Resources.h"
#include "GameButton.h"
#include "LawnFeatureLabels.h"
#include "../../SexyAppFramework/DDInterface.h"
#include "../System/Music.h"

MoreSettingsDialog::MoreSettingsDialog(LawnApp* theApp) :
	LawnDialog(theApp, Dialogs::DIALOG_MORESETTINGS, true, _S("MORE SETTINGS"), "", _S("CLOSE"), Dialog::BUTTONS_FOOTER)
{
	mApp = theApp;
	mPage1 = MakeButton(MoreSettingsDialog::MoreSettingsDialog_Page1, this, _S("1"));
	mPage2 = MakeButton(MoreSettingsDialog::MoreSettingsDialog_Page2, this, _S("2"));

	// PP1
	mHardwareAcceleration = MakeNewCheckbox(MoreSettingsDialog::MoreSettingsDialog_HardwareAcceleration, this, mApp->Is3DAccelerated());
	mCustomCursor = MakeNewCheckbox(MoreSettingsDialog::MoreSettingsDialog_CustomCursor, this, false); // !mApp->mWindowCursor
	mFPSToggle = MakeNewCheckbox(MoreSettingsDialog::MoreSettingsDialog_FPS, this, false); // mApp->mFPSToggled
	mAutoPause = MakeNewCheckbox(MoreSettingsDialog::MoreSettingsDialog_AutoPause, this, false); //  !mApp->mNoAutoPause
	mShowToolTip = MakeNewCheckbox(MoreSettingsDialog::MoreSettingsDialog_NoToolTip, this, false); //!mApp->mNoTooltip

	// PP2 / 游戏设置
	mAutoCollectSun = MakeNewCheckbox(MoreSettingsDialog::MoreSettingsDialog_AutoCollectSun, this, false);
	mAutoCollectCoins = MakeNewCheckbox(MoreSettingsDialog::MoreSettingsDialog_AutoCollectCoins, this, false);
	SyncSettingsFromApp();

	ChangePage(MoreSettingsDialog::MoreSettingsPage_1);

	Resize(0, 0, 600, 450);
	LawnApp::CenterDialog(this, mWidth, mHeight);
}

// 让复选框与 LawnApp 里真正生效的开关保持一致
void MoreSettingsDialog::SyncSettingsFromApp()
{
#ifdef _HAS_FEATURE_MENU
	mAutoCollectSun->SetChecked(mApp->mFeatures.mAutoCollectSun, false);
	mAutoCollectCoins->SetChecked(mApp->mFeatures.mAutoCollectCoins, false);
#endif
}

MoreSettingsDialog::~MoreSettingsDialog() 
{
	delete mPage1;
	delete mPage2;

	delete mHardwareAcceleration;
	delete mCustomCursor;
	delete mFPSToggle;
	delete mAutoPause;
	delete mShowToolTip;
	delete mAutoCollectSun;
	delete mAutoCollectCoins;
}

void MoreSettingsDialog::AddedToManager(Sexy::WidgetManager* theWidgetManager) 
{
	LawnDialog::AddedToManager(theWidgetManager);
	AddWidget(mPage1);
	AddWidget(mPage2);

	AddWidget(mHardwareAcceleration);
	AddWidget(mCustomCursor);
	AddWidget(mFPSToggle);
	AddWidget(mAutoPause);
	AddWidget(mShowToolTip);
	AddWidget(mAutoCollectSun);
	AddWidget(mAutoCollectCoins);
}

void MoreSettingsDialog::RemovedFromManager(Sexy::WidgetManager* theWidgetManager)
{
	LawnDialog::RemovedFromManager(theWidgetManager);
	RemoveWidget(mPage1);
	RemoveWidget(mPage2);

	RemoveWidget(mHardwareAcceleration);
	RemoveWidget(mCustomCursor);
	RemoveWidget(mFPSToggle);
	RemoveWidget(mAutoPause);
	RemoveWidget(mShowToolTip);
	RemoveWidget(mAutoCollectSun);
	RemoveWidget(mAutoCollectCoins);
}

void MoreSettingsDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
	LawnDialog::Resize(theX, theY, theWidth, theHeight);

	int aStartY = mContentInsets.mTop + mBackgroundInsets.mTop + DIALOG_HEADER_OFFSET + 10;
	if (mDialogHeader.size() > 0)
	{
		int aOffsetY = aStartY - mHeaderFont->GetAscentPadding() + mHeaderFont->GetAscent();
		aStartY = aOffsetY - mHeaderFont->GetAscent() + mHeaderFont->GetHeight() + mSpaceAfterHeader;
	}

	int startX = mBackgroundInsets.mLeft + mContentInsets.mLeft + 2;
	int offsetY = 0;
	int aWidth = mWidth - mBackgroundInsets.mLeft - mContentInsets.mLeft - IMAGE_DIALOG_BOTTOMRIGHT->GetWidth() / 2 + 4;

	if (mCurPage == MoreSettingsPage_1)
	{
		mHardwareAcceleration->Resize(startX, aStartY + offsetY - 12, 46, 45);
		mCustomCursor->Resize(startX, mHardwareAcceleration->mY + mHardwareAcceleration->mHeight / 1.75f + 10, 46, 45);
		mFPSToggle->Resize(startX, mCustomCursor->mY + mCustomCursor->mHeight / 1.75f + 10, 46, 45);

		startX += aWidth / 2;

		mAutoPause->Resize(startX, aStartY - 12, 46, 45);
		mShowToolTip->Resize(startX, mAutoPause->mY + mAutoPause->mHeight / 1.75f + 10, 46, 45);
	}
	else if (mCurPage == MoreSettingsPage_2)
	{
		// 游戏设置页：自动拾取阳光 / 金币
		startX += aWidth / 2;

		mAutoCollectSun->Resize(startX, aStartY - 12, 46, 45);
		mAutoCollectCoins->Resize(startX, mAutoCollectSun->mY + mAutoCollectSun->mHeight / 1.75f + 10, 46, 45);
	}

	// ================================================================================================
	// 翻页按钮
	//
	// ⚠ 这里原来用的是 mPage1->mWidth —— 但 MakeButton() 只设了 mHeight = 33，
	// 而 LawnStoneButton::SetLabel() 也**不**设宽度（GameButton.cpp:277）。
	// 结果两个按钮的 mWidth 一直是 0：既画不出来、也点不到，
	// 表现就是"看不到翻页按钮，而且怎么点都进不了第二页"。
	// 所以这里必须显式给宽度。
	//
	// 纵向位置：放在最后一排控件下方，同时离底部 CLOSE 按钮留出余量
	// （LawnDialog 的页脚按钮区域从 mHeight - 46 - 36 - 51 + 2 起算，约 y=319）。
	// ================================================================================================
	{
		const int PAGE_BUTTON_WIDTH = 94;
		const int PAGE_BUTTON_HEIGHT = 36;

		const int aLastRowBottom = aStartY - 12 + 45 + 2 * ((45 / 1.75f) + 10) + 45;
		int aPageY = aLastRowBottom + 10;
		const int aPageMaxY = mHeight - 130;
		if (aPageY > aPageMaxY)
			aPageY = aPageMaxY;

		const int aPageTotalWidth = PAGE_BUTTON_WIDTH * 2 + 12;
		int aPageX = (mWidth - aPageTotalWidth) / 2;
		if (aPageX < 20)
			aPageX = 20;

		mPage1->Resize(aPageX, aPageY, PAGE_BUTTON_WIDTH, PAGE_BUTTON_HEIGHT);
		mPage2->Resize(aPageX + PAGE_BUTTON_WIDTH + 12, aPageY, PAGE_BUTTON_WIDTH, PAGE_BUTTON_HEIGHT);
	}
}
void MoreSettingsDialog::Draw(Graphics* g)
{
	LawnDialog::Draw(g);

	int aStartY = mContentInsets.mTop + mBackgroundInsets.mTop + DIALOG_HEADER_OFFSET;
	if (mDialogHeader.size() > 0) 
	{
		int aOffsetY = aStartY - mHeaderFont->GetAscentPadding() + mHeaderFont->GetAscent();
		aStartY = aOffsetY - mHeaderFont->GetAscent() + mHeaderFont->GetHeight() + mSpaceAfterHeader;
	}

	int startX = mBackgroundInsets.mLeft + mContentInsets.mLeft + 2;
	int width = mWidth - mBackgroundInsets.mLeft - mContentInsets.mLeft - IMAGE_DIALOG_BOTTOMRIGHT->GetWidth() / 2 + 4;

	Color fontColor(107, 109, 145);
	TodDrawString(g, StrFormat("%d/%d", mCurPage + 1, MoreSettingsPages::NUM_OF_PAGES),
		startX + width + IMAGE_DIALOG_BOTTOMRIGHT->GetWidth() / 2 - 70 /*60*/, aStartY - DIALOG_HEADER_OFFSET + 24 /*28*/, FONT_DWARVENTODCRAFT18, fontColor, DS_ALIGN_RIGHT);

	g->PushState();
	g->SetColor(fontColor);
	//g->DrawLine(startX, aStartY, mBackgroundInsets.mLeft + mContentInsets.mLeft + 2 + width, aStartY);
	g->DrawLine(startX, aStartY - 7, mBackgroundInsets.mLeft + mContentInsets.mLeft + 2 + width, aStartY - 7);
	g->PopState();

	int aTextOffsetX = 40;
	int aTextOffsetY = 24;

	if (mCurPage == MoreSettingsPage_1)
	{
		TodDrawString(g, "3D Acceleration", mHardwareAcceleration->mX + aTextOffsetX, mHardwareAcceleration->mY + aTextOffsetY, FONT_DWARVENTODCRAFT18, fontColor, DS_ALIGN_LEFT);
		TodDrawString(g, "Custom Cursor", mCustomCursor->mX + aTextOffsetX, mCustomCursor->mY + aTextOffsetY, FONT_DWARVENTODCRAFT18, fontColor, DS_ALIGN_LEFT);
		TodDrawString(g, "Show FPS", mFPSToggle->mX + aTextOffsetX, mFPSToggle->mY + aTextOffsetY, FONT_DWARVENTODCRAFT18, fontColor, DS_ALIGN_LEFT);
		TodDrawString(g, "Auto-Pause", mAutoPause->mX + aTextOffsetX, mAutoPause->mY + aTextOffsetY, FONT_DWARVENTODCRAFT18, fontColor, DS_ALIGN_LEFT);
		TodDrawString(g, "Show Tooltip", mShowToolTip->mX + aTextOffsetX, mShowToolTip->mY + aTextOffsetY, FONT_DWARVENTODCRAFT18, fontColor, DS_ALIGN_LEFT);
	}
	else if (mCurPage == MoreSettingsPage_2)
	{
		TodDrawString(g, LAWN_LABEL_AUTO_COLLECT_SUN, mAutoCollectSun->mX + aTextOffsetX, mAutoCollectSun->mY + aTextOffsetY, FONT_DWARVENTODCRAFT18, fontColor, DS_ALIGN_LEFT);
		TodDrawString(g, LAWN_LABEL_AUTO_COLLECT_COINS, mAutoCollectCoins->mX + aTextOffsetX, mAutoCollectCoins->mY + aTextOffsetY, FONT_DWARVENTODCRAFT18, fontColor, DS_ALIGN_LEFT);
	}
}

void MoreSettingsDialog::CheckboxChecked(int theId, bool checked)
{
	switch (theId)
	{
		case MoreSettingsDialog::MoreSettingsDialog_HardwareAcceleration:
		{
			if (checked)
			{
				if (!mApp->Is3DAccelerationSupported())
				{
					mHardwareAcceleration->SetChecked(false, false);
					mApp->DoDialog(
						Dialogs::DIALOG_INFO,
						true,
						_S("[NOT_SUPPORTED_HEADER]"),
						_S("[NOT_SUPPORTED_LINES]"),
						_S("[OK_LABEL]"),
						Dialog::BUTTONS_FOOTER
					);

					return;
				}
				else if (!mApp->Is3DAccelerationRecommended())
				{
					mApp->DoDialog(
						Dialogs::DIALOG_INFO,
						true,
						_S("[WARNING_HEADER]"),
						_S("[WARNING_LINES]"),
						_S("[OK_LABEL]"),
						Dialog::BUTTONS_FOOTER
					);
				}

			}

			break;
		}

#ifdef _HAS_FEATURE_MENU
		case MoreSettingsDialog::MoreSettingsDialog_AutoCollectSun:
		{
			mApp->mFeatures.mAutoCollectSun = checked;
			break;
		}

		case MoreSettingsDialog::MoreSettingsDialog_AutoCollectCoins:
		{
			mApp->mFeatures.mAutoCollectCoins = checked;
			break;
		}
#endif
	}

	mApp->PlaySample(SOUND_BUTTONCLICK);
}

void MoreSettingsDialog::SelectPage(int thePageIndex)
{
	ChangePage((MoreSettingsPages)ClampInt(thePageIndex, 0, (int)NUM_OF_PAGES - 1));
}

void MoreSettingsDialog::ChangePage(MoreSettingsPages thePage)
{
	if (thePage < 0 || thePage >= NUM_OF_PAGES)
		return;

	if (thePage == mCurPage)
	{
		// 同一页也重排一次，避免窗口尺寸变化后控件留在旧位置
		Resize(mX, mY, mWidth, mHeight);
		return;
	}

	mCurPage = thePage;
	mApp->PlaySample(SOUND_BUTTONCLICK);

	mHardwareAcceleration->mVisible = mCustomCursor->mVisible = mFPSToggle->mVisible =
	mAutoPause->mVisible  = mShowToolTip->mVisible = mCurPage == MoreSettingsPage_1;

	mAutoCollectSun->mVisible = mAutoCollectCoins->mVisible = mCurPage == MoreSettingsPage_2;

	// 翻页时两组控件都要重排：第二页那两个也要拿到自己的位置，
	// 否则从第一页切过来时它们还停在 (0,0)。
	Resize(mX, mY, mWidth, mHeight);
}

// ====================================================================================================
// 翻页
//
// 原来的写法是**相对**翻页（按钮 1 = 上一页，按钮 2 = 下一页），只有两页时逻辑上能转，
// 但按钮命中依赖鼠标事件链（ButtonWidget::MouseUp 要求 mIsOver && mWidgetManager->mHasFocus），
// 一旦焦点/命中判定出点问题就"进不去第二页"。改成**绝对**选页，并且补上键盘 1 / 2 快捷键：
// 即使鼠标这条路出问题，键盘也能进第二页。
// ====================================================================================================
void MoreSettingsDialog::ButtonDepress(int theId)
{
	LawnDialog::ButtonDepress(theId);

	switch (theId)
	{
	case MoreSettingsDialog::MoreSettingsDialog_Page1:
		ChangePage(MoreSettingsPage_1);
		break;

	case MoreSettingsDialog::MoreSettingsDialog_Page2:
		ChangePage(MoreSettingsPage_2);
		break;
	}
}

void MoreSettingsDialog::KeyDown(KeyCode theKey)
{
	// 键盘 1 / 2 直接选页，不用点按钮
	if (theKey == MoreSettingsDialog::MoreSettingsDialog_Page1)
	{
		ChangePage(MoreSettingsPage_1);
		return;
	}
	if (theKey == MoreSettingsDialog::MoreSettingsDialog_Page2)
	{
		ChangePage(MoreSettingsPage_2);
		return;
	}

	// 方向键 / Tab 也能切换，方便手柄和键盘用户
	if (theKey == KeyCode::KEYCODE_RIGHT || theKey == KeyCode::KEYCODE_TAB)
	{
		ChangePage((MoreSettingsPages)min((int)mCurPage + 1, (int)NUM_OF_PAGES - 1));
		return;
	}
	if (theKey == KeyCode::KEYCODE_LEFT)
	{
		ChangePage((MoreSettingsPages)max((int)mCurPage - 1, 0));
		return;
	}

	LawnDialog::KeyDown(theKey);
}

void MoreSettingsDialog::Update()
{
	LawnDialog::Update();
}