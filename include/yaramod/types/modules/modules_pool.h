/**
 * @file src/types/modules/modules_pool.h
 * @brief Declaration of class Module.
 * @copyright (c) 2020 Avast Software, licensed under the MIT license
 */

#pragma once

#include "yaramod/modules_path.h"
#include "yaramod/types/modules/module.h"

#include <filesystem>

namespace yaramod {

/**
 * Class maintaining importable modules.
 */
class ModulesPool {
public:
	ModulesPool() : ModulesPool(YARAMOD_PUBLIC_MODULES_DIR)	{}
	ModulesPool(const std::string& directory);

	/**
	 * Loads the module based on its name from the table of known modules.
	 *
	 * @param name Name of the module to load
	 * @param features Determines which symbols to import
	 *
	 * @return Module if found, @c nullptr otherwise.
	 */
	std::shared_ptr<Module> load(const std::string& name)
	{
		auto itr = _knownModules.find(name);
		// Check that the module exists
		if (itr == _knownModules.end())
			return nullptr;

		// Initialize the module if it is not already initialized.
		if (!itr->second->isInitialized())
			itr->second->initialize();

		return itr->second;
	}

private:
	bool _addModule(std::filesystem::path path);
	std::unordered_map<std::string, std::shared_ptr<Module>> _knownModules = {}; ///< Table of all known modules
};

} //namespace yaramod
