/**
 * @file src/types/modules/modules_pool.cpp
 * @brief Implementation of CustomModule.
 * @copyright (c) 2017 Avast Software, licensed under the MIT license
 */

#include <sstream>

#include "yaramod/types/modules/modules_pool.h"
#include <filesystem>


namespace yaramod {

using Json = nlohmann::json;

ModulesPool::ModulesPool(const std::string& directory)
{
	/* For each path create new module or add the path to existing one. */
	for (const auto& entry : std::filesystem::directory_iterator(directory))
	//for (const auto& path : paths)
	{
		const auto& p = entry.path();
		if (p.extension() == ".json")
		{
			auto path = p.string();
			auto json = readJsonFile(path);
			if (json.contains("kind") && accessJsonString(json, "kind") == "struct")
			{
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
		}
	}
	// Initializes all modules
	for (auto itr = _knownModules.begin(); itr != _knownModules.end(); ++itr)
		itr->second->initialize();
}

}
