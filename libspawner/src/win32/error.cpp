#include <error.h>
#include <windows.h>
#include <sstream>

error_list error_list::instance;

void error_list::push_error(const std::string &where)
{
    std::stringstream res;
    LPCSTR errorText = NULL;
    DWORD error_code = GetLastError();
    FormatMessageA(
        // use system message tables to retrieve error text
        FORMAT_MESSAGE_FROM_SYSTEM
        // allocate buffer on local heap for error text
        |FORMAT_MESSAGE_ALLOCATE_BUFFER
        // Important! will fail otherwise, since we're not 
        // (and CANNOT) pass insertion parameters
        |FORMAT_MESSAGE_IGNORE_INSERTS,  
        NULL,    // unused with FORMAT_MESSAGE_FROM_SYSTEM
        error_code,
        MAKELANGID(LANG_ENGLISH /*LANG_NEUTRAL*/, SUBLANG_DEFAULT),
        (LPTSTR)&errorText,  // output 
        0, // minimum size for output buffer
        NULL);   // arguments - see note
    res << where << " failed with error code " << error_code << ": " << errorText;
    instance.errors.push_back(res.str());
}

std::string error_list::pop_error()
{
    if (!error_list::remains())
        return "";
    std::string res = instance.errors.front();
    instance.errors.pop_front();
    return res;
}

bool error_list::remains()
{
    return instance.errors.size() > 0;
}