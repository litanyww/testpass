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
                    if (entry->d_name[0] != '.') {
                        switch (entry->d_type) {
                            case DT_REG:
                            case DT_LNK:
                                result.push_back(std::string(path) + "/" + entry->d_name);
                                break;
                            case DT_DIR:
                                {
                                    strings_t subdir = getFilesInDirectory(std::string(path) + "/" + entry->d_name);
                                    result.insert(result.end(), subdir.begin(), subdir.end());
                                }
                                break;
                            default:
                                break;
                        }

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
                    out_steps.addStep(ist);
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
            " -s CONDITIONS\tspecify the starting state" << std::endl <<
            " -r DIRECTORY\tspecify directory containing required tests" << std::endl <<
            " -i LOGFILE\tinteractive mode" << std::endl <<
            std::endl;
        }

    bool
        isFirstRequired(const WW::TestStep& step, WW::StepList& list)
        {
            bool result = false;
            WW::StepList::iterator it = list.find(step);
            if (it != list.end()) {
                result = true;
                list.erase(it);
            }
            return result;
        }

    void
        write_log(std::ostream& ost, const WW::TestStep& step, const std::string& flags, const std::string& note, const WW::Steps::attributes_t& state)
        {
            time_t when;
            time(&when);
            ost << step.short_desc() <<
                ":" << when <<
                ":" << flags <<
                ":" << WW::sanitize(note) <<
                std::endl <<
                ":" << state <<
                std::endl;
        }

    void
        write_log(const std::string& file, const WW::TestStep& step, const std::string& flags, const std::string& note, const WW::Steps::attributes_t& state)
        {
            std::ofstream ost(file.c_str(), std::ios_base::out | std::ios_base::app);
            write_log(ost, step, flags, note, state);
        }

    WW::Steps::attributes_t
        read_log(const std::string& logFile, WW::Steps& steps)
        {
            WW::Steps::attributes_t state;
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
                        std::string short_desc = text.substr(0, colon);
                        WW::TestStep* step = steps.step(short_desc, state);
                        if (step != 0) {
                            step->required(false);
                        }
                    }
                }
            }

            return state;
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
    std::string logFile;
    bool clearedRequired = false;

    bool loaded = false;
    WW::Steps steps;
    WW::Steps::attributes_t state;

    for (int arg = 1 ; arg < argc ; ++arg)
    {
        if (argv[arg][0] == '-') {
            switch (argv[arg][1]) {
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
    WW::StepList requiredSteps = steps.requiredSteps();

    if (interactive_mode) {
        WW::Steps::attributes_t logState = read_log(logFile, steps);

        if (state.size() == 0) {
            state = logState;
        }
        requiredSteps = steps.requiredSteps();
    }

    steps.setState(state);

    try
    {
        solution = steps.calculate();
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

    WW::StepList requiredCopy = requiredSteps;
    for (WW::StepList::const_iterator it = solution.begin(); it != solution.end(); ++it)
    {
        bool hasScript = false;

        if (!it->script().empty()) {
            hasScript = true;
        }
        char dot = hasScript ? '*' : '.';
        char space = isFirstRequired(*it, requiredCopy) ? '>' : ' ';
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

            std::string description = WW::strip(it->description());
            if (description[0] == '@') {
                // Use description from another test step
                const WW::TestStep* donor = steps.step(description.substr(1));
                if (donor != 0) {
                    description = donor->description();
                }
            }
            std::string script = WW::strip(it->script());
            if (script[0] == '@') {
                // Use script from another test step
                const WW::TestStep* donor = steps.step(script.substr(1));
                if (donor != 0) {
                    script = donor->script();
                }
            }

            bool hasScript = false;
            std::string note;

            if (!script.empty()) {
                hasScript = true;
            }
            char dot = hasScript ? '*' : '.';
            char space = isFirstRequired(*it, requiredSteps) ? '>' : ' ';
            bool showStep = true;
            std::string outcome = "";
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
                        description << std::endl <<
                        std::endl;
                    showStep = false;
                }
                if (description.empty() && script.empty()) {
                    // there's no interactive content; the step likely only exists to resolve dependencies
                    std::cout << "Skipping step because it has no content" << std::endl;
                    break;
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
                    std::cout << std::endl << script << std::endl <<
                        std::endl;
                }
                else if (hasScript && input[0] == 's')
                {
                    outcome += "s";
                    std::string output;
                    bool success = WW::executeScript(script, output);
                    note += output;
                    if (!success) {
                        outcome += "F";
                    }
                }
                else if (input[0] == 'f')
                {
                    outcome += "f";
                    note += WW::TestStep::strip(input.substr(1));
                    break;
                }
                else if (input[0] == 'F')
                {
                    outcome += "f";
                    note = WW::externalEditor(note);
                    break;
                }
                else if (input[0] == 'n')
                {
                    note += WW::TestStep::strip(input.substr(1));
                    break;
                }
                else if (input[0] == 'N')
                {
                    note = WW::externalEditor(note);
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
                        "F\t\tDescribe the failure using an external editor" << std::endl <<
                        "n NOTE\t\tAdd a note for this test step" << std::endl <<
                        "N\t\tEdit a note using an external editor" << std::endl <<
                        "p\t\tShow the test step details" << std::endl <<
                        "q\t\tQuit the test pass" << std::endl <<
                        "?\t\tShow this help" << std::endl <<
                        std::endl;
                }
                else if (input.empty())
                {
                    break;
                }
            }
            if (quitNow) {
                break;
            }
            it->operation().modify(state);
            write_log(logFile, *it, outcome, note, state);
        }
    }

    return 0;
}
