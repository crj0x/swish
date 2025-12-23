#pragma once
#include <vector>
#include <string>

extern const char PATH_DELIMITER;

std::vector<std::string> parse_input(const std::string &input);
bool is_executable(const std::string &path_str);
std::string find_in_path(const std::string &cmd);