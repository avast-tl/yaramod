/**
 * @file src/types/modules/module.cpp
 * @brief Implementation of class Module.
 * @copyright (c) 2017 Avast Software, licensed under the MIT license
 */

#include <sstream>

#include "yaramod/types/expression.h"
#include "yaramod/types/modules/module.h"
#include "yaramod/types/symbol.h"

namespace yaramod {

using Json = nlohmann::json;

/**
 * A mapping converting a given string to corresponding ExpressionType.
 */
std::optional<ExpressionType> stringToExpressionType (const std::string& str)
{
	if (str == "undefined")
		return ExpressionType::Undefined;
	else if (str == "bool" || str == "b")
		return ExpressionType::Bool;
	else if (str == "int" || str == "i")
		return ExpressionType::Int;
	else if (str == "string" || str == "s")
		return ExpressionType::String;
	else if (str == "regexp" || str == "r")
		return ExpressionType::Regexp;
	else if (str == "object" || str == "o")
		return ExpressionType::Object;
	else if (str == "float" || str == "f")
		return ExpressionType::Float;
	return std::nullopt;
}

/**
 * Constructor.
 *
 * @param name Name of the module
 */
Module::Module(const std::string& name, const std::string& filePath)
	: _name(name)
	, _structure()
{
	addPath(filePath);
}

/**
 * Destructor.
 */
Module::~Module()
{
}

void Module::addPath(const std::string& path)
{
	_filePaths.push_back(path);
}

/**
 * Returns the name of the module.
 *
 * @return Module name.
 */
const std::string& Module::getName() const
{
	return _name;
}

/**
 * Returns the paths to JSON files specifying the module.
 *
 * @return Module JSON file paths.
 */
const std::vector<std::string>& Module::getPaths() const
{
	return _filePaths;
}

/**
 * Returns the paths to JSON files specifying the module as a single string.
 *
 * @return Module JSON file paths as a string.
 */
std::string Module::getPathsAsString() const
{
	std::stringstream ss;
	for (const auto& path : getPaths())
		ss << "'" << path << "', ";
	auto message = ss.str();
	message.erase(message.size()-2, 2);
	return message;
}

/**
 * Returns the structure symbol of the module.
 *
 * @return Module structure symbol.
 */
const std::shared_ptr<StructureSymbol>& Module::getStructure() const
{
	return _structure;
}

/**
 * Returns whether the module is already initialized.
 *
 * @return @c true if initialized, otherwise @c false.
 */
bool Module::isInitialized() const
{
	return _structure != nullptr;
}

/**
 * Creates a dictionary or an array from given json depending on its `kind` value.
 *  - A structured iterable is created when the json contains `structure` entry.
 *  - Otherwise iterable without structure is created using `type` entry. 
 * @param json structure supplied in json to be created ("kind" must be "array" or "dictionary")
 * @param base already existing Structure which gets the new dictionary as its attribute. Must not be nullptr.
 */
void Module::_addIterable(StructureSymbol* base, const Json& json)
{
	assert(base);

	bool is_dictionary = false;
	if (accessJsonString(json, "kind") == "dictionary")
		is_dictionary = true;
	if (!is_dictionary)
		assert(accessJsonString(json, "kind") == "array");

	auto name = accessJsonString(json, "name");

	std::optional<std::shared_ptr<Symbol>> existing = base->getAttribute(name);
	if (existing)
	{
		if (is_dictionary && existing.value()->getType() != Symbol::Type::Dictionary)
			throw ModuleError("Colliding definitions of " + name + " attribute with different kind. Expected dictionary." + getPathsAsString());
		if (!is_dictionary && existing.value()->getType() != Symbol::Type::Array)
			throw ModuleError("Colliding definitions of " + name + " attribute with different kind. Expected array." + getPathsAsString());
	}
	else if(json.contains("structure"))
	{
		auto structure_json = accessJsonSubjson(json, "structure");
		auto embedded_structure = _addStruct(nullptr, structure_json);

		if (is_dictionary)
			base->addAttribute(std::make_shared<DictionarySymbol>(name, embedded_structure));
		else
			base->addAttribute(std::make_shared<ArraySymbol>(name, embedded_structure));
	}
	else
	{
		auto t = stringToExpressionType(accessJsonString(json, "type"));
		if (!t)
			throw ModuleError("Unknown dictionary type '" + accessJsonString(json, "type") + "'");
		auto type = t.value();
		if (is_dictionary)
			base->addAttribute(std::make_shared<DictionarySymbol>(name, type));
		else
			base->addAttribute(std::make_shared<ArraySymbol>(name, type));
	}
}

/**
 * Creates a function from supplied json and:
 *  - adds the new function as a attribute of structure base or
 *  - it modifies already existing attribute of base with the same name.
 *
 * @param json structure supplied in json to be created ("kind": "function")
 * @param base already existing Structure which gets the new function as its attribute
 */
void Module::_addFunctions(StructureSymbol* base, const Json& json)
{
	assert(accessJsonString(json, "kind") == "function");
	assert(base);

	auto name = accessJsonString(json, "name");
	auto return_type = stringToExpressionType(accessJsonString(json, "return_type"));
	if (!return_type)
		throw ModuleError("Unknown function return type type '" + accessJsonString(json, "return_type") + "'");

	auto overloads = accessJsonArray(json, "overloads");
	for (const auto& overload : overloads)
	{
		auto typeVector = std::vector<ExpressionType> {return_type.value()};
		auto arguments = accessJsonArray(overload, "arguments");
		for (const auto& item : arguments)
		{
			auto t = accessJsonString(item, "type");
			auto type = stringToExpressionType(t);
			if (!type)
				throw ModuleError("Unknown function parameter type '" + t + "'");
			typeVector.emplace_back(type.value());
		}
		auto function = std::make_shared<FunctionSymbol>(name, typeVector);
		base->addAttribute(function);
	}
}

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
std::shared_ptr<StructureSymbol> Module::_addStruct(StructureSymbol* base, const Json& json)
{
	assert(accessJsonString(json, "kind") == "struct");

	auto name = accessJsonString(json, "name");
	auto attributes = accessJsonArray(json, "attributes");

	// Before creating new structure we first look for its existence within base attributes:	
	std::optional<std::shared_ptr<Symbol>> existing = base ? base->getAttribute(name) : std::nullopt;
	if (existing)
	{
		if (existing.value()->getType() != Symbol::Type::Structure)
			throw ModuleError("Expected " + name + " to be a struct within the module json files:\n" + getPathsAsString());
		auto existingStructure = std::static_pointer_cast<StructureSymbol>(existing.value());
		for (const auto& attr : attributes)
			_addAttributeFromJson(existingStructure.get(), attr);
	}
	else
	{
		auto newStructure = std::make_shared<StructureSymbol>(name);
		for (const auto& attr : attributes)
			_addAttributeFromJson(newStructure.get(), attr);
		if (!base)
			return newStructure;
		base->addAttribute(newStructure);
	}
	return nullptr;
}

/**
 * Creates a value from supplied json and:
 * adds the new value as a attribute of structure base.
 * When already existing attribute of base with specified name,
 * this method checks that the values are the same.
 *
 * @param json structure supplied in json to be created ("kind": "value")
 * @param base already existing Structure which gets the new value as its attribute. Must not be nullptr
 */
void Module::_addValue(StructureSymbol* base, const Json& json)
{
	assert(accessJsonString(json, "kind") == "value");
	assert(base);

	auto name = accessJsonString(json, "name");
	auto t = stringToExpressionType(accessJsonString(json, "type"));

	if (!t)
		throw ModuleError("Unknown value type '" + accessJsonString(json, "type") + "'");
	auto type = t.value();

	// Before creating new structure we first look for its existence within base attributes:	
	std::optional<std::shared_ptr<Symbol>> existing = base->getAttribute(name);
	if (existing)
	{
		if (existing.value()->getType() != Symbol::Type::Value)
			throw ModuleError("Colliding definitions of " + name + " attribute with different kind. Expected value." + getPathsAsString());
		if (existing.value()->getDataType() != type)
			throw ModuleError("Colliding definitions of " + name + " attribute. The value is defined twice with different types. " + getPathsAsString());
	}
	else
	{
		auto newValue = std::make_shared<ValueSymbol>(name, type);
		base->addAttribute(newValue);
	}
}

/**
 * Handler method to controll unwrapping of the json based on its `kind` entry.
 *
 * @param json structure to be parsed
 * @param structure to be modified by adding attribute built according to the json content
 */
void Module::_addAttributeFromJson(StructureSymbol* base, const Json& json)
{
	auto kind = accessJsonString(json, "kind");
	if (kind == "function")
		_addFunctions(base, json);
	else if (kind == "struct")
		_addStruct(base, json);
	else if (kind == "value")
		_addValue(base, json);
	else if (kind == "dictionary" || kind == "array")
		_addIterable(base, json);
	else
		throw ModuleError("Unknown kind entry '" + kind + "'");
}

/**
 * Initializes module structure.
 *
 * @return @c true if success, otherwise @c false.
 */
bool Module::initialize()
{
	if (_filePaths.empty())
		throw ModuleError("No .json file supplied to initialize a module.");

	for (const auto& filePath : _filePaths)
	{
		auto json = readJsonFile(filePath);
		if (accessJsonString(json, "kind") != "struct")
			throw ModuleError("The first level 'kind' entry must be 'struct' in " + filePath);
		auto name = accessJsonString(json, "name");
		if (name == std::string{})
			throw ModuleError("Module name must be non-empty.");
		else if (!_structure) // First iteration - need to create the structure.
			_structure = _addStruct(nullptr, json);
		else if (_name != name) // Throws - name of the module must be the same accross the files.
			throw ModuleError("Module name must be the same in all files, but " + name + " != " + _name + ".\n" + getPathsAsString());
		else // _struct already created, need only to add new attributes
		{
			const auto& attributes = accessJsonArray(json, "attributes");
			for (const auto& attr : attributes)
				_addAttributeFromJson(_structure.get(), attr);
		}
	}
	return true;
}

}
