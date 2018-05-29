#include "ftl.h"



const static double LOWESTGAP = 1e-6;

// ====================PageData=========================
bool PageData::m_isRealData = false;
int PageData::m_bytePerPage = 4096;

PageData::PageData() : m_realPageData(NULL), m_notRealPageData(0),
    m_isValid(false), m_isFree(true), m_realPageNum(-1) {
}

PageData::~PageData() {
    if (m_realPageData) delete m_realPageData;
}

void PageData::clearData() {
    if (m_realPageData) {
        delete m_realPageData;
    }
    m_realPageData = NULL;
    m_notRealPageData = 0;
}

void PageData::copyData(const PageData *source) {
    if (source == NULL) return;

    if (m_isRealData) {
        assert(m_bytePerPage > 0);
        m_realPageData = m_realPageData ? m_realPageData : new char[m_bytePerPage];
        strncpy(m_realPageData, source->m_realPageData, m_bytePerPage);
    }
    m_notRealPageData = source->m_notRealPageData;
}

void PageData::mvData(PageData *source) {
    if (source == NULL) return;
    copyData(source);
    source->clearData();
}


bool PageData::getDataAsBit(char *realPageData) {
    if (realPageData == NULL ||
            m_isRealData == false ||
            m_realPageData == NULL) {
        return false;
    }
    strncpy(realPageData, m_realPageData, m_bytePerPage);
    return true;
}

bool PageData::getDataAsErrCount(int &notRealData) {
    if (m_isRealData) {
        return false;
    }
    notRealData = m_notRealPageData;
    return true;
}

bool PageData::setDataAsBit(const char *realPageData) {
    if (realPageData == NULL || m_isRealData == false) {
        return false;
    }

    assert(m_bytePerPage > 0);
    m_realPageData = m_realPageData ? m_realPageData : new char[m_bytePerPage];
    strncpy(m_realPageData, realPageData, m_bytePerPage);
    return true;
}

bool PageData::setDataAsErrCount(int notRealData) {
    if (m_isRealData == true || notRealData < 0) {
        return false;
    }
    m_notRealPageData = notRealData;
    return true;
}



void PageData::setValid(bool isValid) {
    m_isValid = isValid;
}

bool PageData::getValid() {
    return m_isValid;
}



void PageData::setIsFree(bool isFree) {
    m_isFree = isFree;
}

bool PageData::getIsFree() {
    return m_isFree;
}


bool PageData::setRealPageNum(int pageNum) {
    m_realPageNum = pageNum;
    return true;
}

int PageData::getRealPageNum() {
    return m_realPageNum;
}

// ====================BlockData=========================
int BlockData::m_pagePerBlock = 0;

BlockData::BlockData() : m_temper(25), m_eraseCount(0), m_isFree(true) {
    assert(m_pagePerBlock > 0);
    m_pageData = new PageData*[m_pagePerBlock];
    for (int pageIndex = 0; pageIndex < m_pagePerBlock; ++pageIndex) {
        m_pageData[pageIndex] = new PageData;
    }
}

BlockData::~BlockData() {
    for (int pageIndex = 0; pageIndex < m_pagePerBlock; ++pageIndex) {
        delete m_pageData[pageIndex];
    }
    delete m_pageData;

}



void BlockData::clearData() {
    for (int index = 0; index < m_pagePerBlock; ++index) {
        m_pageData[index]->clearData();
    }
}

void BlockData::copyData(BlockData *blockdata) {
    if (blockdata == NULL) return;

    for (int index = 0; index < m_pagePerBlock ; ++index) {
        if (blockdata->m_pageData[index]) {
            m_pageData[index]->copyData(blockdata->m_pageData[index]);
        }
    }
}

void BlockData::mvData(BlockData *blockdata) {
    if (blockdata == NULL) return;
    copyData(blockdata);
    blockdata->clearData();
}


PageData *BlockData::getPage(int pageNum) {
    if (pageNum < 0 || pageNum >= m_pagePerBlock) {
        std::cerr << "The pageNum is illegal !!!" << std::endl;
        return NULL;
    }
    return m_pageData[pageNum];
}

bool BlockData::getPageData(int offset, PageData &pagedata) {
    if (offset < 0 || offset >= m_pagePerBlock) {
        std::cerr << "The page num is ilegal !!!" << std::endl;
        return false;
    }

    pagedata.copyData(m_pageData[offset]);
    return true;
}

bool BlockData::setPageData(int offset, const PageData &pagedata) {
    if (offset < 0 || offset >= m_pagePerBlock) {
        std::cerr << "The page num is ilegal !!!" << std::endl;
        return false;
    }

    m_pageData[offset]->copyData(&pagedata);
    return true;
}



void BlockData::setTemper(double temper) {
    m_temper = temper;
}

double BlockData::getTemper() {
    return m_temper;
}



bool BlockData::addEraseCount(int eraseCount) {
    if (m_eraseCount + eraseCount < 0) return false;
    m_eraseCount += eraseCount;
    return true;
}

int BlockData::getEraseCount() {
    return m_eraseCount;
}



void BlockData::setIsFree(bool isFree) {
    m_isFree = isFree;
}

bool BlockData::getIsFree() {
    return m_isFree;
}

// ====================SSD=========================

// note: if m_executor != NULL, all space should-be allocated

int SSD::m_logicBlock = 0;
int SSD::m_physicBlock = 0;
int SSD::m_minimumFreeBlock = 1;

SSD::SSD() : m_executor(NULL), m_blockData(NULL),
    m_entryTable(NULL), m_replaceTable(NULL) {
}

SSD::~SSD() {
    if (m_executor != NULL) {
        reset();
    }
}


void SSD::allocateSpace(Executor *executor) {
    assert(executor != NULL);

    m_executor = executor;

    // set variation
    PageData::m_bytePerPage = m_executor->m_bytePerPage;
    PageData::m_isRealData = m_executor->m_isRealData;
    BlockData::m_pagePerBlock = m_executor->m_pagePerBlock;

    m_logicBlock = m_executor->m_logicBlock;
    m_physicBlock = m_executor->m_physicBlock;
    m_minimumFreeBlock = m_executor->m_minimumFreeBlock;

    assert(PageData::m_bytePerPage > 0);
    assert(BlockData::m_pagePerBlock > 0);
    assert(m_logicBlock > 0);
    assert(m_physicBlock > 0);
    assert(m_minimumFreeBlock >= 1);

    // allocate spaceee
    m_blockData = new BlockData*[m_physicBlock];
    for (int blockIndex = 0; blockIndex < m_physicBlock; ++blockIndex) {
        m_blockData[blockIndex] = new BlockData;
    }

    m_entryTable = new int[m_logicBlock];
    for (int index = 0; index < m_logicBlock; ++index) {
        m_entryTable[index] = -1;
    }
    m_replaceTable = new int[m_physicBlock];
    for (int index = 0; index < m_physicBlock; ++index) {
        m_replaceTable[index] = -1;
    }
}

bool SSD::initTable(std::string filename, std::string tablename) {
    assert(m_executor);
    if (m_executor->m_dataFromFile == false) return true;
    if (filename.empty()) return false;

    int *tablePtr = NULL, numElement = 0;
    if (tablename == "entryTable") {
        tablePtr = m_entryTable;
        numElement = m_logicBlock;
    } else if (tablename == "replaceTable") {
        tablePtr = m_replaceTable;
        numElement = m_physicBlock;
    } else {
        std::cerr << "Wrong table name !!!" << std::endl;
        return false;
    }

    // read value from file
    std::ifstream inStream;
    inStream.open(filename.c_str(), std::ios_base::in);
    if (!inStream.is_open()) {
        std::cerr << filename <<" file open error !!!" << std::endl;
        return false;
    }
    for (int index = 0; index < numElement; ++index) {
        inStream >> tablePtr[index];
    }
    return true;
}

bool SSD::writeBackTable(std::string filename, std::string tablename) {
    assert(m_executor);
    if (filename.empty()) {
        std::cerr << "The table " << tablename << " file is empty !!!" << std::endl;
        return false;
    }

    int *tablePtr = NULL, numElement = 0;
    if (tablename == "entryTable") {
        tablePtr = m_entryTable;
        numElement = m_logicBlock;
    } else if (tablename == "replaceTable") {
        tablePtr = m_replaceTable;
        numElement = m_physicBlock;
    } else {
        std::cerr << "Wrong table name !!!" << std::endl;
        return false;
    }

    // read value from file
    std::ofstream outStream;
    outStream.open(filename.c_str(), std::ios_base::out);

    if (!outStream.is_open()) {
        std::cerr << filename <<" file open error !!!" << std::endl;
        return false;
    }
    for (int index = 0; index < numElement; ++index) {
        outStream << tablePtr[index] << " ";
    }
    return true;
}


bool SSD::initFreeBlockList(std::string filename) {
    assert(m_executor);

    if (m_executor->m_dataFromFile == false) {
        for (int index = 0; index < m_physicBlock; ++index) {
            m_freeBlock.push_back(index);
        }
        return true;
    }
    if (filename.empty() == true) return false;

    std::ifstream inStream;
    inStream.open(filename.c_str(), std::ios_base::in);
    if (inStream.is_open() == false) {
        std::cerr << "Fail open freeBlockList File " << filename << std::endl;
        return false;
    }

    int freeBlock;
    while (inStream.eof() == false) {
        inStream >> freeBlock;
        m_freeBlock.push_back(freeBlock);
    }
    inStream.close();
    return true;
}

bool SSD::writeBackFreeBlockList(std::string filename) {
    assert(m_executor);

    if (filename.empty() == true) {
        std::cerr << "FreeBlockList filename shouldn't be empty !!!" << std::endl;
        return false;
    }

    std::ofstream outStream;
    outStream.open(filename.c_str(), std::ios_base::out);
    if (outStream.is_open() == false) {
        std::cerr << "Fail open freeBlockList File " << filename << std::endl;
        return false;
    }

    for (std::list<int>::iterator iterList = m_freeBlock.begin();
         iterList != m_freeBlock.end(); ++iterList) {
            outStream << *iterList << " ";
    }

    outStream.close();
    return true;
}


bool SSD::initMetaData(std::string filename) {
    assert(m_executor);
    if (m_executor->m_dataFromFile == false) return true;
    if (filename.empty() == true) return false;

    std::ifstream inStream;
    inStream.open(filename.c_str(), std::ios_base::in);
    if (inStream.is_open()) {
        std::cerr << "Fail open meta data file " << filename << std::endl;
        return false;
    }

    double blockTemper;
    int blockEraseCount, blockIsFree;
    int pageIsValid, pageIsFree, realPageNum;

    BlockData *blockData = NULL;
    PageData *pageData = NULL;
    for (int blockIndex = 0; blockIndex < m_physicBlock; ++blockIndex) {
        // block meta data
        blockData = m_blockData[blockIndex];
        inStream >> blockTemper >> blockEraseCount >> blockIsFree;
        blockData->setTemper(blockTemper);
        blockData->addEraseCount(blockEraseCount);
        blockData->setIsFree(blockIsFree == 0 ? false : true);

        // page meta data
        for (int pageIndex = 0; pageIndex < BlockData::m_pagePerBlock; ++pageIndex) {
            pageData = blockData->getPage(pageIndex);
            inStream >> pageIsValid >> pageIsFree >> realPageNum;

            pageData->setValid(pageIsValid == 0 ? false : true);
            pageData->setIsFree(pageIsFree == 0 ? false : true);
            pageData->setRealPageNum(realPageNum);
        }
    }
    return true;
}

bool SSD::writeBackMetaData(std::string filename) {
    assert(m_executor);
    std::ofstream outStream;
    outStream.open(filename.c_str(), std::ios_base::out);
    if (!outStream.is_open()) {
        std::cerr << "Fail open meta data file " << filename << std::endl;
        return false;
    }

    BlockData *blockData;
    PageData *pageData;
    for (int blockIndex = 0; blockIndex < m_physicBlock; ++blockIndex) {
        // block meta data
        blockData = m_blockData[blockIndex];
        outStream << blockData->getTemper() << " "
                        << blockData->getEraseCount() << " "
                        << (blockData->getIsFree() ? 1 : 0) << " ";

        // page meta data
        for (int pageIndex = 0; pageIndex < BlockData::m_pagePerBlock; ++pageIndex) {
            pageData = blockData->getPage(pageIndex);

            outStream << (pageData->getValid() ? 1 : 0) << " "
                            << (pageData->getIsFree() ? 1 : 0) << " "
                            << pageData->getRealPageNum() << " ";
        }
    }
    return true;
}



bool SSD::initBlockData(std::string filenamePrefix) {
    assert(m_executor);

    if (m_executor->m_dataFromFile == false) return true;
    // 0, Empty SSD by default
    if (filenamePrefix.empty()) {
        std::cerr << "BlockData filename Prefix is empty !!!" << std::endl;
        return false;
    }

    // 1, read data from dataFile
    std::string filename;
    std::ifstream inStream;
    for (int levelIndex = 0; levelIndex < m_executor->m_levelPerChip; ++levelIndex) {
        // DataFile'name format : prefix_level
        std::ostringstream filenameStream;
        filenameStream << filenamePrefix << "_" << levelIndex;
        filename = filenameStream.str();

        if (PageData::m_isRealData) {
            inStream.open(filename.c_str(), std::ios_base::in | std::ios_base::binary);
        } else {
            inStream.open(filename.c_str(), std::ios_base::in);
        }

        if(loadLevelBlockData(inStream, levelIndex) == false) {
            reset();
            std::cerr << "Fail load block data !!!" << std::endl;
            return false;
        }
        inStream.close();
    }
    return true;
}

bool SSD::writeBackBlockData(std::string filenamePrefix) {
    assert(m_executor);
    // 0, Empty SSD by default
    if (filenamePrefix.empty()) {
        std::cerr << "BlockData filename Prefix is empty !!!" << std::endl;
        return false;
    }

    // 1, read data from dataFile
    std::string filename;
    std::ofstream outStream;
    for (int levelIndex = 0; levelIndex < m_executor->m_levelPerChip; ++levelIndex) {
        // DataFile'name format : prefix_level
        std::ostringstream filenameStream;
        filenameStream << filenamePrefix << "_" << levelIndex;
        filename = filenameStream.str();

        if (PageData::m_isRealData) {
            outStream.open(filename.c_str(), std::ios_base::out | std::ios_base::binary);
        } else {
            outStream.open(filename.c_str(), std::ios_base::in);
        }

        if (writeLevelBlockData(outStream, levelIndex) == false) {
            std::cerr << "Fail write back block data !!!" << std::endl
                      << "Abort" << std::endl;
            exit(1);
        }
        outStream.close();
    }
    return true;
}

bool SSD::writeBackSummary(std::string filename) {

    if (filename.empty()) {
        std::cerr << "Summary file's name is empty !!!" << std::endl;
        return false;
    }

    std::ofstream outStream;
    outStream.open(filename.c_str(), std::ios_base::out);
    if (!outStream.is_open()) {
        std::cerr << "Fail open summary file !!!" << std::endl;
        return false;
    }

    // erase count
    int maxEraseCount = 0;
    double avgEraseCount = 0, maxTemper = 0;

    for (int blockIndex = 0; blockIndex < m_physicBlock; ++blockIndex) {
        if (maxEraseCount < m_blockData[blockIndex]->getEraseCount()) {
                maxEraseCount = m_blockData[blockIndex]->getEraseCount();
        }
        avgEraseCount += m_blockData[blockIndex]->getEraseCount();

        if (maxTemper - m_blockData[blockIndex]->getTemper() < LOWESTGAP) {
            maxTemper = m_blockData[blockIndex]->getTemper();
        }
    }
    avgEraseCount /= m_physicBlock;

    outStream << "maxEraseCount = " << maxEraseCount << "\n"
              << "averageEraseCount = " << avgEraseCount << "\n"
              << "maxTemper = " << maxTemper << "\n";

    outStream.close();
}



bool SSD::loadLevelBlockData(std::ifstream &inStream, int levelIndex) {
    if (inStream.is_open() == false) return false;

    char *dataBit = NULL, *dataBitModel = NULL;
    int dataErrCount = 0;
    if (PageData::m_isRealData) {
        dataBit = new char[PageData::m_bytePerPage],
                dataBitModel = new char[PageData::m_bytePerPage];
        memset(dataBitModel, 0, PageData::m_bytePerPage);
    }

    int startBlock = levelIndex * m_executor->m_blockPerLine * m_executor->m_linePerLevel,
            endBlock = (levelIndex + 1) * m_executor->m_blockPerLine * m_executor->m_linePerLevel;
    PageData tmpPagedata;

    for (int blockIndex = startBlock; blockIndex < endBlock; ++blockIndex) {
        for (int pageIndex = 0; pageIndex < BlockData::m_pagePerBlock; ++pageIndex) {

            if (PageData::m_isRealData) {
                inStream.read(dataBit, PageData::m_bytePerPage);
                if (memcmp(dataBit, dataBitModel, PageData::m_bytePerPage) == 0) continue;
            } else {
                inStream >> dataErrCount;
                if (0 == dataErrCount) continue;
            }

            if (PageData::m_isRealData) tmpPagedata.setDataAsBit(dataBit);
            else tmpPagedata.setDataAsErrCount(dataErrCount);

            m_blockData[blockIndex]->setPageData(pageIndex, tmpPagedata);
        }
    }

    if (PageData::m_isRealData) {
        delete dataBit;
        delete dataBitModel;
    }
    return true;
}

bool SSD::writeLevelBlockData(std::ofstream &outStream, int levelIndex) {
    if (!outStream.is_open() == false) return false;

    char *dataBit = PageData::m_isRealData ? new char[PageData::m_bytePerPage] : NULL;
    int dataErrCount = 0;

    int startBlock = levelIndex * m_executor->m_blockPerLine * m_executor->m_linePerLevel,
            endBlock = (levelIndex + 1) * m_executor->m_blockPerLine * m_executor->m_linePerLevel;

    PageData *pageData = NULL;
    for (int blockIndex = startBlock; blockIndex < endBlock; ++blockIndex) {
        for (int pageIndex = 0; pageIndex < BlockData::m_pagePerBlock; ++pageIndex) {

            pageData = m_blockData[blockIndex]->getPage(pageIndex);
            if (PageData::m_isRealData) {
                pageData->getDataAsBit(dataBit);
                outStream.write(dataBit, PageData::m_bytePerPage);
            } else {
                pageData->getDataAsErrCount(dataErrCount);
                outStream << dataErrCount << " ";
            }
        }
    }
    return true;
}



void SSD::reset() {
    if (m_executor == NULL) return;

    m_executor = NULL;
    for (int blockIndex = 0; blockIndex < m_physicBlock; ++blockIndex) {
        delete m_blockData[blockIndex];
    }
    delete m_blockData;
    delete m_entryTable;
    delete m_replaceTable;
    m_freeBlock.clear();
}

bool SSD::setExecutor(Executor *executor) {
    if (m_executor) {
        std::cerr << "SSD space should reset before new Executor is set !!" << std::endl;
        return false;
    }
    m_executor = executor;
    return true;
}



int SSD::readPage(int logicNum, PageData *data) {
    int phyBlock = -1, phyPage = -1;
    if (data == NULL || logic2physic(logicNum, phyBlock, phyPage) == false) {
        return -1;
    }

//    while (m_blockData[phyBlock]->getTemper() >= 150) {
//        m_executor->disspateTemper();
//    }
    m_blockData[phyBlock]->getPageData(phyPage, *data);

    m_executor->checkTemper(phyBlock, m_executor->m_readPageTemper);
    m_executor->checkTime(m_executor->m_readPageTime);

    return phyBlock;
}

int SSD::writePage(int logicNum, PageData *data) {
    int phyBlock = -1, phyPage = -1;
    if (data == NULL || findfreePage(logicNum, phyBlock, phyPage) == false) {
        return -1;
    }

//    while (m_blockData[phyBlock]->getTemper() >= 150) {
//        m_executor->disspateTemper();
//    }
    m_blockData[phyBlock]->setPageData(phyPage, *data);

    int logicPage = logicNum % BlockData::m_pagePerBlock;
    PageData *pageData = m_blockData[phyBlock]->getPage(phyPage);
    pageData->setIsFree(false);
    pageData->setRealPageNum(logicPage);
    pageData->setValid(true);

    m_executor->checkTemper(phyBlock, m_executor->m_writePageTemper);
    m_executor->checkTime(m_executor->m_writeOobTime +
                                                m_executor->m_writePageTime);

    return phyBlock;
}




int SSD::findfree() {
    assert(m_minimumFreeBlock >=1);
    if (m_freeBlock.size() <= m_minimumFreeBlock) return -1;

    int freeBlock = m_freeBlock.front();
    m_freeBlock.pop_front();
    m_blockData[freeBlock]->setIsFree(false);
    return freeBlock;
}

bool SSD::gc(std::vector<int> &gcChain, int pageNum) {

    // find the longest chain
    std::vector<int> maxChain, tmpChain;
    int entryIndex = 0;
    for (; entryIndex < m_logicBlock; ++entryIndex) {
        int primary = m_entryTable[entryIndex];
        while (primary != -1) {
            tmpChain.push_back(primary);
            primary = m_replaceTable[primary];
        }
        if (tmpChain.size() > maxChain.size()) {
            maxChain = tmpChain;
        }
    }

    // whether the original chain is the longest | will be changed
    bool originalChainChanged = (maxChain.size() == gcChain.size());
    if (originalChainChanged == true) {
        for (int index = 0; index < maxChain.size(); ++index) {
            if (maxChain[index] != gcChain[index]) {
                originalChainChanged = false;
                break;
            }
        }
    }

    // copy all data into one block
    int primary = m_freeBlock.front();
    m_freeBlock.pop_front();
    m_blockData[primary]->setIsFree(false);

    m_entryTable[entryIndex] = primary;
    m_replaceTable[primary] = -1;

    PageData *srcPage = NULL, *targetPage = NULL;
    for (std::vector<int>::iterator iterBlock = maxChain.begin(); iterBlock != maxChain.end(); ++iterBlock) {
        m_replaceTable[*iterBlock] = -1;
        m_blockData[*iterBlock]->setIsFree(true);
        m_blockData[*iterBlock]->addEraseCount(1);
        m_freeBlock.push_back(*iterBlock);


        m_executor->checkTime(m_executor->m_writeOobTime +
                                                m_executor->m_eraseBlockTime);
        m_executor->checkTemper(*iterBlock, m_executor->m_eraseBlockTemper);

        for (int pageIndex = 0; pageIndex < BlockData::m_pagePerBlock; ++pageIndex) {
            srcPage = m_blockData[*iterBlock]->getPage(pageIndex);
            srcPage->setIsFree(true);
            srcPage->setRealPageNum(-1);
            srcPage->setValid(false);

            m_executor->checkTime(m_executor->m_writeOobTime);

            if (originalChainChanged == true && pageIndex == pageNum) {
                srcPage->clearData();
                continue;
            }
            if (srcPage->getValid() == true) {
                targetPage = m_blockData[primary]->getPage(pageIndex);
                targetPage->mvData(srcPage);

                targetPage->setIsFree(false);
                targetPage->setRealPageNum(pageIndex);
                targetPage->setValid(true);

                m_executor->checkTime(m_executor->m_readOobTime +
                                                            m_executor->m_writeOobTime +
                                                            m_executor->m_readPageTime +
                                                            m_executor->m_writePageTime);
                m_executor->checkTemper(*iterBlock, m_executor->m_readPageTemper);
                m_executor->checkTemper(primary, m_executor->m_writePageTemper);
            }
        }
    }

    if (originalChainChanged == true) {
        gcChain.clear();
        gcChain.push_back(primary);
    }

    return originalChainChanged;
}

bool SSD::logic2physic(int logicNum, int &phyBlock, int &phyPage) {
    assert(m_executor);

    if (logicNum < 0 || logicNum >= m_executor->m_pagePerBlock * m_logicBlock) {
        std::cerr << "the LogicNum is illegal !!!" << std::endl;
        return false;
    }

    int logicBlock = logicNum / m_executor->m_pagePerBlock;
    phyPage = logicNum % m_executor->m_pagePerBlock;

    // entryTable
    int primaryBlock = m_entryTable[logicBlock];
    if (primaryBlock == -1) return false;

    while (primaryBlock != -1) {

        m_executor->checkTime(m_executor->m_readOobTime);

        if (m_blockData[primaryBlock]->getPage(phyPage)->getValid() == true) {
            phyBlock = primaryBlock;
            return true;
        }
        primaryBlock = m_replaceTable[primaryBlock];
    }
    return false;
}

bool SSD::findfreePage(int logicNum, int &phyBlock, int &phyPage) {
    assert(m_executor);

    int logicBlock = logicNum / m_executor->m_pagePerBlock;
    phyPage = logicNum % m_executor->m_pagePerBlock;

    int primaryBlock = m_entryTable[logicBlock];

    std::vector<int> gcChain;
    while (primaryBlock != -1) {
        gcChain.push_back(primaryBlock);

        m_executor->checkTime(m_executor->m_readOobTime);

        if (m_blockData[primaryBlock]->getPage(phyPage)->getIsFree() == true) {
            phyBlock = primaryBlock;
            return true;
        }
        primaryBlock = m_replaceTable[primaryBlock];
    }

    // the chain has been changed
    if ((primaryBlock = findfree()) == -1 && gc(gcChain, phyPage) == true) {
        phyBlock = gcChain.back();
        return true;
    }

    assert((primaryBlock = findfree()) != -1);

    if (gcChain.size() == 0) {
        m_entryTable[logicBlock] = primaryBlock;
        m_replaceTable[primaryBlock] = -1;
    }
    else {
        m_replaceTable[gcChain.back()] = primaryBlock;
        m_replaceTable[primaryBlock] = -1;
    }

    phyBlock = primaryBlock;
    return true;
}



bool SSD::linear2threeDimesion(int linear, int &x, int &y, int &z) {
    assert(m_executor);

    if (linear < 0 || linear >= m_physicBlock) {
        std::cerr << "Linear is illegal !!! " << std::endl;
        return false;
    }

    x = linear % m_executor->m_blockPerLine;
    y = (linear / m_executor->m_blockPerLine) % m_executor->m_linePerLevel;
    z = linear / m_executor->m_blockPerLine / m_executor->m_linePerLevel;
    return true;
}

bool SSD::threeDimension2linear(int x, int y, int z, int &linear) {
    assert(m_executor);
    if (x < 0 || x >= m_executor->m_blockPerLine) {
        std::cerr << "The \"x\" index is illegal" << std::endl;
        return false;
    }
    if (y < 0 || y >= m_executor->m_linePerLevel) {
        std::cerr << "The \"y\" index is illegal" << std::endl;
        return false;
    }
    if (z < 0 || z >= m_executor->m_levelPerChip) {
        std::cerr << "The \"z\" index is illegal" << std::endl;
        return false;
    }

    linear = z * m_executor->m_blockPerLine * m_executor->m_linePerLevel +
            y * m_executor->m_blockPerLine + x;
    return true;
}


// ======================SSD_1======================

SSD_1::SSD_1() {

}

SSD_1::~SSD_1() {
}


const static int JUMPGAP = 5;
bool SSD_1::initFreeBlockList(std::string filename) {
    assert(m_executor);

    if (m_executor->m_dataFromFile == false) {
        int xIter = 0, yIter = 0, zIter = 0;
        int freeBlock = 0;
        do {
            do {
                do {
                    assert(threeDimension2linear(xIter, yIter, zIter, freeBlock) == true);
                    m_freeBlock.push_back(freeBlock);
                    zIter = (zIter + JUMPGAP) % m_executor->m_levelPerChip;
                } while(zIter !=0);
                zIter = 0;
                xIter = (xIter + JUMPGAP) % m_executor->m_blockPerLine;
            } while (xIter != 0);
            xIter = 0;
            yIter = (yIter + JUMPGAP) % m_executor->m_linePerLevel;
        } while (yIter != 0);
        return true;
    }

    if (filename.empty() == true) return false;
    // read from file
    std::ifstream inStream;
    inStream.open(filename.c_str(), std::ios_base::in);
    if (inStream.is_open() == false) {
        std::cerr << "Fail open freeBlockList File " << filename << std::endl;
        return false;
    }

    int freeBlock;
    while (inStream.eof() == false) {
        inStream >> freeBlock;
        m_freeBlock.push_back(freeBlock);
    }
    inStream.close();
    return true;
}

#include <algorithm>
const static int NUMCMP = 10;
int SSD_1::findfree() {

    if (m_freeBlock.size() < m_minimumFreeBlock) return -1;

//    std::vector<double> temperArr;

//    // sort temper
//    for (std::list<int>::iterator iterFreeBlock = m_freeBlock.begin();
//         iterFreeBlock != m_freeBlock.end() && temperArr.size() <= NUMCMP;
//         ++iterFreeBlock) {
//                temperArr.push_back(m_blockData[*iterFreeBlock]->getTemper());
//    }
//    std::sort(temperArr.begin(), temperArr.end());

//    // find minimum temper block
//    double minimumTemper = *(temperArr.begin());
//    int count = 0, freeBlock = 0;
//    for (std::list<int>::iterator iterFreeBlock = m_freeBlock.begin(); iterFreeBlock != m_freeBlock.end();) {
//        ++count;
//        if ( minimumTemper == m_blockData[*iterFreeBlock]->getTemper()) {
//            freeBlock = *iterFreeBlock;
//            iterFreeBlock = m_freeBlock.erase(iterFreeBlock);
//            break;
//        } else {
//            ++iterFreeBlock;
//        }
//    }

//    assert(count <= 11);

//    m_blockData[freeBlock]->setIsFree(false);

    int freeBlock = m_freeBlock.front();
    double minimumTemper = m_blockData[freeBlock]->getTemper();

    for (std::list<int>::iterator iterBlock = m_freeBlock.begin();
         iterBlock != m_freeBlock.end(); ++iterBlock) {
        if (minimumTemper - m_blockData[*iterBlock]->getTemper() > LOWESTGAP) {
            freeBlock = *iterBlock;
            minimumTemper = m_blockData[freeBlock]->getTemper();
        }
    }

    for (std::list<int>::iterator iterBlock = m_freeBlock.begin();
         iterBlock != m_freeBlock.end(); ++iterBlock) {
        if (freeBlock == *iterBlock) {
            m_freeBlock.erase(iterBlock);
            break;
        }
    }

    m_blockData[freeBlock]->setIsFree(false);
    return freeBlock;
}


bool SSD_1::gc(std::vector<int> &gcChain, int logicNum) {

    std::cout << "gc" << std::endl;
    std::vector<int> chain;
    bool isGcChain = true;

    static int logicBlock = 0;
    if (gcChain.size() < 3) {

        int oldLogicBlock = logicBlock;
        for (int numBlock = 3; numBlock > 1; --numBlock) {
            for (logicBlock = (logicBlock + 1) % m_logicBlock;
                 logicBlock != oldLogicBlock;
                 logicBlock = (logicBlock + 1) % m_logicBlock) {
                    std::vector<int> eraseChain;
                    int phyBlock = m_entryTable[logicBlock];
                    while (phyBlock != -1) {
                        eraseChain.push_back(phyBlock);
                        phyBlock = m_replaceTable[phyBlock];
                    }
                    if (eraseChain.size() == numBlock) {
                        chain = eraseChain;
                        break;
                    }
            }
            if (chain.size() != 0) break;
        }

        assert(chain.size() > 1);

        isGcChain = (chain.size() == gcChain.size());
        for (int blockIndex = 0; blockIndex < chain.size() && isGcChain; ++blockIndex) {
            isGcChain = (chain[blockIndex] == gcChain[blockIndex]);
        }
    }

    std::vector<int> targetChain, srcChain;
    if (isGcChain == false || gcChain.size() < 3) {
        int primaryBlock = m_freeBlock.front();
        m_freeBlock.pop_front();
        m_blockData[primaryBlock]->setIsFree(false);

        targetChain.push_back(primaryBlock);
        srcChain = chain;

    } else { // gcChain.size() == 3
       int bufferBlock = gcChain[2], numInvalid = 0;
       for (int pageIndex = 0; pageIndex < BlockData::m_pagePerBlock; ++pageIndex) {
           if (m_blockData[bufferBlock]->getPage(pageIndex)->getIsFree() == false &&
                   m_blockData[bufferBlock]->getPage(pageIndex)->getValid() == false) ++numInvalid;
       }
       // transfer buffer to primary
       if (numInvalid <= BlockData::m_pagePerBlock - numInvalid) {
           targetChain.push_back(bufferBlock);
           gcChain.pop_back();
           srcChain = gcChain;
           gcChain.clear();
           gcChain = targetChain;
       } else { // not transfer
           int primaryBlock = m_freeBlock.front();
           m_freeBlock.pop_front();
           m_blockData[primaryBlock]->setIsFree(false);

           targetChain.push_back(primaryBlock);
           srcChain = gcChain;
           gcChain.clear();
           gcChain = targetChain;
       }
    }

    // modify table
    if (isGcChain == false) {
        m_entryTable[logicBlock] = targetChain.back();
    }
    for (std::vector<int>::iterator iterBlock = srcChain.begin(); iterBlock != srcChain.end(); ++iterBlock) {
        m_replaceTable[*iterBlock] = -1;

        m_blockData[*iterBlock]->setIsFree(true);
        m_blockData[*iterBlock]->addEraseCount(1);
        m_freeBlock.push_back(*iterBlock);

        m_executor->checkTime(m_executor->m_eraseBlockTime +
                                                m_executor->m_writeOobTime);
        m_executor->checkTemper(*iterBlock, m_executor->m_eraseBlockTemper);
    }
    m_replaceTable[targetChain.back()] = -1;

    // mv data
    PageData *srcPage = NULL, *targetPage = NULL;
    for (int pageIndex = 0; pageIndex < BlockData::m_pagePerBlock; ++pageIndex) {
        targetPage = m_blockData[targetChain.back()]->getPage(pageIndex);
        for (std::vector<int>::iterator iterSrcBlock = srcChain.begin(); iterSrcBlock !=srcChain.end(); ++iterSrcBlock) {
                srcPage = m_blockData[*iterSrcBlock]->getPage(pageIndex);

                if (srcPage->getValid() == true) {
                    targetPage->mvData(srcPage);

                    targetPage->setIsFree(false);
                    targetPage->setRealPageNum(srcPage->getRealPageNum());
                    targetPage->setValid(true);

                    m_executor->checkTime(m_executor->m_writeOobTime +
                                                            m_executor->m_readPageTime +
                                                            m_executor->m_writePageTime);
                    m_executor->checkTemper(targetChain.back(), m_executor->m_writePageTemper);
                    m_executor->checkTemper(*iterSrcBlock, m_executor->m_readPageTemper);
                } else {
                    srcPage->clearData();
                }

                srcPage->setIsFree(true);
                srcPage->setRealPageNum(-1);
                srcPage->setValid(false);

                m_executor->checkTime(m_executor->m_writeOobTime +
                                                        m_executor->m_readOobTime);
        }
    }

    return isGcChain;

}

bool SSD_1::logic2physic(int logicNum, int &phyBlock, int &phyPage) {
    assert(m_executor);

    if (logicNum < 0 || logicNum >= m_executor->m_pagePerBlock * m_logicBlock) {
        std::cerr << "the LogicNum is illegal !!!" << std::endl;
        return false;
    }

    int logicBlock = logicNum / m_executor->m_pagePerBlock;
    phyPage = logicNum % m_executor->m_pagePerBlock;

    // 0, primary block, entryTable
    int primaryBlock = m_entryTable[logicBlock];
    if (primaryBlock == -1) return false;

    m_executor->checkTime(m_executor->m_readOobTime);

    if (m_blockData[primaryBlock]->getPage(phyPage)->getValid() == true) {
        phyBlock = primaryBlock;
        return true;
    }

    // 1, replace block, replaceTable
    int replaceBlock = m_replaceTable[primaryBlock];
    if (replaceBlock == -1) return false;

    for (int pageIndex = BlockData::m_pagePerBlock - 1; pageIndex >= 0; --pageIndex) {

        m_executor->checkTime(m_executor->m_readOobTime);

        if (m_blockData[replaceBlock]->getPage(pageIndex)->getValid() == true &&
                m_blockData[replaceBlock]->getPage(pageIndex)->getRealPageNum() == phyPage) {
            phyPage = pageIndex;
            phyBlock = replaceBlock;
            return true;
        }
    }

    // 2, buffer block, replaceTable
    int bufferBlock = m_replaceTable[replaceBlock];
    if (bufferBlock == -1) return false;

    m_executor->checkTime(m_executor->m_readOobTime);

    if (m_blockData[bufferBlock]->getPage(phyPage)->getValid() == true) {
        phyBlock = bufferBlock;
        return true;
    }
    return false;
}

bool SSD_1::findfreePage(int logicNum, int &phyBlock, int &phyPage) {
    assert(m_executor);

    int logicBlock = logicNum / BlockData::m_pagePerBlock,
            logicPage = logicNum % BlockData::m_pagePerBlock;

    std::vector<int> gcChain;
    PageData *pageData = NULL;

    // primaryBlock, entryTable
    int primaryBlock = m_entryTable[logicBlock];
    if (primaryBlock == -1) {
        primaryBlock = findfree();
        if (primaryBlock == -1) {
            gc(gcChain, logicPage);
            primaryBlock = findfree();
        }

        m_entryTable[logicBlock] = primaryBlock;
        m_replaceTable[primaryBlock] = -1;
    }
    gcChain.push_back(primaryBlock);
    pageData = m_blockData[primaryBlock]->getPage(logicPage);

    m_executor->checkTime(m_executor->m_readOobTime);

    if (pageData->getIsFree() == true) {
        phyBlock = primaryBlock;
        phyPage = logicPage;
        return true;
    }
    m_executor->checkTime(m_executor->m_writeOobTime);
    pageData->setIsFree(false);
    pageData->setRealPageNum(-1);
    pageData->setValid(false);

    // replaceBlock, replaceTable
    int replaceBlock = m_replaceTable[primaryBlock];
    if (replaceBlock == -1) {
        replaceBlock = findfree();
        if (replaceBlock == -1) {
            gc(gcChain, logicPage);
            replaceBlock = findfree();
        }

        m_replaceTable[primaryBlock] = replaceBlock;
        m_replaceTable[replaceBlock] = -1;
    }
    gcChain.push_back(replaceBlock);
    for (int pageIndex = 0; pageIndex < BlockData::m_pagePerBlock; ++pageIndex) {
        pageData = m_blockData[replaceBlock]->getPage(pageIndex);

        m_executor->checkTime(m_executor->m_readOobTime);

        if (pageData->getRealPageNum() == logicPage && pageData->getValid() == true) {
            pageData->setIsFree(false);
            pageData->setValid(false);
            pageData->setRealPageNum(-1);

            m_executor->checkTime(m_executor->m_writeOobTime);
        }
        if (pageData->getIsFree() == true) {
            phyBlock = replaceBlock;
            phyPage = pageIndex;
            return true;
        }
    }

    // bufferBlock, replaceTable
    int bufferBlock = m_replaceTable[replaceBlock];
    if (bufferBlock == -1) {
        bufferBlock = findfree();
        if (bufferBlock == -1) {
            if (gc(gcChain, logicPage) == true) { // new chain
                bufferBlock = gcChain.back();
                m_entryTable[logicBlock] = bufferBlock;
                m_replaceTable[bufferBlock] = -1;
            } else { // append bufferBlock
                bufferBlock = findfree();
                m_replaceTable[replaceBlock] = bufferBlock;
                m_replaceTable[bufferBlock] = -1;
                gcChain.push_back(bufferBlock);
            }
        }
    }
    pageData = m_blockData[bufferBlock]->getPage(logicPage);

    if (pageData->getIsFree() == true) {
        phyBlock = bufferBlock;
        phyPage = logicPage;
        return true;
    }
    pageData->setIsFree(false);
    pageData->setRealPageNum(-1);
    pageData->setValid(false);

    m_executor->checkTime(m_executor->m_writeOobTime);

    gc(gcChain, logicPage);
    primaryBlock = gcChain.back();
    m_entryTable[logicBlock] = primaryBlock;
    m_replaceTable[primaryBlock] = -1;

    phyBlock = primaryBlock;
    phyPage = logicPage;
    assert(phyBlock != -1);
    return true;
}


// ====================SSD_2=========================
SSD_2::SSD_2() {
}

SSD_2::~SSD_2() {
}

bool SSD_2::initFreeBlockList(std::string filename) {
    assert(m_executor);

    if (m_executor->m_dataFromFile == false) {
        for (int index = 0; index < m_physicBlock; ++index) {
            m_freeBlock.push_back(index);
        }
        return true;
    }

    if (filename.empty()) return false;

    std::ifstream inStream;
    inStream.open(filename.c_str(), std::ios_base::in);
    if (inStream.is_open() == false) {
        std::cerr << "Fail open freeBlockList File " << filename << std::endl;
        return false;
    }

    int freeBlock;
    while (inStream.eof() == false) {
        inStream >> freeBlock;
        m_freeBlock.push_back(freeBlock);
    }
    inStream.close();
    return true;
}

int SSD_2::findfree() {

    if (m_freeBlock.size() < m_minimumFreeBlock) return -1;

    std::default_random_engine generator(time(nullptr));
    std::uniform_int_distribution<int> distribution(0, m_freeBlock.size()-1);

    int index = distribution(generator);

    std::list<int>::iterator iterFreeBlock = m_freeBlock.begin();
    int freeBlock = 0;
    for (; index > 0; ++iterFreeBlock, --index) ;
    freeBlock = *iterFreeBlock;
    m_freeBlock.erase(iterFreeBlock);
    m_blockData[freeBlock]->setIsFree(false);
    return freeBlock;
}


// ====================Executor=========================

Executor::Executor() : m_ssd(NULL) {

    // structure of SSD
    m_bytePerPage =
            m_pagePerBlock =
            m_blockPerLine =
            m_linePerLevel =
            m_levelPerChip =
            m_minimumFreeBlock = 0;

    m_logicBlock =
        m_physicBlock = 0;

    m_isRealData =
            m_dataFromFile = false;

    // time variation
    m_readOobTime =
            m_writeOobTime =
            m_readPageTime =
            m_writePageTime =
            m_eraseBlockTime = 0;

    // temperature dissipation percentage
    m_lrTemperPercent =
            m_fbTemperPercent =
            m_udTemperPercent = 0;

    // temperature raise
    m_readPageTemper =
            m_writePageTemper =
            m_eraseBlockTemper = 0;

    // time interval
    m_scanTemperInterval =
            m_dissTemperInterval =
            m_createErrInterval =
            m_calErrInterval = 0;

    // time ticker
    m_dissipateTemperTime =
            m_statisticTemperTime =
            m_injectErrTime =
            m_statisticErrTime = 0;
}

Executor::~Executor() {
    m_ssd = NULL;
}



bool Executor::readConfigFile(std::string configFileName) {
    std::ifstream inStream;
    inStream.open(configFileName.c_str(), std::ios_base::in);

    if (!inStream.is_open()) {
        std::cerr << "ConfigureFile Fail open " << configFileName << " !!!" << std::endl;
        return false;
    }

    // structure of SSD
    inStream >> m_bytePerPage
            >> m_pagePerBlock
            >> m_blockPerLine
            >> m_linePerLevel
            >> m_levelPerChip
            >> m_minimumFreeBlock;

    m_physicBlock = m_blockPerLine * m_linePerLevel * m_levelPerChip;
    double logicPercent;
    inStream >> logicPercent;
    m_logicBlock = static_cast<int> (m_physicBlock * logicPercent);

    int isRealData = 0;
    inStream >> isRealData;
    m_isRealData = isRealData == 0 ? false : true;
    int dataFromFile = 0;
    inStream >> dataFromFile;
    m_dataFromFile = dataFromFile == 0 ? false :true;

    // time variation
    inStream >> m_readOobTime
            >> m_writeOobTime
            >> m_readPageTime
            >> m_writePageTime
            >> m_eraseBlockTime;

    // temperature dissipation percentage
    inStream >> m_lrTemperPercent
            >> m_fbTemperPercent
            >> m_udTemperPercent;

    // temperature raise
    inStream >> m_readPageTemper
            >> m_writePageTemper
            >> m_eraseBlockTemper;

    // time interval
    inStream >> m_scanTemperInterval
            >> m_dissTemperInterval
            >> m_createErrInterval
            >> m_calErrInterval;

    // error ratio
    int temperStage, errCount;
    double tmpTemper;
    inStream >> temperStage;
    for (;temperStage > 0; --temperStage) {
        inStream >> tmpTemper >> errCount;
        m_temper2numBlockErr[tmpTemper] = errCount;
    }
    inStream >> m_eccNumBit;


    // input filename
    // trace file list
    int numTraceFile;
    inStream >> numTraceFile;
    std::string traceFilename;
    for (; numTraceFile > 0; --numTraceFile) {
        inStream >> traceFilename;
        m_traceFileNameList.push_back(traceFilename);
    }

    inStream >> m_entryTableFile
            >> m_replaceTableFile
            >> m_dataFilePrefix
            >> m_metaDataFile
            >> m_freeBlockListFile;

    // output filename
    inStream >> m_temperShotPrefix
            >> m_errorFilename
            >> m_summaryFilename;

    inStream.close();
    return true;
}

bool Executor::initData() {
    if (m_ssd == NULL) {
        std::cerr << "SSD should be configure first !!" << std::endl;
        return false;
    }

    m_ssd->initTable(m_entryTableFile, "entryTable");
    m_ssd->initTable(m_replaceTableFile, "replaceTable");
    m_ssd->initFreeBlockList(m_freeBlockListFile);
    m_ssd->initMetaData(m_metaDataFile);
    m_ssd->initBlockData(m_dataFilePrefix);

    return true;
}



void Executor::setSSD(SSD *ssd) {
    m_ssd = ssd;
}



bool Executor::beginProcess() {
    std::ifstream inStream;

    for (int traceIndex = 0; traceIndex < m_traceFileNameList.size(); ++traceIndex) {
        inStream.open(m_traceFileNameList[traceIndex].c_str(), std::ios_base::in);
        if (inStream.is_open() == false) {
            std::cerr << "Fail to open tracefile" << m_traceFileNameList[traceIndex] <<std::endl;
            continue;
        }
        if (processRequest(inStream) == false) {
            std::cerr << "Fail to process tracefile " << m_traceFileNameList[traceIndex] << std::endl;
            inStream.close();
            exit(1);
        }
        inStream.close();
    }
    return true;
}

bool Executor::endProcess() {
    m_ssd->writeBackTable(m_entryTableFile, "entryTable");
    m_ssd->writeBackTable(m_replaceTableFile, "replaceTable");
    m_ssd->writeBackFreeBlockList(m_freeBlockListFile);
    m_ssd->writeBackMetaData(m_metaDataFile);
    m_ssd->writeBackBlockData(m_dataFilePrefix);
    m_ssd->writeBackSummary(m_summaryFilename);
}



void Executor::checkTime(double time) {
    assert(m_ssd);

    m_dissipateTemperTime += time;
    m_statisticTemperTime += time;
    m_injectErrTime += time;
    m_statisticErrTime += time;

    if (m_injectErrTime - m_createErrInterval > LOWESTGAP) {
        injectErr();
        m_injectErrTime = 0;
    }

    if (m_statisticErrTime - m_calErrInterval > LOWESTGAP) {
        statisticErr();
        m_statisticErrTime = 0;
    }

    if (m_dissipateTemperTime - m_dissTemperInterval > LOWESTGAP) {
        disspateTemper();
        m_dissipateTemperTime = 0;
    }

    if (m_statisticTemperTime - m_scanTemperInterval > LOWESTGAP) {
        statisticTemper();
        m_statisticTemperTime = 0;
    }
}

void Executor::checkTemper(int blockIndex, double raiseTemper) {
    double blockTemper = 0;
    blockTemper = m_ssd->m_blockData[blockIndex]->getTemper();
    blockTemper += raiseTemper;
    m_ssd->m_blockData[blockIndex]->setTemper(blockTemper);
}




void Executor::disspateTemper() {

    int midBlockPos = m_blockPerLine / 2,
            midLinePos = m_linePerLevel / 2,
            midLevelPos = m_levelPerChip / 2;

    int preIndex = 0, nextIndex = 0,
            phyBlock = 0, neighborPre = 0, neighborNext = 0;

    // level-exchange heat
    for (preIndex = midLevelPos - 1, nextIndex = midLevelPos;
         preIndex >= 0 ; --preIndex, ++nextIndex) {
        for (int lineIndex = 0; lineIndex < m_linePerLevel; ++lineIndex) {
            for (int blockIndex = 0; blockIndex < m_blockPerLine; ++blockIndex) {
                // up partial
                if (preIndex > 0) m_ssd->threeDimension2linear(blockIndex, lineIndex, preIndex -1, neighborPre);
                else neighborPre = -1;
                m_ssd->threeDimension2linear(blockIndex, lineIndex, preIndex, phyBlock);
                m_ssd->threeDimension2linear(blockIndex, lineIndex, preIndex + 1, neighborNext);
                this->releaseTemper(neighborPre, phyBlock, neighborNext, m_udTemperPercent);

                // down partial
                m_ssd->threeDimension2linear(blockIndex, lineIndex, nextIndex - 1, neighborPre);
                m_ssd->threeDimension2linear(blockIndex, lineIndex, nextIndex, phyBlock);
                if (nextIndex < m_levelPerChip - 1) m_ssd->threeDimension2linear(blockIndex, lineIndex, nextIndex + 1, neighborNext);
                else neighborNext = -1;
                this->releaseTemper(neighborPre, phyBlock, neighborNext, m_udTemperPercent);
            }
        }
    }

    // line-exchange hear
    for (int levelIndex = 0; levelIndex < m_levelPerChip; ++levelIndex) {
        for (preIndex = midLinePos - 1, nextIndex = midLinePos; preIndex >= 0;
             --preIndex, ++nextIndex) {
            for (int blockIndex = 0; blockIndex < m_blockPerLine; ++blockIndex) {
                // front partial
                if (preIndex > 0) m_ssd->threeDimension2linear(blockIndex, preIndex -1, levelIndex, neighborPre);
                else neighborPre = -1;
                m_ssd->threeDimension2linear(blockIndex, preIndex, levelIndex, phyBlock);
                m_ssd->threeDimension2linear(blockIndex, preIndex + 1, levelIndex, neighborNext);
                this->releaseTemper(neighborPre, phyBlock, neighborNext, m_fbTemperPercent);

                // back parti
                m_ssd->threeDimension2linear(blockIndex, nextIndex -1, levelIndex, neighborPre);
                m_ssd->threeDimension2linear(blockIndex, nextIndex, levelIndex, phyBlock);
                if (nextIndex < m_linePerLevel - 1) m_ssd->threeDimension2linear(blockIndex, nextIndex + 1, levelIndex, neighborNext);
                else neighborNext = -1;
                this->releaseTemper(neighborPre, phyBlock, neighborNext, m_fbTemperPercent);
            }
        }
    }

    // block-exchange heat
    for (int levelIndex = 0; levelIndex < m_levelPerChip; ++levelIndex) {
        for (int lineIndex = 0; lineIndex < m_linePerLevel; ++lineIndex) {
            for (preIndex = midBlockPos - 1, nextIndex = midBlockPos;
                 preIndex >= 0; --preIndex, ++nextIndex) {

                // left partial
                if (preIndex > 0) m_ssd->threeDimension2linear(preIndex -1, lineIndex, levelIndex, neighborPre);
                else neighborPre = -1;
                m_ssd->threeDimension2linear(preIndex, lineIndex, levelIndex, phyBlock);
                m_ssd->threeDimension2linear(preIndex + 1, lineIndex, levelIndex, neighborNext);
                this->releaseTemper(neighborPre, phyBlock, neighborNext, m_lrTemperPercent);

                // right partial
                m_ssd->threeDimension2linear(nextIndex - 1, lineIndex, levelIndex, neighborPre);
                m_ssd->threeDimension2linear(nextIndex, lineIndex, levelIndex, phyBlock);
                if (nextIndex < m_blockPerLine -1) m_ssd->threeDimension2linear(nextIndex + 1, lineIndex, levelIndex, neighborNext);
                else neighborNext = -1;
                this->releaseTemper(neighborPre, phyBlock, neighborNext, m_lrTemperPercent);

            }
        }
    }
}

void Executor::releaseTemper(int neighborPre, int phyBlock, int neighborNext,
                             double percent) {
    double preTemper = 0, phyTemper = 0, nextTemper = 0;

    if (neighborPre == -1) preTemper = 25;
    else preTemper = m_ssd->m_blockData[neighborPre]->getTemper();

    phyTemper = m_ssd->m_blockData[phyBlock]->getTemper();

    if (neighborNext == -1) nextTemper = 25;
    else nextTemper = m_ssd->m_blockData[neighborNext]->getTemper();


    double temperArr[3] = {preTemper, phyTemper, nextTemper};
    int blockArr[3] = {neighborPre, phyBlock, neighborNext};

    double tmpTemper;
    int tmpBlock;
    if (temperArr[0] > temperArr[1]) {
        tmpTemper = temperArr[1];
        temperArr[1] = temperArr[0];
        temperArr[0] = tmpTemper;

        tmpBlock = blockArr[1];
        blockArr[1] = blockArr[0];
        blockArr[0] = tmpBlock;
    }
    if (temperArr[2] < temperArr[0]) {
        tmpTemper = temperArr[2];
        temperArr[2] = temperArr[0];
        temperArr[0] = tmpTemper;

        tmpBlock = blockArr[2];
        blockArr[2] = blockArr[0];
        blockArr[0] = tmpBlock;
    } else if (temperArr[2] < temperArr[1]){
        tmpTemper = temperArr[1];
        temperArr[1] = temperArr[2];
        temperArr[2] = tmpTemper;

        tmpBlock = blockArr[1];
        blockArr[1] = blockArr[2];
        blockArr[2] = tmpBlock;
    }

    double maxMidGap = (temperArr[2] - temperArr[1]) * percent,
            midMinGap = (temperArr[1] - temperArr[0]) * percent;
    temperArr[2] -= maxMidGap;
    temperArr[1] += maxMidGap;
    temperArr[1] -= midMinGap;
    temperArr[0] += midMinGap;

    for (int index = 0; index < 3; ++index) {
        if (blockArr[index] != -1) {
            m_ssd->m_blockData[blockArr[index]]->setTemper(temperArr[index]);
        }
    }
}

void Executor::statisticTemper() {
    static int ShotCount = 0;

    std::string filename; // shotTemperPrefix_times_level
    std::ofstream outStream;
    for (int levelIndex = 0; levelIndex < m_levelPerChip; ++levelIndex) {
        std::ostringstream filenameStream;
        filenameStream << m_temperShotPrefix << "_" << ShotCount << "_"<<levelIndex;
        filename = filenameStream.str();

        outStream.open(filename.c_str(), std::ios_base::out);
        int phyBlockIndex = 0;
        for (int lineIndex = 0; lineIndex < m_linePerLevel ; ++lineIndex) {
            for (int blockIndex = 0; blockIndex < m_blockPerLine; ++blockIndex) {
                m_ssd->threeDimension2linear(blockIndex, lineIndex, levelIndex, phyBlockIndex);
                outStream /*<< lineIndex << " "
                          << blockIndex << " "*/
                          << m_ssd->m_blockData[phyBlockIndex]->getTemper() << "\n";
            }
            outStream << "\n";
        }
        outStream.close();
    }
    ShotCount += 1;

}



void Executor::injectErr() {
    assert(m_temper2numBlockErr.size() > 0);

    for (int blockIndex = 0; blockIndex < m_physicBlock; ++blockIndex) {

        double blockTemper = m_ssd->m_blockData[blockIndex]->getTemper(),
                temperIndex= 0;
        std::map<double, int>::iterator iterMap = m_temper2numBlockErr.begin();
        for (; iterMap != m_temper2numBlockErr.end() &&
             blockTemper - iterMap->first >= LOWESTGAP; ++iterMap) {
                temperIndex = iterMap->first;
        }
        if (iterMap == m_temper2numBlockErr.end()) {
            --iterMap;
            temperIndex = iterMap->first;
        }
        createErrForBlock(blockIndex, m_temper2numBlockErr[temperIndex]);
    }
}

void Executor::createErrForBlock(int blockIndex, int errCount) {

    if (m_isRealData) {
        char *pageData = new char[m_bytePerPage];
        for (int errIndex = 0; errIndex < errCount; ++errIndex) {
            // generate byte index
            std::default_random_engine generatorByte(time(nullptr));
            std::uniform_int_distribution<int> distributionByte(0, m_bytePerPage * m_pagePerBlock - 1);
            int byteIndexGlobal = distributionByte(generatorByte),
                    byteIndex = byteIndexGlobal % m_bytePerPage,
                    pageIndex = byteIndexGlobal / m_bytePerPage;
            // generate bit index
            std::default_random_engine generatorBit(time(nullptr));
            std::uniform_int_distribution<int> distributionBit(0, 7);
            int bitIndex = distributionBit(generatorBit);

            m_ssd->m_blockData[blockIndex]->getPage(pageIndex)->getDataAsBit(pageData);
            pageData[byteIndex] ^= (1 << bitIndex);
            m_ssd->m_blockData[blockIndex]->getPage(pageIndex)->setDataAsBit(pageData);
        }
        delete pageData;
    } else {
        for (int errIndex = 0; errIndex < errCount; ++errIndex) {

            // generate page index
            std::default_random_engine generatorPage(time(nullptr));
            std::uniform_int_distribution<int> distributionPage(0, m_pagePerBlock - 1);

            int pageIndex = distributionPage(generatorPage),
                    numPageErr = 0;

            m_ssd->m_blockData[blockIndex]->getPage(pageIndex)->getDataAsErrCount(numPageErr);
            if (numPageErr + 1 > m_bytePerPage * 8) continue;
            numPageErr += 1;
            m_ssd->m_blockData[blockIndex]->getPage(pageIndex)->setDataAsErrCount(numPageErr);
        }
    }
}

void Executor::statisticErr() {

    int errAmount = 0;
    char byte = 0, bitModel = 1, *pageDataBit = m_isRealData ? new char[m_bytePerPage] : NULL;

    int errCount = 0;
    for (int blockIndex = 0; blockIndex < m_physicBlock; ++blockIndex) {
        for (int pageIndex = 0; pageIndex < m_pagePerBlock; ++pageIndex) {

            errCount = 0;
            if (m_isRealData) {
                m_ssd->m_blockData[blockIndex]->getPage(pageIndex)->getDataAsBit(pageDataBit);
                for (int byteIndex = 0; byteIndex < m_bytePerPage; ++byteIndex) {
                    byte = pageDataBit[byteIndex];
                    bitModel = 1;
                    for (int bitIndex = 0; bitIndex < 8; ++bitIndex) {
                        bitModel <<= 1;
                        if (byte & bitModel) errCount += 1;
                    }
                }
                if (errCount > m_eccNumBit) errAmount += errCount;
            } else {
                m_ssd->m_blockData[blockIndex]->getPage(pageIndex)->getDataAsErrCount(errCount);
                if (errCount > m_eccNumBit) errAmount += errCount;
            }
        }
    }
    if (pageDataBit) delete pageDataBit;

    std::ofstream outStream;
    outStream.open(m_errorFilename.c_str(), std::ios_base::app);
    outStream << errAmount << "\n";
    outStream.close();
}



bool Executor::processRequest(std::ifstream &inStream) {

    static int requestCount = 0;

    static char *dataModel = new char[m_bytePerPage];
    static int errCount = 0;

    int logicNum;
    std::string cmd;

    PageData pageData;
    for (int lineIndex = 0; inStream.eof() == false; ++lineIndex) {
        if (m_isRealData == true) {
            memset(dataModel, 0, m_bytePerPage);
            pageData.setDataAsBit(dataModel);
        } else {
            errCount = 0;
            pageData.setDataAsErrCount(errCount);
        }

        inStream >> logicNum >> cmd;
        if (cmd == std::string("r")) {
            m_ssd->readPage(logicNum, &pageData);
            std::cout <<requestCount++ <<" r " << logicNum << std::endl;
        } else if (cmd == std::string("w")) {
            m_ssd->writePage(logicNum, &pageData);
            std::cout << requestCount++ << " w " << logicNum << std::endl;
        } else {
            std::cerr << "Fail to process request line " << lineIndex << std::endl;
            std::cout << "w " << logicNum << std::endl;
            return false;
        }
    }
    return true;
}
