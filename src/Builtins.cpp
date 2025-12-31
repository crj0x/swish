#include "Builtins.hpp"
#include "Utils.hpp"

#include <cstdlib> // for std::getenv, exit()
#include <iostream>
#include <filesystem>
#include <cmath>

using CommandHandler = std::function<void(const std::vector<std::string> &args)>;

const std::unordered_set<std::string> builtin_names = {"exit", "echo", "pwd", "cd", "type", "history"};
const std::unordered_map<std::string, CommandHandler> builtins = {
    {"exit", handle_exit},
    {"echo", handle_echo},
    {"pwd", handle_pwd},
    {"cd", handle_cd},
    {"type", handle_type},
    {"history", handle_history}};
std::vector<std::string> previous_commands;

void handle_exit(const std::vector<std::string> &args)
{
  std::exit(0);
}

void handle_echo(const std::vector<std::string> &args)
{
  for (size_t i = 1; i < args.size(); i++)
  {
    if (i > 1)
    {
      std::cout << ' ';
    }
    std::cout << args[i];
  }
  std::cout << std::endl;
}

void handle_pwd(const std::vector<std::string> &args)
{
  std::filesystem::path current_path = std::filesystem::current_path();
  std::cout << current_path.string() << std::endl;
}

void handle_cd(const std::vector<std::string> &args)
{
  // TODO: implement cd ~/dir0/dir1, cd -, cd ~username/dir0/dir1
  // TODO: handle multiple arguements
  std::string cd_arg;
  if (args.size() == 1)
  {
    // handle cd with no args
    cd_arg = "~";
  }
  else
  {
    cd_arg = args[1];
  }
  if (cd_arg == "~")
  {
    char *home_path = std::getenv("HOME");
    if (home_path == nullptr)
    {
      std::cerr << "cd: HOME not set" << std::endl;
      return;
    }
    cd_arg = std::string(home_path);
  }
  std::error_code ec;
  std::filesystem::current_path(cd_arg, ec);

  // error messages
  if (ec)
  {
    if (ec == std::errc::no_such_file_or_directory)
    {
      std::cerr << "cd: " << cd_arg << ": No such file or directory" << std::endl;
    }
    else if (ec == std::errc::permission_denied)
    {
      std::cerr << "cd: " << cd_arg << ": Permission denied" << std::endl;
    }
    else
    {
      std::cerr << "cd: " << cd_arg << ec.message() << std::endl;
    }
  }
}

void handle_type(const std::vector<std::string> &args)
{
  if (args.size() == 1)
  {
    std::cout << std::endl;
    return;
  }
  for (size_t i = 1; i < args.size(); i++)
  {
    if (builtin_names.count(args[i]))
    {
      std::cout << args[i] << " is a shell builtin" << std::endl;
    }
    else
    {
      std::string full_path = find_in_path(args[i]);
      if (full_path.empty())
      {
        std::cout << args[i] << ": not found" << std::endl;
      }
      else
      {
        std::cout << args[i] << " is " << full_path << std::endl;
      }
    }
  }
}

void handle_history(const std::vector<std::string> &args)
{
  int i_digit_count = 1;
  int next_digit_count_trigger = 10;
  size_t start_point = 1;
  if (args.size() == 2 && (std::stoi(args[1]) < previous_commands.size()))
  {
    start_point = previous_commands.size() - std::stoi(args[1]) + 1;
    int temp_start_point = start_point / 10;
    while (temp_start_point)
    {
      temp_start_point /= 10;
      i_digit_count++;
    }
    next_digit_count_trigger = pow(10, i_digit_count);
  }

  for (size_t i = start_point; i <= previous_commands.size(); i++)
  {
    if (i == next_digit_count_trigger)
    {
      next_digit_count_trigger *= 10;
      i_digit_count++;
    }

    // padding is not robust here, it will break for more than 10^5 commands...
    std::string padding(5 - i_digit_count, ' ');
    std::cout << padding << i << "  " << previous_commands[i - 1] << std::endl;
  }
}
