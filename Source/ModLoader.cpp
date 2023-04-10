// ReSharper disable CppExpressionWithoutSideEffects
#include "ModLoader.h"
#include "Globals.h"
#include "CRIWARE/Criware.h"
#include "Utilities.h"
#include "Mod.h"

void ModLoader::Init(const char* configPath)
{
	g_loader = this;
	g_binder = binder.get();
	g_vfs = vfs;
	
	{
		root_path.resize(MAX_PATH, 0);
		do
		{
			const DWORD size = GetModuleFileNameA(GetModuleHandleA(nullptr), root_path.data(), root_path.length());
			if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
			{
				root_path.resize(size);
				break;
			}

		} while (GetLastError() == ERROR_INSUFFICIENT_BUFFER);
		size_t pos = root_path.find_last_of('\\');
		if (pos == std::string::npos)
		{
			pos = root_path.find_last_of('/');
		}

		if (pos != std::string::npos)
		{
			root_path.resize(pos);
		}
		else
		{
			root_path.clear();
		}
	}

	if (!AttachConsole(ATTACH_PARENT_PROCESS))
	{
		AllocConsole();
	}

	freopen("CONOUT$", "w", stdout);
	config_path = configPath;

	const auto file = std::unique_ptr<Buffer>(read_file(config_path.c_str(), true));

	const Ini ini{ reinterpret_cast<char*>(file->memory) };
	const auto cpkSection = ini["CPKREDIR"];

	std::string dbPath = strtrim(cpkSection["ModsDbIni"], "\"");
	if (dbPath.empty())
	{
		dbPath = "Mods\\ModsDb.ini";
	}

	LoadDatabase(dbPath);
	InitCri(this);
}

void ModLoader::LoadDatabase(const std::string& databasePath, bool append)
{
	if (!append)
	{
		mods.clear();
	}

	database_path = databasePath;
	const auto file = std::unique_ptr<Buffer>(read_file(database_path.c_str(), true));
	if (!file)
	{
		return;
	}

	char buf[32];
	const auto ini = Ini{ reinterpret_cast<char*>(file->memory) };
	const auto mainSection = ini["Main"];
	const auto modsSection = ini["Mods"];

	const int activeModCount = std::stoi(mainSection["ActiveModCount"]);
	for (int i = 0; i < activeModCount; i++)
	{
		sprintf_s(buf, sizeof(buf), "ActiveMod%d", i);
		const auto modKey = strtrim(mainSection[buf], "\"");
		const auto* modPath = modsSection[modKey.c_str()];

		if (modPath)
		{
			RegisterMod(strtrim(modPath, "\""));
		}
	}
}

namespace v0
{
	struct Mod
	{
		const char* Name;
		const char* Path;
	};

	struct ModInfo
	{
		std::vector<Mod*>* ModList;
		Mod* CurrentMod;
	};
}

wchar_t CurrentDirectory[2048];
bool ModLoader::RegisterMod(const std::string& path)
{
	v0::Mod m{};
	v0::ModInfo info{ nullptr, &m };
	auto mod = std::make_unique<Mod>(this);

	m.Name = mod->title.c_str();
	m.Path = path.c_str();
	if (!mod->Init(path))
	{
		return false;
	}

	GetCurrentDirectoryW(2048, CurrentDirectory);
	SetCurrentDirectoryA(mod->root.string().c_str());
	mod->RaiseEvent("Init", &info);
	SetCurrentDirectoryW(CurrentDirectory);
	mods.push_back(std::move(mod));
	return true;
}

void ModLoader::BroadcastMessageImm(void* message) const
{
	for (const auto& mod : mods)
	{
		mod->SendMessageImm(message);
	}
}
