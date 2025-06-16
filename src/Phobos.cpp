#include "Phobos.h"

#include <Drawing.h>
#include <SessionClass.h>
#include <Unsorted.h>

#include <Utilities/Debug.h>
#include <Utilities/Patch.h>
#include <Utilities/Macro.h>
#include "Utilities/AresHelper.h"
#include "Utilities/Parser.h"

bool Phobos::HideWarning = false;
bool Phobos::PoweredByEC = false;

HANDLE Phobos::hInstance = 0;

char Phobos::readBuffer[Phobos::readLength];
wchar_t Phobos::wideBuffer[Phobos::readLength];

const char* Phobos::AppIconPath = nullptr;

bool Phobos::ShowCurrentInfo = false;
bool Phobos::DisplayDamageNumbers = false;
bool Phobos::IsLoadingSaveGame = false;

bool Phobos::Optimizations::Applied = false;
bool Phobos::Optimizations::DisableRadDamageOnBuildings = true;
bool Phobos::Optimizations::DisableSyncLogging = false;

#ifdef STR_GIT_COMMIT
const wchar_t* Phobos::VersionDescription = L"Phobos sp nightly #" _STR(BUILD_NUMBER) L"+" _STR(MERGE_NUMBER) L"(" STR_GIT_COMMIT L")";
#elif !defined(IS_RELEASE_VER)
const wchar_t* Phobos::VersionDescription = L"Phobos sp build #" _STR(BUILD_NUMBER) L"+" _STR(MERGE_NUMBER) L"_" _STR(MERGE_PATCH);
#else
const wchar_t* Phobos::VersionDescription = L"Phobos sp release v" FILE_VERSION_STR;
#endif


void Phobos::CmdLineParse(char** ppArgs, int nNumArgs)
{
	bool foundInheritance = false;
	bool foundInclude = false;
	bool dontSetExceptionHandler =
#ifdef DEBUG
		true;
#else
		false;
#endif // DEBUG
	Parser<bool> boolParser { };

	// > 1 because the exe path itself counts as an argument, too!
	for (int i = 1; i < nNumArgs; i++)
	{
		const char* pArg = ppArgs[i];
		std::string arg = pArg;

		if (_stricmp(pArg, "-Icon") == 0)
		{
			Phobos::AppIconPath = ppArgs[++i];
		}
		if (_stricmp(pArg, "-SPB=" _STR(BUILD_NUMBER) "+" _STR(MERGE_NUMBER) "_" _STR(MERGE_PATCH)) == 0)
		{
			Phobos::HideWarning = true;
		}
		if (_stricmp(pArg, "-Inheritance") == 0)
		{
			foundInheritance = true;
		}
		if (_stricmp(pArg, "-Include") == 0)
		{
			foundInclude = true;
		}
		if (arg.starts_with("-ExceptionHandler="))
		{
			auto delimIndex = arg.find("=");
			auto value = arg.substr(delimIndex + 1, arg.size() - delimIndex - 1);

			bool v = dontSetExceptionHandler;
			if (boolParser.TryParse(value.c_str(), &v))
				dontSetExceptionHandler = !v;
		}
	}

	if (!Phobos::HideWarning && !Phobos::IsTrialValid() && !Phobos::PoweredByEC)
	{
		Debug::Log("Initialized version: " PRODUCT_VERSION " failed! \n");
		MessageBoxExW(NULL, L"试用期已结束，且未检测到授权！", Phobos::VersionDescription, MB_ICONERROR, 0);
		FatalExit(0xDEAD);
	}

	if (foundInclude)
	{
		Patch::Apply_RAW(0x474200, // Apply CCINIClass_ReadCCFile1_DisableAres
			{ 0x8B, 0xF1, 0x8D, 0x54, 0x24, 0x0C }
		);

		Patch::Apply_RAW(0x474314, // Apply CCINIClass_ReadCCFile2_DisableAres
			{ 0x81, 0xC4, 0xA8, 0x00, 0x00, 0x00 }
		);
	}
	else
	{
		Patch::Apply_RAW(0x474230, // Revert CCINIClass_Load_Inheritance
			{ 0x8B, 0xE8, 0x88, 0x5E, 0x40 }
		);
	}

	if (foundInheritance)
	{
		Patch::Apply_RAW(0x528A10, // Apply INIClass_GetString_DisableAres
			{ 0x83, 0xEC, 0x0C, 0x33, 0xC0 }
		);

		Patch::Apply_RAW(0x526CC0, // Apply INIClass_GetKeyName_DisableAres
			{ 0x8B, 0x54, 0x24, 0x04, 0x83, 0xEC, 0x0C }
		);
	}
	else
	{
		Patch::Apply_RAW(0x528BAC, // Revert INIClass_GetString_Inheritance_NoEntry
			{ 0x8B, 0x7C, 0x24, 0x2C, 0x33, 0xC0, 0x8B, 0x4C, 0x24, 0x28 }
		);
	}

	Game::DontSetExceptionHandler = dontSetExceptionHandler;

	Debug::Log("Initialized version: " PRODUCT_VERSION "\n");
	Debug::Log("ExceptionHandler is %s\n", dontSetExceptionHandler ? "not present" : "present");
}

void Phobos::ExeRun()
{
	Patch::ApplyStatic();

#ifdef DEBUG

	if (Phobos::DetachFromDebugger())
	{
		MessageBoxW(NULL,
		L"You can now attach a debugger.\n\n"

		L"Press OK to continue YR execution.",
		L"Debugger Notice", MB_OK);
	}
	else
	{
		MessageBoxW(NULL,
		L"You can now attach a debugger.\n\n"

		L"To attach a debugger find the YR process in Process Hacker "
		L"/ Visual Studio processes window and detach debuggers from it, "
		L"then you can attach your own debugger. After this you should "
		L"terminate Syringe.exe because it won't automatically exit when YR is closed.\n\n"

		L"Press OK to continue YR execution.",
		L"Debugger Notice", MB_OK);
	}

	if (!Console::Create())
	{
		MessageBoxW(NULL,
		L"Failed to allocate the debug console!",
		L"Debug Console Notice", MB_OK);
	}

#endif
}

void Phobos::ExeTerminate()
{
	Phobos::UpdateTrialFile(Phobos::GetCurrent());
	Console::Release();
}

std::filesystem::path Phobos::GetProgramDirectory()
{
	wchar_t buffer[MAX_PATH];
	GetModuleFileNameW(NULL, buffer, MAX_PATH);
	return std::filesystem::path(buffer).parent_path();
}

std::string Phobos::XorEncryptDecrypt(const std::string& data)
{
	std::string result = data;

	for (size_t i = 0; i < data.size(); ++i)
		result[i] = data[i] ^ ((0x55AA1234 >> (8 * (i % 4))) & 0xFF);

	return result;
}

std::string Phobos::TimeToString(time_t t)
{
    const std::tm* tm = std::localtime(&t);
    char buffer[30];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", tm);
    return buffer;
}

time_t Phobos::StringToTime(const std::string& s)
{
    std::tm tm = {};
    std::istringstream iss(s);
    iss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
    return mktime(&tm);
}

time_t Phobos::GetCurrent()
{
	return time(nullptr);
}

time_t Phobos::GetCompile()
{
    std::string dateStr = __DATE__;
    std::string timeStr = __TIME__;
    std::tm tm = {};
    std::istringstream(dateStr + " " + timeStr) >> std::get_time(&tm, "%b %d %Y %H:%M:%S");
    return mktime(&tm);
}

bool Phobos::IsTrialValid()
{
	std::filesystem::path recordFile = Phobos::GetProgramDirectory() / "ra2.dat";
	const time_t currentTime = Phobos::GetCurrent();
	const time_t compileTime = Phobos::GetCompile();

	if (currentTime < compileTime)
		return false;

	const double daysPassed = difftime(currentTime, compileTime) / (60 * 60 * 24);

	if (daysPassed > 15)
		return false;

	if (std::filesystem::exists(recordFile))
	{
		std::ifstream inFile(recordFile, std::ios::binary);
		std::string encryptedData((std::istreambuf_iterator<char>(inFile)), std::istreambuf_iterator<char>());

		if (!encryptedData.empty())
		{
			std::string lastTimeStr = Phobos::XorEncryptDecrypt(encryptedData);
			time_t lastTime = Phobos::StringToTime(lastTimeStr);

			if (currentTime < lastTime)
				return false;
		}
	}

	Phobos::UpdateTrialFile(currentTime);
	return true;
}

void Phobos::UpdateTrialFile(time_t t)
{
	std::filesystem::path recordFile = Phobos::GetProgramDirectory() / "ra2.dat";
	std::ofstream outFile(recordFile, std::ios::binary);
	std::string currentTimeStr = Phobos::TimeToString(t);
	outFile << Phobos::XorEncryptDecrypt(currentTimeStr);
}

// =============================
// hooks

bool __stdcall DllMain(HANDLE hInstance, DWORD dwReason, LPVOID v)
{
	if (dwReason == DLL_PROCESS_ATTACH)
	{
		Phobos::hInstance = hInstance;
	}
	return true;
}

DEFINE_HOOK(0x7CD810, ExeRun, 0x9)
{
	Phobos::ExeRun();
	AresHelper::Init();

	return 0;
}
// Avoid confusing the profiler unless really necessary
#ifdef DEBUG
DEFINE_NAKED_HOOK(0x7CD8EA, _ExeTerminate)
{
	// Call WinMain
	SET_REG32(EAX, 0x6BB9A0);
	CALL(EAX);
	PUSH_REG(EAX);

	__asm {call Phobos::ExeTerminate};

	// Jump back
	POP_REG(EAX);
	SET_REG32(EBX, 0x7CD8EF);
	__asm {jmp ebx};
}
#endif
DEFINE_HOOK(0x52F639, _YR_CmdLineParse, 0x5)
{
	GET(char**, ppArgs, ESI);
	GET(int, nNumArgs, EDI);

	Phobos::CmdLineParse(ppArgs, nNumArgs);
	Debug::LogDeferredFinalize();
	return 0;
}

DEFINE_HOOK(0x67E44D, LoadGame_SetFlag, 0x5)
{
	Phobos::IsLoadingSaveGame = true;
	return 0;
}

DEFINE_HOOK(0x67E68A, LoadGame_UnsetFlag, 0x5)
{
	Phobos::IsLoadingSaveGame = false;
	Phobos::ApplyOptimizations();
	return 0;
}

DEFINE_HOOK(0x683E7F, ScenarioClass_Start_Optimizations, 0x7)
{
	Phobos::ApplyOptimizations();
	return 0;
}
/*
DEFINE_HOOK(0x4F4583, GScreenClass_DrawText, 0x6)
{
	if (!Phobos::HideWarning)
	{
		RectangleStruct wanted = Drawing::GetTextDimensions(Phobos::VersionDescription, Point2D::Empty, 0);
		Point2D location { DSurface::Composite->GetWidth() - wanted.Width - 5, 5 };
		DSurface::Composite->DrawText(Phobos::VersionDescription, &location, 0x061C);
	}
	return 0;
}
*/
// Mainly used to disable hooks for optimization.
// Called after loading saved game and at end of scenario start after all INI data etc has been initialized.
// Only executed once per game session.
void Phobos::ApplyOptimizations()
{
	if (Phobos::Optimizations::Applied)
		return;

	// Disable BuildingClass_AI_Radiation
	if (Phobos::Optimizations::DisableRadDamageOnBuildings)
		Patch::Apply_RAW(0x43FB23, { 0x53, 0x55, 0x56, 0x8B, 0xF1 });

	if (SessionClass::IsMultiplayer())
	{
		// Disable MainLoop_SaveGame
		Patch::Apply_LJMP(0x55DBCD, 0x55DC99);
	}
	else
	{
		// Disable Random2Class_Random_SyncLog
		Patch::Apply_RAW(0x65C7D0, { 0xC3, 0x90, 0x90, 0x90, 0x90, 0x90 });

		// Disable Random2Class_RandomRanged_SyncLog
		Patch::Apply_RAW(0x65C88A, { 0xC2, 0x08, 0x00, 0x90, 0x90, 0x90 });

		// Disable FacingClass_Set_SyncLog
		Patch::Apply_RAW(0x4C9300, { 0x83, 0xEC, 0x10, 0x53, 0x56 });

		// Disable InfantryClass_AssignTarget_SyncLog
		Patch::Apply_RAW(0x51B1F0, { 0x53, 0x56, 0x8B, 0xF1, 0x57 });

		// Disable BuildingClass_AssignTarget_SyncLog
		Patch::Apply_RAW(0x443B90, { 0x56, 0x8B, 0xF1, 0x57, 0x83, 0xBE, 0xAC, 0x0, 0x0, 0x0, 0x13 });

		// Disable TechnoClass_AssignTarget_SyncLog
		Patch::Apply_RAW(0x6FCDB0, { 0x83, 0xEC, 0x0C, 0x53, 0x56 });

		// Disable AircraftClass_AssignDestination_SyncLog
		Patch::Apply_RAW(0x41AA80, { 0x53, 0x56, 0x57, 0x8B, 0x7C, 0x24, 0x10 });

		// Disable BuildingClass_AssignDestination_SyncLog
		Patch::Apply_RAW(0x455D50, { 0x56, 0x8B, 0xF1, 0x83, 0xBE, 0xAC, 0x0, 0x0, 0x0, 0x13 });

		// Disable InfantryClass_AssignDestination_SyncLog
		Patch::Apply_RAW(0x51AA40, { 0x83, 0xEC, 0x2C, 0x53, 0x55 });

		// Disable UnitClass_AssignDestination_SyncLog
		Patch::Apply_RAW(0x741970, { 0x81, 0xEC, 0x80, 0x0, 0x0, 0x0 });

		// Disable AircraftClass_OverrideMission_SyncLog
		Patch::Apply_RAW(0x41BB30, { 0x8B, 0x81, 0xAC, 0x0, 0x0, 0x0 });

		// Disable FootClass_OverrideMission_SyncLog
		Patch::Apply_RAW(0x4D8F40, { 0x8B, 0x54, 0x24, 0x4, 0x56 });

		// Disable TechnoClass_OverrideMission_SyncLog
		Patch::Apply_RAW(0x7013A0, { 0x8B, 0x54, 0x24, 0x4, 0x56 });

		Phobos::Optimizations::DisableSyncLogging = true;
	}

	Phobos::Optimizations::Applied = true;
}
