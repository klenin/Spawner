#ifndef _DELEGATE_RUNNER_H_
#define _DELEGATE_RUNNER_H_

#include <fstream>
#include <string>

#include "securerunner.h"

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
#endif // _DELEGATE_RUNNER_H
