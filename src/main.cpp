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

    std::vector<std::string> args = take_input();

    if (args.size() == 0)
    {
      std::cout << std::endl;
      continue;
    }
    if (builtins.find(args[0]) != builtins.end())
    {
      // we use .at() here instead of builtins[] syntax as builtins is a const unordered_map
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
