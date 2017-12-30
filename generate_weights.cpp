#include <iostream>
#include <fstream>
#include <sstream>

int main(int argc, char *argv[]) {
    int MAX_EDGE_WEIGHT = 100;
    if (argc != 2) {
        std::cerr << "usage: " << argv[0] << " inputfile" << std::endl;
        return 1;
    }

    srand (1337);

    std::string filename{argv[1]};
    std::ifstream inputfile{filename};
    std::ofstream outputfile{filename+".weighted"};

    if (!inputfile.is_open()) {
        throw "File " + filename + " not opened.";
    }

    int a, b;
    std::string line;
    while (std::getline(inputfile, line)) {
        if (line[0] != '#') {
            std::istringstream iss{line};
            iss >> a >> b;
            outputfile << a << " " << b << " " << rand() % MAX_EDGE_WEIGHT + 1 << std::endl;
        }
        else
            outputfile << line;
    }
}