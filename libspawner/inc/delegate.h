#ifndef _SPAWNER_DELEGATE_H_
#define _SPAWNER_DELEGATE_H_

#include <fstream>
#include <inc/securerunner.h>


class delegate_runner: public secure_runner
{
protected:
    virtual void create_process();
    virtual bool apply_restrictions();
    static handle_t job_object;
    static handle_t job_object_access_mutex;
    static void set_allow_breakaway(bool allow);
    //virtual void requisites();
public:
    delegate_runner(const std::string &program, const options_class &options, const restrictions_class &restrictions);
/*    delegate_runner(const std::string &program, const options_class &options, const restrictions_class &restrictions)
    {
        //
    }*/
};

class delegate_instance_runner: public secure_runner {
protected:
    virtual bool create_restrictions();
    virtual void wait();
    virtual void requisites_();
public:
    delegate_instance_runner(const std::string &program, const options_class &options, const restrictions_class &restrictions);
};


#endif//_SPAWNER_DELEGATE_H_