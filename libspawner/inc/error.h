#ifndef _SPAWNER_ERROR_H_
#define _SPAWNER_ERROR_H_

#include <list>
#include <string>

#define LAST "Last function"

class error_list
{
private:
    static error_list instance;
    std::list<std::string> errors;
public:
    static void push_error(const std::string &place);
    static std::string pop_error();
    static bool remains();
};

template <class T>
void raise_error(T &obj, const std::string &place)
{
    error_list::push_error(place);
    obj.safe_release();
}

#endif//_SPAWNER_ERROR_H_