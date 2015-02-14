// Copyright 2015 Sophos Limited. All rights reserved.
//
// Sophos is a registered trademark of Sophos Limited and Sophos Group.
//

#ifndef INCLUDE_WW_UTILS_HEADER
#define INCLUDE_WW_UTILS_HEADER

#include <string>
#include <vector>

namespace WW
{
    typedef std::vector<std::string> strings_t;

    std::string strip(const std::string& text);
    bool executeScript(const std::string& script, std::string output);
    std::string externalEditor(const std::string contentToEdit);
    std::string sanitize(const std::string& text);
    strings_t split(const std::string& text, char ch = ',', size_t max_split = static_cast<size_t>(-1));
    std::string toLower(const std::string& text);
    bool textToBoolean(const std::string& text);
}

#endif // INCLUDE_WW_UTILS_HEADER
