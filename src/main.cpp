#include <iostream>
#include <string>
#include <sstream>

int main()
{
  // Flush after every std::cout / std:cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

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
    else
    {
      std::cout << cmd_name << ": command not found" << std::endl;
    }
  }
}
