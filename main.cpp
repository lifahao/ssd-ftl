#include "ftl.h"



int main(int argc, char *argv[]) {
    if (argc <= 1 || argc > 3) {
        std::cerr << "Wrong num of argument !!!" << std::endl;
        return 1;
    }

    std::string configFile(argv[1]);
    std::string methodNum(argv[2]);

    SSD *ssdPtr = NULL;

    if (methodNum == std::string("1")) {
        ssdPtr = new SSD_1;
    } else if (methodNum == std::string("2")) {
        ssdPtr = new SSD_2;
    } else {
        std::cerr << "Wrong method number !!!" << std::endl;
        return 1;
    }

    Executor *executor = new Executor;
    executor->readConfigFile(configFile);

    executor->setSSD(ssdPtr);
    ssdPtr->allocateSpace(executor);
    std::cout << "allocate over \n";
    executor->initData();
    std::cout << "init over \n";
    executor->beginProcess();
    executor->endProcess();

    return 0;
}
