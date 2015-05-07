#ifndef _PROCESS_H_
#define _PROCESS_H_

class CProcess
{
public:
    CProcess(/*arguments*/);
    void SetRestrictions(/*restrictions*/);
    void SetArguments(); // ?!
    int Run();
    ~CProcess();
};

#endif//_PROCESS_H_
