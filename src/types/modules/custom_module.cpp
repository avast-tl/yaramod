/**
 * @file src/types/modules/custom_module.cpp
 * @brief Implementation of CustomModule.
 * @copyright (c) 2017 Avast Software, licensed under the MIT license
 */

#include <fstream>
#include <sstream>

#include "yaramod/types/expression.h"
#include "yaramod/types/modules/custom_module.h"
#include "yaramod/types/symbol.h"

namespace yaramod {

using Json = nlohmann::json;
using Type = Expression::Type;

/**
 * Constructor.
 */
CustomModule::CustomModule(std::vector<std::string>&& filePaths)
	: Module(std::string{}, ImportFeatures::Basic)
	, _filePaths(filePaths)
{
}

Json readJsonFile(const std::string& filePath)
{
	std::ifstream input{filePath, std::ios::out};
	if (!input.is_open())
		throw YaramodError("Could not open '" + filePath);
	std::stringstream buffer;
	buffer << input.rdbuf();
	return Json::parse(buffer.str());
}

template<typename T>
T accessJsonValue(const Json& json, const std::string& key)
{
	if (!json.contains(key))
	{
		std::stringstream ss;
		ss << "The key '" << key << "' not found among provided keys ";
		for (const auto& item : json.items())
			ss << "'" << item.key() << "', ";
		std::string message = ss.str();
		throw YaramodError(message.erase(message.size()-2, 2));
	}
	return json[key];
}

std::string accessJsonString(const Json& json, const std::string& key)
{
	return accessJsonValue<std::string>(json, key);
}

std::vector<Json> accessJsonArray(const Json& json, const std::string& key)
{
	return accessJsonValue<std::vector<Json>>(json, key);
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

void CustomModule::_addFunctions(StructureSymbol* base, const Json& json)
{
	assert(accessJsonString(json, "kind") == "function");
	assert(base);

	const auto& name = accessJsonString(json, "name");
	const auto& arguments = accessJsonArray(json, "arguments");
	for (const auto& typeArray : arguments)
	{
		std::vector<Type> typeVector;
		for (const auto& type : typeArray)
		{
			std::string t = type.get<std::string>();
			if (t == "int")
				typeVector.emplace_back(Type::Int);
			else if (t == "regexp")
				typeVector.emplace_back(Type::Regexp);
			else if (t == "string")
				typeVector.emplace_back(Type::String);
			else if (t == "float")
				typeVector.emplace_back(Type::Float);
			else
				throw YaramodError("Unknown function parameter type '" + t + "'");
		}
		auto function = std::make_shared<FunctionSymbol>(name, typeVector);
		base->addAttribute(function);
	}
}

std::shared_ptr<StructureSymbol> CustomModule::_addStruct(StructureSymbol* base, const Json& json)
{
	assert(accessJsonString(json, "kind") == "struct");

	const auto& name = accessJsonString(json, "name");
	const auto& attributes = accessJsonArray(json, "attributes");

	// Before creating new structure we first look for its existence within base attributes:	
	std::optional<std::shared_ptr<Symbol>> existing = base ? base->getAttribute(name) : std::nullopt;
	if (existing)
	{
		if (existing.value()->getType() != Symbol::Type::Structure)
			throw YaramodError("Expected " + name + " to be a struct within the module json files:\n" + getPathsAsString());
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
	std::string kind = accessJsonString(json, "kind");
	if (kind == "function")
		_addFunctions(base, json);
	else if (kind == "struct")
		_addStruct(base, json);
	else
		throw YaramodError("Unknown kind entry '" + kind + "'");
}

/**
 * Initializes module structure.
 *
 * @return @c true if success, otherwise @c false.
 */
bool CustomModule::initialize(ImportFeatures features)
{
	if (_filePaths.empty())
		throw YaramodError("No .json file supplied to initialize a module.");

	for (const auto& filePath : _filePaths)
	{
		auto json = readJsonFile(filePath);
		if (accessJsonString(json, "kind") != "struct")
			throw YaramodError("The first level 'kind' entry must be 'struct' in " + filePath);
		auto name = accessJsonString(json, "name");
		if (name == std::string{})
			throw YaramodError("Module name must be non-empty.");
		else if (!_structure) // First iteration - need to create the structure.
		{
			_structure = _addStruct(nullptr, json);
			_name = name;
		}
		else if (_name != name) // Throws - name of the module must be the same accross the files.
			throw YaramodError("Module name must be the same in all files, but " + name + " != " + _name + ".\n" + getPathsAsString());
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
