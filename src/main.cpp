#include "Utils.hpp"

#include <iostream>
#include <cstdlib>
#include <readline/history.h>

int main()
{
  // Flush after every std::cout / std:cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

  const char *histfile_env = std::getenv("HISTFILE");
  if (histfile_env != nullptr)
  {
    // read history initially
    read_history(histfile_env);
  }

  // main REPL
  while (true)
  {
    std::vector<std::string> args = take_input();
    process_input(args);
  }
}
