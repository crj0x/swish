#include "Utils.hpp"
#include "Builtins.hpp"
#include "Executor.hpp"

#include <iostream>

int main()
{
  // Flush after every std::cout / std:cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

  // main REPL
  while (true)
  {
    // print the prompt
    std::cout << "$ ";

    // take the input for the input string
    std::string input_line;
    // handle EOF
    if (!getline(std::cin, input_line))
    {
      std::cout << std::endl;
      break;
    }

    std::vector<std::string> args = parse_input(input_line);

    if (args.size() == 0)
    {
      std::cout << std::endl;
      continue;
    }
    if (builtins.find(args[0]) != builtins.end())
    {
      // we use .at() here instead of builtins[] syntax as builtins is a const map
      // builtins[args[0]] might modify builtins if args[0] doesnt exist in builtins
      // .at() just throws error if the key doesn't exist.
      builtins.at(args[0])(args);
    }
    else
    {
      execute_external(args);
    }
  }
}
