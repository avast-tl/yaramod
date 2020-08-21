/**
 * @file include/yaramod/utils/json.h
 * @brief Declaration of utility functions.
 * @copyright (c) 2017 Avast Software, licensed under the MIT license
 */

#pragma once

#include <json/json.hpp>

namespace yaramod {

nlohmann::json readJsonFile(const std::string& filePath);

template<typename T>
T accessJsonValue(const nlohmann::json& json, const std::string& key);
std::string accessJsonString(const nlohmann::json& json, const std::string& key);
std::vector<nlohmann::json> accessJsonArray(const nlohmann::json& json, const std::string& key);

}
