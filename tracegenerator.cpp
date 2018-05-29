#include <iostream>
#include <fstream>
#include <random>

#include <cstdlib>

int main(int ac, char **av) {

    if (ac != 4) {
        std::cerr << "Wrong num of argument \n";
        return 1;
    }

    std::ifstream inStream;
    inStream.open(av[1], std::ios_base::in);
    if (inStream.is_open() == false) {
        std::cerr << "Fail open configur File \n";
        return 1;
    }

    std::ofstream outStream;
    outStream.open(av[2], std::ios_base::out);
    if (outStream.is_open() == false) {
        std::cerr << "Fail open output file \n";
        return 2;
    }

    int amountTrace = atoi(av[3]);
    if (amountTrace < 1) {
        std::cerr << "Fail transfer amount trace \n";
        return 3;
    }


    int bytePerPage,
            pagePerBlock,
            blockPerLine,
            linePerLevel,
            levelPerChip,
            minimumFreeBlock,
            numlogicPage;
    double logicBlockPercent;
    inStream >> bytePerPage
            >> pagePerBlock
            >> blockPerLine
            >> linePerLevel
            >> levelPerChip
            >> minimumFreeBlock
            >> logicBlockPercent;
    numlogicPage = (static_cast<int>(blockPerLine * linePerLevel *
                                  levelPerChip * logicBlockPercent)) * pagePerBlock;

    inStream.close();

    int logicPage = 0;
    std::default_random_engine generator(time(NULL));
    std::uniform_int_distribution<int> distribution(0, numlogicPage -1);
    for (int index = 0; index < amountTrace; ++index) {
        logicPage = distribution(generator);
        for (int pageNum = 0; pageNum < 8 &&  logicPage < numlogicPage;
             ++pageNum, ++logicPage) {
            outStream << logicPage << "\tw\n";
        }
    }

    std::default_random_engine generatorR(time(NULL));
    std::uniform_int_distribution<int> distributionR(static_cast<int>(numlogicPage / 10.0 * 9), numlogicPage -1);
    for (int index = 0 ; index < amountTrace; ++index) {
        logicPage = distributionR(generatorR);
        for (int pageNum = 0; pageNum < 8 &&  logicPage < numlogicPage;
             ++pageNum, ++logicPage) {
            outStream << logicPage << "\tr\n";
        }
    }


    outStream.close();
    return 0;
}
