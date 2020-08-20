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
	CustomModule(std::vector<std::string>&& filePaths);
	/// @}

	/// @name Destructor
	/// @{
	virtual ~CustomModule() override = default;
	/// @}

	/// @name Initialization method
	/// @{
	virtual bool initialize(ImportFeatures features) override;
	/// @}

	/// @name Getters
	/// @{
	std::string getPathsAsString() const;
	/// @}

private:
	/**
	 * Creates function from supplied json and:
	 *  - adds the new function as a attribute of structure base or
	 *  - it modifies already existing attribute of base with the same name.
	 *
	 * @param json structure supplied in json to be created ("kind": "struct")
	 * @param base already existing Structure which gets the new structure as its attribute
	 */
	void _addFunctions(StructureSymbol* base, const Json& json);
	/**
	 * Creates structure from supplied json
	 * If base is supplied, this method returns nullptr and it either:
	 *  - adds structure from json as a attribute of base or
	 *  - it modifies already existing attribute of base with the same name.
	 * If base is nullptr, this method returns new Structure constructed from supplied json
	 *
	 * @param json structure supplied in json to be created ("kind": "struct")
	 * @param base already existing Structure which gets the new structure as its attribute
	 */
	std::shared_ptr<StructureSymbol> _addStruct(StructureSymbol* base, const Json& json);
	void _addAttributeFromJson(StructureSymbol* base, const Json& json);
	std::vector<std::string> _filePaths;
};

}
