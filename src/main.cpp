#include <ParseArguments.h>
#include <process.h>
#include <pipes.h>
#include <iostream>

int main(int argc, char *argv[])
{
	CArguments arg(argc, argv);
    CProcess process;
    //process.SetRestrictionForKind(RESTRICTION_MEMORY_LIMIT, 2000000);
    //process.SetRestrictionForKind(RESTRICTION_USER_TIME_LIMIT, 10000);
    //process.SetRestrictionForKind(RESTRICTION_SECURITY_LIMIT, RESTRICTION_LIMITED);
    process.RunAsync();
    string s;
    getline(process.stderror.stream(), s);
    cout << "!!!!" << s;
	return 0;
}