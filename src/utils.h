// Copyright 2015 Sophos Limited. All rights reserved.
//
// Sophos is a registered trademark of Sophos Limited and Sophos Group.
//

#ifndef INCLUDE_WW_UTILS_HEADER
#define INCLUDE_WW_UTILS_HEADER

#include <string>

namespace WW
{
    std::string strip(const std::string& text);
    bool executeScript(const std::string& script, std::string output);
    std::string externalEditor(const std::string contentToEdit);
}

#endif // INCLUDE_WW_UTILS_HEADER
