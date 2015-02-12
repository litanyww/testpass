// Copyright 2015 Sophos Limited. All rights reserved.
//
// Sophos is a registered trademark of Sophos Limited and Sophos Group.
//

#include "utils.h"

#include <iostream>
#include <fstream>
#include <sstream>

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/select.h>
#include <sys/wait.h>
#include <sys/types.h>

#define DEFAULT_EDITOR "vi"
#define TEMP_TEMPLATE "/tmp/testXXXXXX"

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

namespace {
    std::string
        makeTempFileWithContent(const std::string& contents)
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

    std::string
        readFileContent(const std::string path)
        {
            std::ostringstream result;
            std::ifstream ifs(path.c_str());
            while (ifs.good()) {
                std::string line;
                std::getline(ifs, line);
                result << line << std::endl;
            }
            return result.str();
        }
}

bool readFromChild(int fd, std::string& output)
{
    char buf[4096];
    ssize_t bytes = read(fd, buf, sizeof(buf));
    if (bytes < 0) {
        std::cerr << "ERROR: failed to get output from subprocess: " << strerror(errno) << std::endl;
    }
    else if (bytes > 0) {
        output.append(buf, bytes);
        return true;
    }
    return false;
}

bool
WW::executeScript(const std::string& script, std::string output)
{
    bool result = true;
    const std::string pathToScript = makeTempFileWithContent(script);
    int out_fd[2];
    int err_fd[2];
    if (pipe(out_fd) == -1)
    {
        std::cerr << "ERROR: failed to create pipe for output: " << strerror(errno) << std::endl;
        return false;
    }
    if (pipe(err_fd) == -1)
    {
        close(out_fd[0]);
        close(out_fd[1]);
        std::cerr << "ERROR: failed to create pipe for stderr: " << strerror(errno) << std::endl;
        return false;
    }

    int child = fork();
    switch (child) {
        case 0: // child
            {
                dup2(out_fd[1], 1);
                dup2(err_fd[1], 2); // might be better to have separate stderr and stdout
                close(out_fd[0]);
                close(out_fd[1]);
                close(err_fd[0]);
                close(err_fd[1]);

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
            close(out_fd[1]);
            close(err_fd[1]);
            break;
    }

    int status = 0;
    for (;;)
    {
        int waitflags = (out_fd[0] == -1 && err_fd[0] == -1) ? 0 : WNOHANG;
        if (child == waitpid(child, &status, waitflags)) {
            if (WEXITSTATUS(status) != 0) {
                result = false;
                std::cerr << "Script failed: " << WEXITSTATUS(status) << std::endl;
                break;
            }
            return true;
        }
        fd_set readfds;
        FD_ZERO(&readfds);
        int max = -1;
        if (out_fd[0] != -1) {
            FD_SET(out_fd[0], &readfds);
            if (max < out_fd[0]) {
                max = out_fd[0];
            }
        }
        if (err_fd[0] != -1) {
            FD_SET(err_fd[0], &readfds);
            if (max < err_fd[0]) {
                max = err_fd[0];
            }
        }

        int count = select(max + 1, &readfds, 0, 0, NULL);
        if (count > 0)
        {
            if (FD_ISSET(out_fd[0], &readfds)) {
                std::string out;
                if (!readFromChild(out_fd[0], out)) {
                    close(out_fd[0]);
                    out_fd[0] = -1;
                }
                else
                {
                    std::cout << out;
                    output.append(out);
                }
            }
            if (FD_ISSET(err_fd[0], &readfds)) {
                std::string err;
                if (!readFromChild(err_fd[0], err)) {
                    close(err_fd[0]);
                    err_fd[0] = -1;
                }
                else
                {
                    std::cerr << err;
                    output.append(err);
                }
            }
        }
    }

    if (out_fd[0] != -1) {
        close(out_fd[0]);
    }
    if (err_fd[0] != -1) {
        close(err_fd[0]);
    }
    unlink(pathToScript.c_str());

    return result;
}

std::string
WW::externalEditor(const std::string contentToEdit)
{
    std::string file = makeTempFileWithContent(contentToEdit);

    char defaultEditor[sizeof(DEFAULT_EDITOR)];
    memcpy(defaultEditor, DEFAULT_EDITOR, sizeof(DEFAULT_EDITOR));

    char* editor = getenv("EDITOR");
    if (editor == 0) {
        editor = defaultEditor;
    }

    int child = fork();
    switch (child) {
        case 0: // child
            {
                char* fileToEdit = const_cast<char*>(file.c_str());
                char* const argv[] = {editor, fileToEdit, 0 };
                execvp(editor, argv);
                exit(1);
            }
        case -1: // error
            {
                std::cerr << "ERROR: Failed to fork: " << strerror(errno) << std::endl;
                return "";
            }
        default:
            break;
    }

    int status;
    while (child != waitpid(child, &status, 0)) {
        std::cerr << "DEBUG: Error while waiting for child." << std::endl;
    }

    std::string result = readFileContent(file);
    unlink(file.c_str());
    return result;
}
