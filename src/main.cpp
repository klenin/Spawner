#include <ParseArguments.h>
#include <process.h>
#include <pipes.h>
#include <iostream>

int main(int argc, char *argv[])
{
	CArguments arg(argc, argv);
    CProcess process;
    process.RunAsync();
	return 0;
}