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
CustomModule::CustomModule(const std::string& filePath)
	: Module(std::string{}, ImportFeatures::Basic)
	, _filePath(filePath)
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

void CustomModule::_addFunctions(StructureSymbol* base, const Json& json)
{
	assert(accessJsonString(json, "kind") == "function");

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
			else
				throw YaramodError("Unknown function parameter type '" + t + "'");
		}
		auto function = std::make_shared<FunctionSymbol>(name, typeVector);
		base->addAttribute(std::move(function));
	}
}

std::shared_ptr<StructureSymbol> CustomModule::_addStruct(StructureSymbol* base, const Json& json)
{
	assert(accessJsonString(json, "kind") == "struct");

	const auto& name = accessJsonString(json, "name");
	const auto& attributes = accessJsonArray(json, "attributes");
	auto structure = std::make_shared<StructureSymbol>(name);

	for (const auto& attr : attributes)
		_addAttributeFromJson(structure.get(), attr);
	if (base)
	{
		base->addAttribute(std::move(structure));
		return nullptr;
	}
	else
		return structure;
	return nullptr;
}

void CustomModule::_addAttributeFromJson(StructureSymbol* base, const Json& json)
{
	std::string kind = accessJsonString(json, "kind");
	if (kind == "function")
	{
		std::cout << "Passing function" << std::endl;
		_addFunctions(base, json);
	}
	if (kind == "struct")
	{
		std::cout << "Passing struct" << std::endl;
		_addStruct(base, json);
	}
}

/**
 * Initializes module structure.
 *
 * @return @c true if success, otherwise @c false.
 */
bool CustomModule::initialize(ImportFeatures features)
{
	auto json = readJsonFile(_filePath);

	if (accessJsonString(json, "kind") != "struct")
		throw YaramodError("The first level 'kind' entry must be 'struct' in '" + _filePath + "'");

	auto customStruct = _addStruct(nullptr, json);

	_structure = customStruct;
	return true;
}

}
