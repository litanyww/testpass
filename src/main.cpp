// Copyright 2015 Sophos Limited. All rights reserved.
//
// Sophos is a registered trademark of Sophos Limited and Sophos Group.
//

#include <gtest/gtest.h>

#include "Steps.h"
#include "TestException.h"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {
    typedef WW::TestStep::value_type attributes_t;
    typedef std::vector<std::string> strings_t;

    strings_t splitOnLines(const std::string& text)
    {
        strings_t result;
        std::istringstream ist(text);
        while (ist)
        {
            std::string line;
            if (std::getline(ist, line) && text.size() > 0)
            {
                result.push_back(line);
            }
        }
        return result;
    }

    std::string strip(const std::string& text)
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

    strings_t split(const std::string& text, char ch = ',', size_t max_split = static_cast<size_t>(-1))
    {
        // 1,3
        strings_t result;
        std::string::size_type start = 0;
        std::string::size_type pos = text.find(ch);
        while (pos != std::string::npos && max_split++ > 1)
        {
            result.push_back(text.substr(start, pos - start));
            start = pos + 1;
            pos = text.find(ch, start);
        }
        result.push_back(text.substr(start));
        return result;
    }
    
    attributes_t attribute_list(const std::string& text)
    {
        attributes_t result;
        std::string::size_type start = 0;
        std::string::size_type pos = text.find(',');
        while (pos != std::string::npos)
        {
            if (text[start] == '!')
            {
                result.forbid(strip(text.substr(start + 1, pos - start - 1)));
            }
            else
            {
                result.require(strip(text.substr(start, pos - start)));
            }
            start = pos + 1;
            pos = text.find(',', start);
        }
        if (text[start] == '!')
        {
            result.forbid(strip(text.substr(start + 1)));
        }
        else
        {
            result.require(strip(text.substr(start)));
        }
        return result;
    }

    std::string toLower(const std::string& text)
    {
        std::string result = text;
        std::transform(result.begin(), result.begin(), result.end(), tolower);
        return result;
    }

    bool textToBoolean(const std::string& text)
    {
        std::string lcText(toLower(text));
        return text == "1" || text == "true" || text == "yes";
    }

    WW::TestStep makeStep(const std::string& instructions)
    {
        WW::TestStep step;
        step.required(true);

        strings_t lines = splitOnLines(instructions);
        for (strings_t::const_iterator it = lines.begin(); it != lines.end(); ++it)
        {
            strings_t x = split(*it, ':', 2);
            if (x.size() == 2)
            {
                if (x[0] == "dependencies" || x[0] == "requirements")
                {
                    step.dependencies(attribute_list(x[1]));
                }
                else if (x[0] == "changes")
                {
                    step.changes(attribute_list(x[1]));
                }
                else if (x[0] == "required")
                {
                    step.required(textToBoolean(x[1]));
                }
                else if (x[0] == "description")
                {
                    step.description(strip(x[1]));
                }
                else if (x[0] == "short")
                {
                    step.short_desc(strip(x[1]));
                }
                else if (x[0] == "cost")
                {
                    step.cost(atol(strip(x[1]).c_str()));
                }
                else
                {
                    std::cerr << "ERROR: unrecognized token '" << x[0] << "'" << std::endl;
                }
            }
        }
        std::cerr << "Step=" << step << std::endl;
        return step;
    }
}


int main(int argc, char* argv[])
{
    static_cast<void>(argc);
    static_cast<void>(argv);
WW::Steps steps;

    steps.addStep(makeStep( // check access eicar while autoclean is enabled will detect and clean it up
                "short:accessToEicarDeniedWithClean\n"
                "dependencies:haveEicar,onaccess,installed,autoclean\n"
                "changes:!haveEicar,!eicarInQuarantine\n"
                "required:yes\n"
                "cost:4\n"
                "description: access /tmp/eicar.com.  Access will be denied.  After a moment, the file /tmp/eicar.com will be removed"
                ));

    steps.addStep(makeStep( // check access is denied when configuration is set to deny access
                "short:accessToEicarDenied\n"
                "dependencies:haveEicar,onaccess,installed\n"
                "changes:eicarInQuarantine\n"
                "required:yes\n"
                "cost:4\n"
                "description: access /tmp/eicar.com.  Access will be denied."
                ));

    steps.addStep(makeStep( // clean eicar using quarantine
                "short:cleanQuarantine\n"
                "dependencies:eicarInQuarantine,!autoclean\n"
                "changes:!eicarInQuarantine,!haveEicar\n"
                "required:yes\n"
                "cost:3\n"
                "description: Check the quarantine.  Details about the file will be displayed.  Click the item, authenticate and choose 'Clean'"
                ));

    steps.addStep(makeStep( // install the product
                "short:install\n"
                "dependencies:!installed\n"
                "changes:installed,onaccess\n"
                "cost:5\n"
                "required:no\n" // force this as a requirement; meaning it'll happen before other test steps
                "description:install the product"
                ));
    steps.addStep(makeStep( // install the product
                "short:uninstall\n"
                "dependencies:installed\n"
                "changes:!installed,uninstalled\n"
                "cost:5\n"
                "required:no\n" // force this as a requirement; meaning it'll happen before other test steps
                "description:uninstall the product"
                ));
    steps.addStep(makeStep( // check the product is fully uninstalled
                "short:checkUninstall\n"
                "dependencies:uninstalled,!installed\n"
                "changes:!uninstalled\n"
                "cost:2\n"
                "required:yes\n"
                "description:Check none of our processes are running, and that all components have been removed"
                ));
    steps.addStep(makeStep( // configure autoclean
                "short:configureAutoClean\n"
                "dependencies:installed,!autoclean\n"
                "changes:autoclean\n"
                "cost:2\n"
                "required:no\n"
                "description:In the product preferences, configure automatic clean on detection"
                ));
    steps.addStep(makeStep( // set deny access
                "short:configureDenyAccess\n"
                "dependencies:installed\n"
                "changes:!autoclean\n"
                "cost:2\n"
                "required:no\n"
                "description:In the product preferences, configure on-access to deny on detection."
                ));
    steps.addStep(makeStep( // turn off on-access scanning
                "short:turnOffOnAccess\n"
                "dependencies:onaccess,installed\n"
                "changes:!onaccess\n"
                "cost:2\n"
                "required:no\n"
                "description:open the product preferenes, turn off on-access scanning in the on-access tab"
                ));
    steps.addStep(makeStep( // turn on on-access scanning
                "short:turnOnOnAccess\n"
                "dependencies:!onaccess,installed\n"
                "changes:onaccess\n"
                "cost:2\n"
                "required:no\n"
                "description:open the product preferenes, turn on on-access scanning in the on-access tab"
                ));
    steps.addStep(makeStep( // drop eicar
                "short:dropEicar\n"
                "dependencies:!haveEicar,!onaccess\n"
                "changes:haveEicar\n"
                "cost:1\n"
                "required:no\n"
                "description: put eicar onto the drive at /tmp/eicar.com"
                ));

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
