#include <iostream>
#include <fstream>
#include <string>

#include <spawner.h>





int main(int argc, char *argv[]) {
    spawner_c spawner(argc, argv);

    spawner.init();
    spawner.run();
    
	return 0;
}
