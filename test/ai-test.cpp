#include "ai/ScriptLoader.h"

#include "mechanics/Player.h"

#include <fstream> // NOLINT

int main(int argc, char *argv[]) {
    std::cout << "starting" << std::endl;
       if (argc < 2) {
           std::cerr << "pass file" << std::endl;
           return 1;
       }
       std::ifstream in;
       in.open(argv[1]);
       std::cout << in.good() << std::endl;
       Player player(0, 0, {});
       ai::ScriptLoader parser(&player);
       return parser.parse(in, std::cout);
}

