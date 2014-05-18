#include <iostream>
#include <fstream>
#include <string>

#include <spawner.h>
#include "arguments.h"





int main(int argc, char *argv[]) {
    settings_parser_c parser;
    parser.register_dictionaries(c_dictionaries);
    parser.register_parsers(c_parsers);
    parser.enable_dictionary("system");
    parser.parse(argc, argv);
    return 0;
    spawner_c spawner(argc, argv);

    spawner.init();
    spawner.run();
    
	return 0;
}
