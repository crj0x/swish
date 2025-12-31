#include "Utils.hpp"

#include <iostream>

int main()
{
  // Flush after every std::cout / std:cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

  // main REPL
  while (true)
  {
    std::vector<std::string> args = take_input();
    process_input(args);
  }
}
