/**
 * @file src/types/modules/custom_module.h
 * @brief Declaration of CuckooModule.
 * @copyright (c) 2017 Avast Software, licensed under the MIT license
 */

#pragma once

#include "yaramod/types/modules/module.h"
#include <json/json.hpp>

namespace yaramod {

using Json = nlohmann::json;

/**
 * Class representing @c cuckoo module.
 */
class CustomModule : public Module
{
public:
	/// @name Constructors
	/// @param filePath a file containing the module structure written in json
	/// @{
	CustomModule(const std::string& filePath);
	/// @}

	/// @name Destructor
	/// @{
	virtual ~CustomModule() override = default;
	/// @}

	/// @name Initialization method
	/// @{
	virtual bool initialize(ImportFeatures features) override;
	/// @}
private:
	void _addFunctions(StructureSymbol* base, const Json& json);
	std::shared_ptr<StructureSymbol> _addStruct(StructureSymbol* base, const Json& json);
	void _addAttributeFromJson(StructureSymbol* base, const Json& json);
	std::string _filePath;
};

}
