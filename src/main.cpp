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

#include <sys/types.h>
#include <dirent.h>

namespace {

    typedef std::vector<std::string> strings_t;

    WW::TestStep makeStep(const std::string& instructions)
    {
        std::istringstream ist(instructions);
        return WW::TestStep(ist);
    }

    strings_t getFilesInDirectory(const std::string& path)
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
}

int main(int argc, char* argv[])
{
    static_cast<void>(argc);
    static_cast<void>(argv);

    std::string path = "steps";

    if (argc >= 2)
    {
        path = argv[1];
    }

    WW::Steps steps;

    strings_t files = getFilesInDirectory(path); // FIXME: get argument from argv

    for (strings_t::const_iterator it = files.begin(); it != files.end(); ++it)
    {
        std::ifstream ist(it->c_str());
        if (ist.good())
        {
            WW::TestStep step(ist);
            // std::cout << "File: " << *it << ": " << step << std::endl;
            steps.addStep(step);
        }
    }

    try
    {
        steps.calculate();
    } catch (WW::TestException& e)
    {
        std::cerr << "ERROR: " << e.what() << std::endl;
        return 1;
    }

    std::cout << "dump of plan: " << std::endl << steps.debug_dump() << std::endl;

    return 0;
}
