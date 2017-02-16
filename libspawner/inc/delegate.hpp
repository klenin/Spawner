#ifndef _SP_DELEGATE_HPP_
#define _SP_DELEGATE_HPP_

#include <fstream>
#include <string>

#include "securerunner.hpp"

class delegate_runner: public runner
{
private:
    std::string program_to_run;
    restrictions_class restrictions;
protected:
    virtual void create_process();
public:
    delegate_runner(const std::string &program, const options_class &options,
        const restrictions_class &restrictions);
};

#endif // _SP_DELEGATE_HPP_
