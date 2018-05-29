#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <map>


int main(int ac, char **av) {
    if (ac != 4) {
        std::cerr << "wrong argument \n";
        return 1;
    }
    double tmpValue;

    int numLevel = std::atoi(av[2]),
            numBlock = std::atoi(av[3]);

    std::ofstream outStream;
    outStream.open("temperCal", std::ios_base::out);

    for (int times = 0; true; ++times) {
        double avTemper = 0;
        std::map<double, bool> mapTemper;

        for (int levelIndex = 0; levelIndex < numLevel; ++levelIndex) {
            std::ostringstream inFilenameStream;
            inFilenameStream << av[1] <<"_" << times << "_" << levelIndex;
            std::string inFilename = inFilenameStream.str();

            std::ifstream inStream;
            inStream.open(inFilename.c_str(), std::ios_base::in);
            if (inStream.is_open() == false) {
                return 0;
            }
            while (inStream.eof() == false){
                inStream >> tmpValue;
                avTemper += tmpValue / numBlock;
                mapTemper[tmpValue] = true;
            }
            inStream.close();
        }

        std::map<double, bool>::iterator iterTemper = mapTemper.end();
        --iterTemper;

        outStream << avTemper << "\t" << iterTemper->first << "\n";
    }

    outStream.close();
    return 0;
}
