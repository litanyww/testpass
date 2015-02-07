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

            std::cout << "Usage: " << name << " [OPTIONS]... DIRECTORY..." << std::endl <<
            "Construct a test pass based on test pass fragments which are loaded from the" << std::endl <<
            "specified directories" << std::endl <<
            std::endl <<
            "Options:" << std::endl <<
            " -c COMPLEXITY\thigher for more suscinct results which takes longer to" << std::endl <<
            "\t\tgenerate (default 2)" << std::endl <<
            " -r DIRECTORY\tspecify directory containing required tests" << std::endl <<
            " -s CONDITIONS\tspecify the starting state" << std::endl <<
            " -i\t\tinteractive mode" << std::endl <<
            std::endl;
        }

    bool
        isFirstRequired(const WW::TestStep& step, WW::StepList& list)
        {
            bool result = false;
            WW::StepList::iterator it = list.find(step);
            if (it != list.end())
            {
                result = true;
                list.erase(it);
            }
            return result;
        }

    std::string
        sanitize(const std::string& text)
        {
            std::ostringstream ost;
            std::string::size_type pos;
            std::string::size_type start = 0;
            while ((pos = text.find_first_of("\n\t", start)) != std::string::npos)
            {
                ost << text.substr(start, pos - start - 1);
                switch (text[pos])
                {
                    case '\n':
                        ost << "\\n";
                        break;
                    case '\t':
                        ost << "\\t";
                        break;
                    default:
                        std::cerr << "ERROR: unexpected code" << std::endl;
                        break;
                }
                start = pos + 1;
            }
            ost << text.substr(start);
            return ost.str();
        }

    void
        write_log(std::ostream& ost, const WW::TestStep& step, const std::string& flags, const std::string& note, const WW::Steps::attributes_t& state)
        {
            time_t when;
            time(&when);
            ost << ":" << state <<
                std::endl <<
                step.short_desc() <<
                ":" << when <<
                ":" << flags <<
                ":" << sanitize(note) <<
                std::endl;
        }

    void
        write_log(const std::string& file, const WW::TestStep& step, const std::string& flags, const std::string& note, const WW::Steps::attributes_t& state)
        {
            std::ofstream ost(file.c_str(), std::ios_base::out | std::ios_base::app);
            write_log(ost, step, flags, note, state);
        }

    strings_t
        read_log(const std::string& logFile, WW::Steps::attributes_t& state)
        {
            strings_t result;
            std::ifstream ifs(logFile.c_str());
            std::string text;

            while (ifs)
            {
                std::getline(ifs, text);
                std::string::size_type colon = text.find_first_of(':');
                if (colon != std::string::npos) {
                    if (colon == 0) {
                        state = WW::Steps::attributes_t(text.substr(1));
                    }
                    else {
                        result.push_back(text.substr(0, colon));
                    }
                }
            }

            return result;
        }

    void
        unsetRequired(WW::Steps& steps)
        {
            WW::StepList required = steps.requiredSteps();
            for (WW::StepList::const_iterator it = required.begin(); it != required.end(); ++it) {
                steps.markNotRequired(it->short_desc());
            }
        }
}

int main(int argc, char* argv[])
{
    bool interactive_mode = false;
    unsigned int complexity = 2; // default complexity
    std::string logFile;
    bool clearedRequired = false;

    bool loaded = false;
    WW::Steps steps;
    WW::Steps::attributes_t state;

    for (int arg = 1 ; arg < argc ; ++arg)
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
                        if (!clearedRequired)
                        {
                            clearedRequired = true;
                            unsetRequired(steps);
                        }

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

                case 's': // starting state
                    {
                        WW::Steps::attributes_t arg_state;
                        if (argv[arg][2] != '\0') {
                            arg_state = WW::Steps::attributes_t(argv[arg] + 2);
                        }
                        else if (arg + 1 < argc) {
                            arg_state = WW::Steps::attributes_t(argv[++arg]);
                        }
                        state.insert(arg_state.begin(), arg_state.end());
                    }
                    break;

                case 'i': // interactive mode
                    {
                        interactive_mode = true;
                        if (argv[arg][2] != '\0') {
                            logFile = argv[arg] + 2;
                        }
                        else if (arg + 1 < argc) {
                            logFile = argv[++arg];
                        }
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
            if (clearedRequired) {
                WW::StepList required = items.requiredSteps();
                for (WW::StepList::const_iterator it = required.begin(); it != required.end(); ++it) {
                    const WW::TestStep* step = steps.step(it->short_desc());
                    bool required = (step && step->required());
                    if (required != it->required()) {
                        WW::TestStep copy(*it);
                        copy.required(required);
                        items.addStep(copy); // this will replace the original
                    }
                }
            }

            steps.add(items);
            loaded = true;
        }
    }

    if (!loaded) {
        addDirectory("steps", steps);
    }

    WW::StepList solution;
    WW::StepList requiredSteps;

    if (interactive_mode) {
        WW::Steps::attributes_t logState;
        strings_t nonRequiredTests = read_log(logFile, logState);
        for (strings_t::const_iterator it = nonRequiredTests.begin(); it != nonRequiredTests.end(); ++it) {
            steps.markNotRequired(*it);

        }

        if (state.size() == 0) {
            const WW::TestStep* lastStep = steps.step(nonRequiredTests.back());
            lastStep->operation().modify(logState);

            state = logState;
        }
    }

    steps.setState(state);

    try
    {
        solution = steps.calculate(complexity);
        requiredSteps = steps.requiredSteps();
    } catch (WW::TestException& e)
    {
        std::cerr << "ERROR: " << e.what() << std::endl;
        return 1;
    }

    if (solution.size() == 0)
    {
        std::cout << "No tests to run" << std::endl;
        return 0;
    }

    std::cout << "dump of plan: " << std::endl;
    unsigned int item = 0;

    for (WW::StepList::const_iterator it = solution.begin(); it != solution.end(); ++it)
    {
        bool hasScript = false;

        if (!it->script().empty()) {
            hasScript = true;
        }
        char dot = hasScript ? '*' : '.';
        char space = isFirstRequired(*it, requiredSteps) ? '>' : ' ';
        std::cout << ++item << dot << space << it->short_desc() << std::endl;
    }

    if (interactive_mode)
    {
        // Interactive mode
        unsigned int item = 0;
        bool quitNow = false;
        for (WW::StepList::const_iterator it = solution.begin(); it != solution.end(); ++it)
        {
            if (quitNow)
            {
                break;
            }
            bool hasScript = false;

            if (!it->script().empty()) {
                hasScript = true;
            }
            char dot = hasScript ? '*' : '.';
            char space = isFirstRequired(*it, requiredSteps) ? '>' : ' ';
            bool showStep = true;
            std::string executedScript = "";
            std::cout << std::endl;

            for (int i = 0 ; i < 78 ; ++i) {
                std::cout << '-';
            }

            for (int i = 0 ; i < 5 ; ++i) {
                std::cout << std::endl;
            }

            for (;;) {
                if (showStep)
                {
                    std::cout << std::endl <<
                        ++item << dot << space << it->short_desc() << std::endl <<
                        std::endl <<
                        it->description() << std::endl <<
                        std::endl;
                    showStep = false;
                }
                std::cout <<
                    "State: " << state << std::endl;
                std::string breadcrumb = "fnqp?";

                if (hasScript) {
                    breadcrumb = "sS" + breadcrumb;
                }
                std::cout << "[" << breadcrumb << "]";
                std::string input;
                std::getline(std::cin, input);

                input = WW::TestStep::strip(input);
                if (hasScript && input[0] == 'S')
                {
                    std::cout << std::endl << it->script() << std::endl <<
                        std::endl;
                }
                else if (hasScript && input[0] == 's')
                {
                    executedScript = "s";
                    std::cout << std::endl << "TODO: EXECUTE SCRIPT: " << it->script() << std::endl <<
                        std::endl;
                }
                else if (input[0] == 'f')
                {
                    write_log(logFile, *it, std::string("f") + executedScript, WW::TestStep::strip(input.substr(1)), state);
                    break;
                }
                else if (input[0] == 'n')
                {
                    write_log(logFile, *it, executedScript, WW::TestStep::strip(input.substr(1)), state);
                    break;
                }
                else if (input[0] == 'p')
                {
                    showStep = true;
                    break;
                }
                else if (input[0] == 'q')
                {
                    quitNow = true;
                    break;
                }
                else if (input[0] == '?')
                {
                    if (hasScript)
                    {
                        std::cout << "s\t\tExecute automation script associated with this step" << std::endl;
                        std::cout << "S\t\tShow automation script associated with this step" << std::endl;
                    }
                    std::cout << "f REASON\tLog a failure for this test step" << std::endl <<
                        "n NOTE\t\tAdd a note for this test step" << std::endl <<
                        "p\t\tShow the test step details" << std::endl <<
                        "q\t\tQuit the test pass" << std::endl <<
                        "?\t\tShow this help" << std::endl <<
                        std::endl;
                }
                else if (input.empty())
                {
                    write_log(logFile, *it, executedScript, "", state);
                    break;
                }
            }
            it->operation().modify(state);
        }
    }

    return 0;
}
