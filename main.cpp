#include "LawnApp.h"
#include "Resources.h"
#include "Sexy.TodLib/TodStringFile.h"
#include "GameConstants.h"

using namespace Sexy;

bool (*gAppCloseRequest)();				//[0x69E6A0]
bool (*gAppHasUsedCheatKeys)();			//[0x69E6A4]
SexyString (*gGetCurrentLevelName)();

//0x44E8F0
// Directory helpers now live in SexyAppFramework/Common.cpp so that the
// save-data selection can use the same ones (see Sexy::GetExeDirectory).

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow)
{
	gHInstance = hInstance;

	// Remember where the process was launched from: the "instance" mode in
	// savedata.ini uses it, and Init() changes the working directory later.
	{
		char aLaunchDir[MAX_PATH] = { 0 };
		if (GetCurrentDirectoryA(MAX_PATH, aLaunchDir) != 0)
			Sexy::SetLaunchDirectory(aLaunchDir);
	}

#if defined(_SHOW_OUTPUT_CONSOLE)
    AllocConsole();

    FILE* dummy;
    freopen_s(&dummy, "CONIN$", "r", stdin);
    freopen_s(&dummy, "CONOUT$", "w", stdout);
    freopen_s(&dummy, "CONOUT$", "w", stderr);

    HANDLE hInput = GetStdHandle(STD_INPUT_HANDLE);
    DWORD prevMode;
    GetConsoleMode(hInput, &prevMode);

	SetConsoleMode(hInput, (prevMode & ~ENABLE_QUICK_EDIT_MODE) | ENABLE_EXTENDED_FLAGS | ENABLE_INSERT_MODE | ENABLE_PROCESSED_INPUT);
#endif

	TodStringListSetColors(gLawnStringFormats, gLawnStringFormatCount);
	gGetCurrentLevelName = LawnGetCurrentLevelName;
	gAppCloseRequest = LawnGetCloseRequest;
	gAppHasUsedCheatKeys = LawnHasUsedCheatKeys;

	gLawnApp = new LawnApp();
	std::string exeDir = GetExeDirectory();
	if (Sexy::FileExists(exeDir + "\\properties\\resources.xml")) {
		gLawnApp->mChangeDirTo = exeDir;
	}
	else {
		gLawnApp->mChangeDirTo = (!Sexy::FileExists(_S("properties\\resources.xml")) &&
			Sexy::FileExists(_S("..\\properties\\resources.xml"))) ?
			_S("..") : _S(".");
	}
	
	gLawnApp->Init();
	gLawnApp->Start();
	gLawnApp->Shutdown();
	if (gLawnApp)
		delete gLawnApp;

	return 0;
};