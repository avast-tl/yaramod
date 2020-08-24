/**
 * @file src/types/modules/custom_module.cpp
 * @brief Implementation of CustomModule.
 * @copyright (c) 2017 Avast Software, licensed under the MIT license
 */

#include <sstream>

#include "yaramod/types/expression.h"
#include "yaramod/types/modules/custom_module.h"
#include "yaramod/types/symbol.h"

namespace yaramod {

using Json = nlohmann::json;


std::optional<ExpressionType> stringToExpressionType (const std::string& str)
{
	if (str == "undefined")
		return ExpressionType::Undefined;
	else if (str == "bool")
		return ExpressionType::Bool;
	else if (str == "int")
		return ExpressionType::Int;
	else if (str == "string")
		return ExpressionType::String;
	else if (str == "regexp")
		return ExpressionType::Regexp;
	else if (str == "object")
		return ExpressionType::Object;
	else if (str == "float")
		return ExpressionType::Float;
	return std::nullopt;
}

/**
 * Constructor.
 */
CustomModule::CustomModule(const std::string& name, const std::string& filePath)
	: Module(name, ImportFeatures::Basic)
{
	addPath(filePath);
}

void CustomModule::addPath(const std::string& path)
{
	_filePaths.push_back(path);
}

std::string CustomModule::getPathsAsString() const
{
	std::stringstream ss;
	for (const auto& path : _filePaths)
		ss << "'" << path << "', ";
	auto message = ss.str();
	message.erase(message.size()-2, 2);
	return message;
}

void CustomModule::_addValue(StructureSymbol* base, const Json& json)
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
			throw ModuleError("Colliding definitions of " + name + " attribute with different kind. " + getPathsAsString());
		if (existing.value()->getDataType() != type)
			throw ModuleError("Colliding definitions of " + name + " attribute. The value is defined twice with different types. " + getPathsAsString());
	}
	else
	{
		auto newValue = std::make_shared<ValueSymbol>(name, type);
		base->addAttribute(newValue);
	}
}

void CustomModule::_addFunctions(StructureSymbol* base, const Json& json)
{
	assert(accessJsonString(json, "kind") == "function");
	assert(base);

	auto name = accessJsonString(json, "name");

	auto arguments = accessJsonArray(json, "variants");
	for (const auto& typeArray : arguments)
	{
		std::vector<ExpressionType> typeVector;
		for (const auto& item : typeArray)
		{
			auto t = item.get<std::string>();
			auto type = stringToExpressionType(t);
			if (!type)
				throw ModuleError("Unknown function parameter type '" + t + "'");
			typeVector.emplace_back(type.value());
		}
		auto function = std::make_shared<FunctionSymbol>(name, typeVector);
		base->addAttribute(function);
	}
}

std::shared_ptr<StructureSymbol> CustomModule::_addStruct(StructureSymbol* base, const Json& json)
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

void CustomModule::_addAttributeFromJson(StructureSymbol* base, const Json& json)
{
	auto kind = accessJsonString(json, "kind");
	if (kind == "function")
		_addFunctions(base, json);
	else if (kind == "struct")
		_addStruct(base, json);
	else if (kind == "value")
		_addValue(base, json);
	else
		throw ModuleError("Unknown kind entry '" + kind + "'");
}

/**
 * Initializes module structure.
 *
 * @return @c true if success, otherwise @c false.
 */
bool CustomModule::initialize(ImportFeatures features)
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
		{
			_structure = _addStruct(nullptr, json);
			// TODO delete:
			_name = name;
		}
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
