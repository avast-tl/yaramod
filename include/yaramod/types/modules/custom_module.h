/**
 * @file src/types/modules/custom_module.h
 * @brief Declaration of CuckooModule.
 * @copyright (c) 2017 Avast Software, licensed under the MIT license
 */

#pragma once

#include "yaramod/types/modules/module.h"
#include "yaramod/utils/json.h"
#include "yaramod/yaramod_error.h"

namespace yaramod {

/**
 * Represents error in module specification.
 */
class ModuleError : public YaramodError
{
public:
	ModuleError(const std::string& errorMsg)
		: YaramodError(errorMsg)
	{
	}
	ModuleError(const ModuleError&) = default;
};

/**
 * Class representing @c cuckoo module.
 */
class CustomModule : public Module
{
public:
	/// @name Constructor
	/// @{
	CustomModule(const std::string& name, const std::string& path);
	/// @}

	/// @name Destructor
	/// @{
	virtual ~CustomModule() override = default;
	/// @}

	/// @name Initialization method
	/// @{
	virtual bool initialize(ImportFeatures features) override;
	void addPath(const std::string& path);
	/// @}

	/// @name Getters
	/// @{
	std::string getPathsAsString() const;
	/// @}

private:
	void _addAttributeFromJson(StructureSymbol* base, const nlohmann::json& json);
	/**
	 * Creates a dictionary or an array from given json depending on its `kind` value.
	 *  - A structured iterable is created when the json contains `structure` entry.
	 *  - Otherwise iterable without structure is created using `type` entry. 
	 * @param json structure supplied in json to be created ("kind" must be "array" or "dictionary")
	 * @param base already existing Structure which gets the new dictionary as its attribute. Must not be nullptr.
	 */
	void _addIterable(StructureSymbol* base, const nlohmann::json& json);
	/**
	 * Creates a function from supplied json and:
	 *  - adds the new function as a attribute of structure base or
	 *  - it modifies already existing attribute of base with the same name.
	 *
	 * @param json structure supplied in json to be created ("kind": "function")
	 * @param base already existing Structure which gets the new function as its attribute
	 */
	void _addFunctions(StructureSymbol* base, const nlohmann::json& json);
	/**
	 * Creates a structure from supplied json
	 * If base is supplied, this method returns nullptr and it either:
	 *  - adds structure from json as a attribute of base or
	 *  - it modifies already existing attribute of base with the same name.
	 * If base is nullptr, this method returns new Structure constructed from supplied json
	 *
	 * @param json structure supplied in json to be created ("kind": "struct")
	 * @param base already existing Structure which gets the new structure as its attribute. Can be nullptr.
	 */
	std::shared_ptr<StructureSymbol> _addStruct(StructureSymbol* base, const nlohmann::json& json);
	/**
	 * Creates a value from supplied json and:
	 * adds the new value as a attribute of structure base.
	 * When already existing attribute of base with specified name,
	 * this method checks that the values are the same.
	 *
	 * @param json structure supplied in json to be created ("kind": "value")
	 * @param base already existing Structure which gets the new value as its attribute. Must not be nullptr
	 */
	void _addValue(StructureSymbol* base, const nlohmann::json& json);

	std::vector<std::string> _filePaths;
};

}
