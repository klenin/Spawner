#include "options.h"

void COptions::add_argument(std::string argument)
{
    arguments.push_back(argument);
}

std::string COptions::get_arguments()
{
    if (arguments.size() == 0)
        return "";
    std::string result = arguments.front();
    std::list<std::string>::iterator it = arguments.begin();
    while (++it != arguments.end())
        result += " " + *it;
    return result;
}