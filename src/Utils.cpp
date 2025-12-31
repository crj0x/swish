#include "Utils.hpp"
#include "Executor.hpp"
#include "Builtins.hpp"

#include <iostream>
#include <filesystem>
#include <cstdlib>  // for std::getenv
#include <unistd.h> // for POSIX access()
#include <fcntl.h>  // for open() syscall
#include <unordered_set>

#include <readline/readline.h>
#include <readline/history.h>

const char PATH_DELIMITER = ':';
const std::unordered_set<char> escapable_in_double_quotes = {'"', '\\', '$', '`'}; // '\n' escaping is handled within the logic itself

void print_prompt()
{
  // print the prompt
  std::cout << "$ ";
}

std::vector<std::string> take_input()
{
  tokenizer_status status = {false, false, false, true, ""};
  std::vector<std::string> args;
  std::string total_input = "";

  // take the input for the input string
  char *buf = readline("$ ");
  std::string input_line(buf);
  free(buf);

  total_input += input_line;
  status = tokenize_string(args, status, input_line);
  while (status.take_input_again)
  {
    char *buf = readline("> ");
    input_line = std::string(buf);
    free(buf);

    total_input += ("\n" + input_line);
    status = tokenize_string(args, status, input_line);
  }

  if (total_input != "")
  {
    add_history(total_input.c_str());
    // TODO: If i edit a command in "readline"'s history it is not updated in "previous_commands" vector.
    // that edited command in history persists throughout executions (unless it is the one getting executed)
    // but readline handles all of that for us
    previous_commands.push_back(total_input);
  }

  return args;
}

// simply parses the string "input" provided to it and tokenizes it
// returns a status about whether we need to keep taking in the input.
tokenizer_status tokenize_string(std::vector<std::string> &args, tokenizer_status status, const std::string &input)
{
  // supports cases like echo "hello""world" -> ["echo", "helloworld"]
  std::string &token = status.token;
  bool &in_single_quotes = status.in_single_quotes;
  bool &in_double_quotes = status.in_double_quotes;
  bool &in_backslash = status.in_backslash;
  bool &take_input_again = status.take_input_again;

  for (size_t i = 0; i < input.size(); i++)
  {
    if (in_backslash)
    {
      // i = 0 here basically means we are escaping the previous '\n'
      if (!in_double_quotes || escapable_in_double_quotes.count(input[i]) || i == 0)
      {
        token += input[i];
      }
      else
      {
        token += '\\';
        token += input[i];
      }
      in_backslash = false;
    }
    else if (in_double_quotes)
    {
      if (input[i] == '"')
      {
        in_double_quotes = false;
      }
      else if (input[i] == '\\')
      {
        in_backslash = true;
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
      else if (input[i] == '\\')
      {
        in_backslash = true;
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
  if (in_backslash)
  {
    return status;
  }
  if (in_double_quotes || in_single_quotes)
  {
    token += '\n';
    return status;
  }
  if (token != "")
  {
    // aviod putting in "" because of whitespaces at the end of the input e.g. "echo hello     " -> ["echo", "hello"]
    args.push_back(token);
    token = "";
  }
  take_input_again = false;

  return status;
}

void process_input(std::vector<std::string> &args)
{
  bool found_pipeline = std::find(args.begin(), args.end(), "|") != args.end();
  if (found_pipeline)
  {
    std::vector<std::vector<std::string>> commands;
    std::vector<std::string> current_command;
    for (size_t i = 0; i < args.size(); i++)
    {
      if (args[i] == "|")
      {
        commands.push_back(current_command);
        current_command.clear();
      }
      else
      {
        current_command.push_back(args[i]);
      }
    }
    commands.push_back(current_command);

    execute_pipeline(commands);
    return;
  }

  int file_fd;
  int saved_fd;
  bool found_redirection = false;
  // write only, create if file doesnt exist, trucate the file if contents are already there (empty the file)
  int write_flags = O_WRONLY | O_CREAT | O_TRUNC;
  int append_flags = O_WRONLY | O_CREAT | O_APPEND;
  // read, write for user, read for group, read for others
  int mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
  int redirected_stream_file_no;

  // TODO: the parser doesn't handle cases like "echo hello>a" for now it needs to have whitespaces in between
  for (size_t i = 0; i < args.size(); i++)
  {
    // for now assuming that there WILL be an arguement after > or 1>
    // assuming there will be only one kind of redirection in one command

    if (args[i] == ">" || args[i] == "1>")
    {
      found_redirection = true;
      file_fd = open(args[i + 1].c_str(), write_flags, mode);
      redirected_stream_file_no = STDOUT_FILENO;
    }
    else if (args[i] == "2>")
    {
      found_redirection = true;
      file_fd = open(args[i + 1].c_str(), write_flags, mode);
      redirected_stream_file_no = STDERR_FILENO;
    }
    else if (args[i] == ">>" || args[i] == "1>>")
    {
      found_redirection = true;
      file_fd = open(args[i + 1].c_str(), append_flags, mode);
      redirected_stream_file_no = STDOUT_FILENO;
    }
    else if (args[i] == "2>>")
    {
      found_redirection = true;
      file_fd = open(args[i + 1].c_str(), append_flags, mode);
      redirected_stream_file_no = STDERR_FILENO;
    }
    if (found_redirection)
    {
      args.erase(args.begin() + (i + 1)); // remove file name from args
      args.erase(args.begin() + (i));     // remove operator from args
      i--;                                // so that an arg doesn't get skipped when scanning
    }
  }
  if (found_redirection)
  {
    // TODO: error handling when opening files
    // create a new fd pointing to stdout/stderr
    saved_fd = dup(redirected_stream_file_no);
    // make original stdout/stderr fd point to the file
    dup2(file_fd, redirected_stream_file_no);
    // close file_fd we don't need it anymore
    close(file_fd);
  }

  if (args.size() == 0)
  {
    return;
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

  // do cleanup
  if (found_redirection)
  {
    // make original stdout/stderr fd point back to the stdout/stderr file description
    dup2(saved_fd, redirected_stream_file_no);
    // close the temporary saved_fd
    close(saved_fd);
  }
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
