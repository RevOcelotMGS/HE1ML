#include "Pch.h"
#include "Mod.h"
#include <filesystem>
#include "Utilities.h"
#include "CpkAdvancedConfig.h"
#include "Game.h"
#include "Globals.h"

bool Mod::Init(const std::string& path)
{
	const std::filesystem::path modPath = path;
	root = modPath.parent_path().string();

	const auto file = std::unique_ptr<Buffer>(read_file(path.c_str(), true));

	const Ini ini{ reinterpret_cast<char*>(file->memory), nullptr };
	const auto mainSection = ini["Main"];
	const auto descSection = ini["Desc"];
	const auto cpksSection = ini["CPKs"];

	if (cpksSection.valid())
	{
		for (const auto property : cpksSection)
		{
			InitAdvancedCpk(strtrim(property.value(), "\"").c_str());
		}
	}

	title = strtrim(descSection["Title"], "\"");
	LOG("Loading mod %s", title.c_str());

	const int includeDirCount = std::atoi(ini["Main"]["IncludeDirCount"]);
	char buf[32];

	for (int i = 0; i < includeDirCount; i++)
	{
		snprintf(buf, sizeof(buf), "IncludeDir%d", i);
		include_paths.push_back(strtrim(mainSection[buf], "\""));
	}

	const auto dllFilesRaw = strtrim(mainSection["DLLFile"], "\"");
	const auto dllPaths = strsplit(dllFilesRaw.c_str(), ",");

	SetDllDirectoryA(root.string().c_str());
	for (const auto& dllPath : dllPaths)
	{
		LOG("\t\tLoading DLL %s", dllPath.c_str());
		HMODULE mod = LoadLibraryA(dllPath.c_str());
		if (!mod)
		{
			LOG("\t\t\tFailed to load DLL %s", dllPath.c_str());
			continue;
		}

		modules.push_back(mod);
	}

	SetDllDirectoryA(nullptr);
	GetEvents("ProcessMessage", msg_processors);

	for (const auto& includePath : include_paths)
	{
		switch (g_game->id)
		{
		case eGameID_SonicGenerations:
			loader->binder->BindDirectory("Sound/", (root / includePath / "Sound").string().c_str());

		case eGameID_SonicLostWorld:
			loader->binder->BindDirectory("movie/", (root / includePath / "movie").string().c_str());
			break;

		default:
			break;
		}
	}

	return true;
}

void Mod::InitAdvancedCpk(const char* path)
{
	const auto file = std::unique_ptr<Buffer>(read_file((root / path).string().c_str(), true));
	CpkAdvancedConfig config{};
	config.Parse(reinterpret_cast<char*>(file->memory));
	config.Process(*loader->binder, root);
}

void Mod::RaiseEvent(const char* name, void* params) const
{
	for (auto& module : modules)
	{
		auto* pEvent = static_cast<void*>(GetProcAddress(module, name));
		if (pEvent)
		{
			reinterpret_cast<ModEvent_t*>(pEvent)(params);
		}
	}
}

int Mod::GetEvents(const char* name, std::vector<ModEvent_t*>& out) const
{
	int count = 0;
	for (auto& module : modules)
	{
		auto* pEvent = reinterpret_cast<ModEvent_t*>(GetProcAddress(module, name));
		if (pEvent)
		{
			out.push_back(pEvent);
			++count;
		}
	}

	return count;
}

void Mod::SendMessageImm(void* message) const
{
	for (auto& processor : msg_processors)
	{
		processor(message);
	}
}
