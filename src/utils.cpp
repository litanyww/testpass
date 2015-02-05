// Copyright 2015 Sophos Limited. All rights reserved.
//
// Sophos is a registered trademark of Sophos Limited and Sophos Group.
//

#include "utils.h"

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
