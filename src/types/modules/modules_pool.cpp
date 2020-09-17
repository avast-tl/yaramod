/**
 * @file src/types/modules/modules_pool.cpp
 * @brief Implementation of CustomModule.
 * @copyright (c) 2017 Avast Software, licensed under the MIT license
 */

#include <sstream>

#include "yaramod/types/modules/modules_pool.h"
#include <filesystem>
// #include "yaramod/utils/filesystem.h"


namespace yaramod {

using Json = nlohmann::json;

ModulesPool::ModulesPool(const std::string& directory)
{
	std::vector<std::string> paths = {
		"include/modules/module_cuckoo.json",
		"include/modules/module_pe.json",
		"include/modules/module_dotnet.json",
		"include/modules/module_elf.json",
		"include/modules/module_hash.json",
		"include/modules/module_magic.json",
		"include/modules/module_math.json",
		"include/modules/module_metadata.json",
		"include/modules/module_time.json",

		// "/home/ts/dev/yaramod/include/modules/module_cuckoo_avast.json",
		// "/home/ts/dev/yaramod/include/modules/module_cuckoo_deprecated.json",
		// "/home/ts/dev/yaramod/include/modules/module_androguard_avast.json",
		// "/home/ts/dev/yaramod/include/modules/module_dex.json",
		// "/home/ts/dev/yaramod/include/modules/module_macho.json",
		// "/home/ts/dev/yaramod/include/modules/module_metadata_avast.json",
		// "/home/ts/dev/yaramod/include/modules/module_phish.json"
	};

	/* For each path create new module or add the path to existing one. */
	/* for (const auto& entry : std::experimental::filesystem::directory_iterator(directory)) */
	for (const auto& path : paths)
	{
		// const auto& path = entry.path();
		auto json = readJsonFile(path);
		auto name = accessJsonString(json, "name");
		auto itr = _knownModules.find(name);
		if (itr == _knownModules.end())
		{
			auto module = std::make_shared<Module>(name, path);
			_knownModules.emplace(std::make_pair(name, std::move(module)));
		}
		else
			itr->second->addPath(path);
	}
	// Initializes all modules
	for (auto itr = _knownModules.begin(); itr != _knownModules.end(); ++itr)
		itr->second->initialize();
}

}
