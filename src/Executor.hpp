#pragma once
#include <vector>
#include <string>

void execute_external(const std::vector<std::string> &args);
void execute_pipeline(const std::vector<std::vector<std::string>> &commands);