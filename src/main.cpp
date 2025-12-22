#include <iostream>
#include <string>
#include <sstream>
#include <unordered_set>
#include <vector>
#include <filesystem>
#include <unistd.h> // for fork, execvp, access
#include <sys/wait.h>
#include <functional>
#include <map>

// preprocessor macro to detect the platform
#ifdef _WIN32
const char PATH_DELIMITER = ';';
#else
const char PATH_DELIMITER = ':';
#endif

std::vector<std::string> parse_input(const std::string &input)
{
  // wrap the string in a string stream to parse it
  std::stringstream tokenizer(input);

  std::vector<std::string> args;
  std::string token;
  while (tokenizer >> token)
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

void handle_type(const std::string &command_name)
{
  std::string full_path = find_in_path(command_name);
  if (full_path.empty())
  {
    std::cout << command_name << ": not found" << std::endl;
  }
  else
  {
    std::cout << command_name << " is " << full_path << std::endl;
  }
}

void execute_external(const std::vector<std::string> &args)
{
  std::vector<char *> c_args;
  for (const std::string &arg : args)
  {
    // we can do (&arg[0]) because even std::strings also have a nullptr terminator at the end.
    // cast away the 'const' because execvp is a legacy C API that wants char*
    // .c_str() returns const char*, so we use const_cast to make it char*
    c_args.push_back(const_cast<char *>(arg.c_str()));
  }
  c_args.push_back(nullptr); // args list that we will pass to execvp also MUST end with nullptr.

  // run executables and pass arguements to them
  // here we can't use system() as it executes given command as a shell command
  // hence, defeating the purpose of creating a shell from scratch
  pid_t child_pid = fork();
  if (child_pid == 0)
  {
    // child process
    // execvp literally turns the process to the new executable so the current code is completely gone from the child process
    execvp(c_args[0], c_args.data());

    // If execvp returns, it failed (command not found/missing perms)
    std::cerr << args[0] << ": command not found" << std::endl;
    exit(1); // kill the child process with exit code 1
  }
  else if (child_pid > 0)
  {
    // parent process
    // wait for the child process to finish
    int status;
    waitpid(child_pid, &status, 0); // puts process to sleep. uses 0% cpu while asleep
  }
  else
  {
    std::cerr << "fork failed" << std::endl;
  }
}

int main()
{
  // Flush after every std::cout / std:cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

  std::unordered_set<std::string> builtin_names = {"exit", "echo", "type", "pwd", "cd"};

  using CommandHandler = std::function<void(const std::vector<std::string> &args)>;
  std::map<std::string, CommandHandler> builtins;
  builtins["exit"] = handle_exit;
  builtins["echo"] = handle_echo;
  builtins["pwd"] = handle_pwd;
  builtins["cd"] = handle_cd;
  builtins["type"] = [&](const std::vector<std::string> &args)
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
        handle_type(args[i]);
      }
    }
  };

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
      builtins[args[0]](args);
    }
    else
    {
      execute_external(args);
    }
  }
}
