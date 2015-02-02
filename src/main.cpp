// Copyright 2015 Sophos Limited. All rights reserved.
//
// Sophos is a registered trademark of Sophos Limited and Sophos Group.
//

#include <gtest/gtest.h>

#include "Steps.h"
#include "TestException.h"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <dirent.h>
#include <strings.h>
#include <sys/types.h>

namespace {

    typedef std::vector<std::string> strings_t;

    strings_t
        getFilesInDirectory(const std::string& path)
        {
            strings_t result;
            struct dirent* entry;
            DIR* dir = opendir(path.c_str());
            if (dir != 0)
            {
                while ((entry = readdir(dir)) != 0)
                {
                    if (entry->d_name[0] != '.')
                    {
                        result.push_back(std::string(path) + "/" + entry->d_name);
                    }
                }
                closedir(dir);
            }
            return result;
        }

    void
        addDirectory(const std::string& path, WW::Steps& out_steps)
        {
            strings_t files = getFilesInDirectory(path);

            for (strings_t::const_iterator it = files.begin(); it != files.end(); ++it)
            {
                std::ifstream ist(it->c_str());
                if (ist.good())
                {
                    WW::TestStep step(ist);
                    // std::cout << "File: " << *it << ": " << step << std::endl;
                    out_steps.addStep(step);
                }
            }
        }
    void
        usage(const std::string& program_path)
        {
            std::string::size_type slash = program_path.find_last_of('/');
            std::string name = (slash == std::string::npos) ? program_path : program_path.substr(slash + 1);

            std::cout << "Usage: " << name << " [OPTIONS]... DIRECTORY..." << std::endl
            << "Construct a test pass based on test pass fragments" << std::endl
            << std::endl
            << "Options:" << std::endl
            << " -c\t\tcomplexity (default 5) higher for more suscinct results which takes longer to generate" << std::endl
            << " -r\t\tspecify directory containing required tests" << std::endl
            << std::endl;
        }

}

int main(int argc, char* argv[])
{
    static_cast<void>(argc);
    static_cast<void>(argv);

    unsigned int complexity = 5; // default complexity

    bool loaded = false;
    WW::Steps steps;

    for (unsigned int arg = 1 ; arg < argc ; ++arg)
    {
        if (argv[arg][0] == '-') {
            switch (argv[arg][1]) {
                case 'c': // complexity
                    if (argv[arg][2] != '\0') {
                        complexity = atoi(argv[arg] + 2);
                    }
                    else if (arg + 1 < argc) {
                        complexity = atoi(argv[++arg]);
                    }
                    break;
                case 'r': // required tests loaded from a specific folder
                    {
                        WW::Steps required;
                        if (argv[arg][2] != '\0') {
                            addDirectory(argv[arg] + 2, required);
                        }
                        else if (arg + 1 < argc) {
                            addDirectory(argv[++arg], required);
                        }
                        loaded = true;
                        steps.addRequired(required);
                    }
                    break;

                default:
                    usage(argv[0]);
                    return 0;
            }
        }
        else
        {
            WW::Steps items;
            addDirectory(argv[arg], items);
            steps.add(items);
            loaded = true;
        }
    }

    if (!loaded) {
        addDirectory("steps", steps);
    }

    WW::StepList solution;
    try
    {
        solution = steps.calculate(complexity);
    } catch (WW::TestException& e)
    {
        std::cerr << "ERROR: " << e.what() << std::endl;
        return 1;
    }

    std::cout << "dump of plan: " << std::endl << steps.debug_dump() << std::endl;
    unsigned int item = 0;

    for (WW::StepList::const_iterator it = solution.begin(); it != solution.end(); ++it)
    {
        if (it->script().empty()) {
            std::cout << ++item << ". " << it->short_desc() << std::endl;
        }
        else {
            std::cout << ++item << "* " << it->short_desc() << std::endl;
        }
    }


    return 0;
}
