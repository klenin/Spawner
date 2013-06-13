#include <iostream>
#include <fstream>
#include <string>

#include <spawner.h>





int main(int argc, char *argv[])
{
	//SetConsoleCP(1251);
	//SetConsoleOutputCP(1251);
    spawner_c spawner(argc, argv);

    spawner.init();
    spawner.run();
    
	return 0;
}
