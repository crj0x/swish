#include "Executor.hpp"
#include "Builtins.hpp"

#include <iostream>
#include <unistd.h> // for fork, execvp
#include <sys/wait.h>

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
  // the cwd (current working directory) of the child process is also the same
  // when we do this the FD (file descriptor) table is preserved
  pid_t child_pid = fork();
  if (child_pid == 0)
  {
    // child process
    // execvp literally turns the process to the new executable so the current code is completely gone from the child process
    // the cwd (curent working directory even after execvp) remains the same
    // when we do this the FD (file descriptor) table is preserved
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

void execute_pipeline_command(const std::vector<std::string> &args, const int stdin_fd, const int stdout_fd)
{
  std::vector<char *> c_args;
  for (const std::string &arg : args)
  {
    c_args.push_back(const_cast<char *>(arg.c_str()));
  }
  c_args.push_back(nullptr);

  pid_t child_pid = fork();
  if (child_pid == 0)
  {
    if (stdin_fd != -1)
    {
      dup2(stdin_fd, 0);
      close(stdin_fd);
    }
    if (stdout_fd != -1)
    {
      dup2(stdout_fd, 1);
      close(stdout_fd);
    }
    if (builtin_names.find(args[0]) != builtin_names.end())
    {
      builtins.at(args[0])(args);
      exit(0);
    }
    else
    {
      execvp(c_args[0], c_args.data());
      std::cerr << args[0] << ": command not found" << std::endl;
      exit(1);
    }
  }
}

void execute_pipeline(const std::vector<std::vector<std::string>> &commands)
{
  int prev_read_pipe = -1;
  int current_pipe[2];

  for (size_t i = 0; i < commands.size(); i++)
  {
    if (i < commands.size() - 1)
    {
      pipe(current_pipe);
    }
    else
    {
      current_pipe[1] = -1;
    }
    execute_pipeline_command(commands[i], prev_read_pipe, current_pipe[1]);
    if (prev_read_pipe != -1)
      close(prev_read_pipe);
    if (current_pipe[1] != -1)
      close(current_pipe[1]);

    prev_read_pipe = current_pipe[0];
  }

  for (size_t i = 0; i < commands.size(); i++)
  {
    // reap zombies
    wait(NULL);
  }
}
