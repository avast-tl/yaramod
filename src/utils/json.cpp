/**
 * @file src/utils/utils.cpp
 * @brief Implementation of utility functions.
 * @copyright (c) 2017 Avast Software, licensed under the MIT license
 */

#include <fstream>
#include <sstream>
#include <string>

#include "yaramod/utils/json.h"
#include "yaramod/yaramod_error.h"

namespace yaramod {

// using Json = nlohmann::json;

nlohmann::json readJsonFile(const std::string& filePath)
{
	std::ifstream input{filePath, std::ios::out};
	if (!input.is_open())
		throw YaramodError("Could not open '" + filePath);
	std::stringstream buffer;
	buffer << input.rdbuf();
	return nlohmann::json::parse(buffer.str());
}

template<typename T>
T accessJson(const nlohmann::json& json, const std::string& key)
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

std::string accessJsonString(const nlohmann::json& json, const std::string& key)
{
	return accessJson<std::string>(json, key);
}

std::vector<nlohmann::json> accessJsonArray(const nlohmann::json& json, const std::string& key)
{
	return accessJson<std::vector<nlohmann::json>>(json, key);
}

nlohmann::json accessJsonSubjson(const nlohmann::json& json, const std::string& key)
{
	return accessJson<nlohmann::json>(json, key);
}

}
