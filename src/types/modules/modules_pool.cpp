/**
 * @file src/types/modules/modules_pool.cpp
 * @brief Implementation of CustomModule.
 * @copyright (c) 2017 Avast Software, licensed under the MIT license
 */

#include <sstream>

#include "yaramod/types/modules/modules_pool.h"


namespace yaramod {

using Json = nlohmann::json;

bool ModulesPool::_addModule(std::filesystem::path p)
{
	if (p.extension() != ".json")
		return false;

	auto path = p.string();
	auto json = readJsonFile(path);
	if (!json.contains("kind") || accessJsonString(json, "kind") != "struct")
		return false;

	auto name = accessJsonString(json, "name");
	auto itr = _knownModules.find(name);
	if (itr == _knownModules.end())
	{
		auto module = std::make_shared<Module>(name, path);
		_knownModules.emplace(std::make_pair(name, std::move(module)));
	}
	else
		itr->second->addPath(path);

	return true;
}

ModulesPool::ModulesPool(const std::string& directory)
{
	bool found_modules = false;
	if (const char* env_p = std::getenv("HOME"))
		std::cout << "Your HOME is: " << env_p << '\n';

	if (const char* env_p = std::getenv("YARAMOD_MODULE_SPEC_PATH"))
	{
		std::cout << "Your YARAMOD_MODULE_SPEC_PATH is: " << env_p << '\n';
		std::stringstream paths;
		paths << env_p;
		for (std::string path; std::getline(paths, path, ':'); )
			found_modules = _addModule(std::filesystem::path(path));
	}

	// Try to load each file in the directory
	if (!found_modules)
	{
		for (const auto& entry : std::filesystem::directory_iterator(directory))
		{
			const auto& p = entry.path();
			found_modules = _addModule(p);
		}
	}

	// Initializes all modules
	for (auto itr = _knownModules.begin(); itr != _knownModules.end(); ++itr)
		itr->second->initialize();
}

}
