#pragma once
#include <vector>
#include <string>
#include <unordered_set>
#include <unordered_map>
#include <functional>

using CommandHandler = std::function<void(const std::vector<std::string> &args)>;
extern const std::unordered_set<std::string> builtin_names;
extern const std::unordered_map<std::string, CommandHandler> builtins;

void handle_exit(const std::vector<std::string> &args);
void handle_echo(const std::vector<std::string> &args);
void handle_pwd(const std::vector<std::string> &args);
void handle_cd(const std::vector<std::string> &args);
void handle_type(const std::vector<std::string> &args);
void handle_history(const std::vector<std::string> &args);