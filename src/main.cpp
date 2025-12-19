#include <iostream>
#include <string>
#include <sstream>
#include <unordered_set>
#include <vector>
#include <filesystem>
#include <unistd.h>

// preprocessor macro to detect the platform
#ifdef _WIN32
const char PATH_DELIMITER = ';';
#else
const char PATH_DELIMITER = ':';
#endif

int main()
{
  // Flush after every std::cout / std:cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

  std::unordered_set<std::string> builtins = {"exit", "echo", "type"};

  while (true)
  {
    // print the prompt
    std::cout << "$ ";

    // take the input for the input string
    std::string input_line;
    std::getline(std::cin, input_line);

    // wrap the string in a string stream to parse it
    std::stringstream tokenizer(input_line);

    // extract the first word in the stream
    std::string cmd_name;
    tokenizer >> cmd_name;

    if (cmd_name == "exit")
    {
      break;
    }
    else if (cmd_name == "echo")
    {
      std::string current_arg;
      bool is_first_arg = true;

      while (tokenizer >> current_arg)
      {
        if (!is_first_arg)
        {
          std::cout << ' ';
        }
        std::cout << current_arg;
        is_first_arg = false;
      }
      // the last new line character at the end
      std::cout << std::endl;
    }
    else if (cmd_name == "type")
    {
      std::string type_arg;
      tokenizer >> type_arg;
      bool found_flag = false;
      if (builtins.count(type_arg))
      {
        std::cout << type_arg << " is a shell builtin" << std::endl;
        found_flag = true;
      }
      else if (type_arg.find('/') != std::string::npos)
      {
        if (access(type_arg.c_str(), X_OK) == 0)
        {
          std::cout << type_arg << " is " << type_arg << std::endl;
          found_flag = true;
        }
      }
      else
      {
        // get the PATH env var
        const char *path_value = std::getenv("PATH");
        if (path_value != nullptr)
        {
          std::string path_str(path_value);

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
            full_path = full_path / type_arg;
            if (access(full_path.c_str(), X_OK) == 0)
            {
              std::cout << type_arg << " is " << full_path.string() << std::endl;
              found_flag = true;
              break;
            }
          }
        }
        if (!found_flag)
        {
          std::cout << type_arg << ": not found" << std::endl;
        }
      }
    }
    else
    {
      std::cout << cmd_name << ": command not found" << std::endl;
    }
  }
}
