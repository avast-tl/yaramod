/**
 * @file src/types/modules/modules_pool.h
 * @brief Declaration of class Module.
 * @copyright (c) 2020 Avast Software, licensed under the MIT license
 */

#pragma once

#include <iostream>

#include "yaramod/types/modules/module.h"
#include "yaramod/types/modules/modules.h"

namespace yaramod {

/**
 * Class maintaining importable modules.
 */
class ModulesPool {
public:
	ModulesPool() : ModulesPool("/home/ts/dev/yaramod/include/modules") {}
	ModulesPool(const std::string& directory);

	/**
	 * Loads the module based on its name from the table of known modules.
	 *
	 * @param name Name of the module to load
	 * @param features Determines which symbols to import
	 *
	 * @return Module if found, @c nullptr otherwise.
	 */
	std::shared_ptr<Module> load(const std::string& name, ImportFeatures features)
	{
		auto itr = _knownModules.find(name);
		// Check that the module exists
		if (itr == _knownModules.end())
		{
			std::cout << "Could not find module '" << name << "'" << std::endl;
			return nullptr;
		}

		// Initialize the module if it is not already initialized.
		if (!itr->second->isInitialized())
			itr->second->initialize(features);

		return itr->second;
	}

private:
	std::unordered_map<std::string, std::shared_ptr<CustomModule>> _knownModules = {
		// { "androguard", std::make_shared<AndroguardModule>() },
		// // { "cuckoo",     std::make_shared<CuckooModule>()     },
		// { "dex",        std::make_shared<DexModule>()        },
		// { "dotnet",     std::make_shared<DotnetModule>()     },
		// { "elf",        std::make_shared<ElfModule>()        },
		// { "hash",       std::make_shared<HashModule>()       },
		// { "macho",      std::make_shared<MachoModule>()      },
		// { "magic",      std::make_shared<MagicModule>()      },
		// { "math",       std::make_shared<MathModule>()       },
		// { "metadata",   std::make_shared<MetadataModule>()   },
		// { "pe",         std::make_shared<PeModule>()         },
		// { "phish",      std::make_shared<PhishModule>()      },
		// { "time",       std::make_shared<TimeModule>()       }
	}; ///< Table of all known modules
};

} //namespace yaramod
