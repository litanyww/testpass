// Copyright 2015 Sophos Limited. All rights reserved.
//
// Sophos is a registered trademark of Sophos Limited and Sophos Group.
//

#include "utils.h"

#include <iostream>

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <error.h>
#include <errno.h>
#include <sys/select.h>
#include <sys/wait.h>
#include <sys/types.h>

std::string
WW::strip(const std::string& text)
{
    std::string::size_type start = text.find_first_not_of("\r\n\t ");
    if (start == std::string::npos)
    {
        return std::string();
    }
    std::string::size_type end = text.find_last_not_of("\r\n\t ");
    // 0^2
    return text.substr(start, 1 + end - start);
}

#define TEMP_TEMPLATE "/tmp/testXXXXXX"
namespace {
    std::string
        makeScript(const std::string& contents)
        {
            char path[sizeof(TEMP_TEMPLATE)]; // sizeof includes the NUL terminator
            memcpy(path, TEMP_TEMPLATE, sizeof(TEMP_TEMPLATE));
            int fd = mkstemp(path);
            if (write(fd, contents.c_str(), contents.size()) == -1) {
                close(fd);
                unlink(path);
                return "";
            }
            close(fd);
            return path;
        }
}

bool readFromChild(pid_t child, int fd, std::string& output)
{
    int waitFlags = WNOHANG;
    for (;;) {
        int status;
        if (child == waitpid(child, &status, waitFlags)) {
            if (WEXITSTATUS(status) != 0) {
                std::cerr << "Script failed: " << WEXITSTATUS(status) << std::endl;
                break;
            }
            return true;
        }
        char buf[4096];
        ssize_t bytes = read(fd, buf, sizeof(buf));
        if (bytes < 0) {
            std::cerr << "ERROR: failed to get output from subprocess: " << strerror(errno) << std::endl;
            break;
        }
        else if (bytes == 0) {
            // Our pipe is no longer viable
            waitFlags = 0; // waitpid now waits forever
        }
        else {
            output.append(buf, bytes);
        }
    }
    return false;
}

bool
WW::executeScript(const std::string& script, std::string output)
{
    bool result = true;
    const std::string pathToScript = makeScript(script);
    int pipe_fd[2];
    if (pipe(pipe_fd) == -1)
    {
        std::cerr << "ERROR: failed to create pipe for output: " << strerror(errno) << std::endl;
        return false;
    }
    int child = fork();
    switch (child) {
        case 0: // child
            {
                dup2(pipe_fd[1], 1);
                dup2(pipe_fd[1], 2); // might be better to have separate stderr and stdout
                close(pipe_fd[0]);
                close(pipe_fd[1]);

                const std::string bashstr = "bash";
                char* bash = const_cast<char*>(bashstr.c_str());
                char* path = const_cast<char*>(pathToScript.c_str());
                char* const argv[] = {bash, path, 0};
                if (execvp("bash", argv) == -1) {
                    std::cerr << "ERROR: exec failed: " << strerror(errno) << std::endl;
                }
                exit(1);
                // not reached
            }
        case -1: // error
            {
                std::cerr << "ERROR: Failed to fork: " << strerror(errno) << std::endl;
                return false;
            }
        default: // main process
            close(pipe_fd[1]);
            break;
    }

    if (!readFromChild(child, pipe_fd[0], output)) {
        result = false;
    }

    close(pipe_fd[0]);
    unlink(pathToScript.c_str());

    return result;
}
