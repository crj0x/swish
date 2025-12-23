#include "Utils.hpp"

#include <filesystem>
#include <cstdlib>  // for std::getenv
#include <unistd.h> // for POSIX access()

const char PATH_DELIMITER = ':';

std::vector<std::string> parse_input(const std::string &input)
{
  // supports cases like echo 'hello''world' -> ["echo", "helloworld"]
  std::vector<std::string> args;
  std::string token = "";

  bool in_single_quotes = false;
  bool in_double_quotes = false;

  for (size_t i = 0; i < input.size(); i++)
  {
    if (in_double_quotes)
    {
      if (input[i] == '\"')
      {
        in_double_quotes = false;
      }
      else
      {
        token += input[i];
      }
    }
    else if (in_single_quotes)
    {
      if (input[i] == '\'')
      {
        in_single_quotes = false;
      }
      else
      {
        token += input[i];
      }
    }
    else
    {
      // ignore spaces if previous was also space
      if ((input[i] == ' ') && (i == 0 || input[i - 1] == ' '))
      {
        continue;
      }
      else if (input[i] == '"')
      {
        in_double_quotes = true;
      }
      else if (input[i] == '\'')
      {
        in_single_quotes = true;
      }
      else if (input[i] == ' ')
      {
        args.push_back(token);
        token = "";
      }
      else
      {
        token += input[i];
      }
    }
  }
  if (token != "")
  {
    args.push_back(token);
  }

  return args;
}

bool is_executable(const std::string &path_str)
{
  // access only works on posix systems
  return access(path_str.c_str(), X_OK) == 0;
}

// Find absolute path of a command using PATH environment variable
std::string find_in_path(const std::string &cmd)
{
  // if it's already a path, check it directly
  if (cmd.find('/') != std::string::npos)
  {
    if (is_executable(cmd))
    {
      return cmd;
    }
    return "";
  }
  // get the PATH env var
  const char *path_env = std::getenv("PATH");
  if (path_env == nullptr)
  {
    return "";
  }
  std::string path_str(path_env);

  // TODO: handle edge cases in parsing like PATH starting with ":", or "::" in PATH
  // parse path_str to extract its elements
  std::vector<std::string> path_elements;

  size_t start = 0;
  size_t end = path_str.find(PATH_DELIMITER);
  while (end != std::string::npos)
  {
    path_elements.push_back(path_str.substr(start, end - start));
    start = end + 1;
    end = path_str.find(PATH_DELIMITER, start);
  }
  path_elements.push_back(path_str.substr(start));

  // TODO: handle cases like "type hello\ world.txt"
  // try the all the elements of path_elements
  for (const std::string &p : path_elements)
  {
    // get the full path of the file
    std::filesystem::path full_path = p;
    full_path = full_path / cmd;
    if (is_executable(full_path))
    {
      return full_path.string();
    }
  }
  return "";
}
