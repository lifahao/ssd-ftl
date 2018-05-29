#include <cassert>
#include <cstring>
#include <ctime>
#include <stdlib.h>

#include <fstream>
#include <iostream>
#include <list>
#include <map>
#include <random>
#include <sstream>
#include <string>
#include <vector>


class Executor;
// ============== PageData ====================

class PageData {
public:
    static bool m_isRealData; //= false;
    static int m_bytePerPage; // = 4096;

public:

    explicit PageData();
    ~PageData();

    void clearData();
    void copyData(const PageData *source);
    void mvData(PageData *source);

    bool getDataAsBit(char *realPageData);
    bool getDataAsErrCount(int &notRealData);
    bool setDataAsBit(const char *realPageData);
    bool setDataAsErrCount(int notRealData);

    void setValid(bool isValid);
    bool getValid();

    void setIsFree(bool isFree);
    bool getIsFree();

    bool setRealPageNum(int pageNum);
    int getRealPageNum();
private:
    char *m_realPageData;
    int m_notRealPageData; // only store amount of error per page

    bool m_isValid;
    bool m_isFree;
    int m_realPageNum; // only valid when replaceBlock method used
};

// ============== BlockData ====================
class BlockData {
public:
    static int m_pagePerBlock; // = 0;

public:
    explicit BlockData();
    ~BlockData();

    void clearData();
    void copyData(BlockData *blockdata);
    void mvData(BlockData *blockdata);

    PageData *getPage(int pageNum);
    bool getPageData(int offset, PageData &pagedata);
    bool setPageData(int offset, const PageData &pagedata);

    void setTemper(double temper);
    double getTemper();

    bool addEraseCount(int eraseCount);
    int getEraseCount();

    void setIsFree(bool isFree);
    bool getIsFree();
private:
    PageData **m_pageData;

    double m_temper;
    int m_eraseCount;
    bool m_isFree;
};

// ============== SSD ====================

class SSD {
public:
    static int m_logicBlock; // = 0
    static int m_physicBlock; // = 0
    static int m_minimumFreeBlock; // =0

public:
    explicit SSD();
    virtual ~SSD();

    void allocateSpace(Executor *executor);
    /**
     * @brief init entryTable & replace Table
     * @param filename
     * @param tablename: entryTable & replaceTable
     */
    bool initTable(std::string filename, std::string tablename);
    bool writeBackTable(std::string filename, std::string tablename);

    virtual bool initFreeBlockList(std::string filename);
    bool writeBackFreeBlockList(std::string filename);
    /**
     * @brief init meta data in class BlockData & PageData
     */
    bool initMetaData(std::string filename);
    bool writeBackMetaData(std::string filename);

    bool initBlockData(std::string filenamePrefix);
    bool writeBackBlockData(std::string filenamePrefix);

    bool writeBackSummary(std::string filename);


    void reset();
    bool setExecutor(Executor *executor);

    int readPage(int logicNum, PageData *data);
    int writePage(int logicNum, PageData *data);

    virtual int findfree();
    virtual bool gc(std::vector<int> &gcChain, int pageNum);
    /**
     * @brief logic2physic : only serves readpage operation
     */
    virtual bool logic2physic(int logicNum, int &phyBlock, int &phyPage);
    /**
     * @brief findfreePage : only serves writepage operation
     * @param startBlock : start block of the block-chain
     * @param phyBlock
     * @param phyPage
     */
    virtual bool findfreePage(int logicNum, int &phyBlock, int &phyPage);

    bool linear2threeDimesion(int linear, int &x, int &y, int &z);
    bool threeDimension2linear(int x, int y, int z, int &linear);

private:
    bool loadLevelBlockData(std::ifstream &inStream, int levelIndex);
    bool writeLevelBlockData(std::ofstream &outStream, int levelIndex);

public:
    Executor *m_executor;
    BlockData **m_blockData;

    int *m_entryTable,
        *m_replaceTable;

    std::list<int> m_freeBlock;
};

// =============== SSD_1 ===================

class SSD_1 : public SSD {
public:
    explicit SSD_1();
    virtual ~SSD_1();

    virtual bool initFreeBlockList(std::string filename);
    virtual int findfree();
    virtual bool gc(std::vector<int> &gcChain, int logicNum);
    /**
     * @brief only used for readpage operation
     */
    virtual bool logic2physic(int logicNum, int &phyBlock, int &phyPage);
    /**
     * @brief only used for writepage operation
     */
    virtual bool findfreePage(int logicNum, int &phyBlock, int &phyPage);
};

// =============== SSD_2 ===================
class SSD_2 : public SSD_1 {
public:
    explicit SSD_2();
    virtual ~SSD_2();

    virtual bool initFreeBlockList(std::string filename);
    virtual int findfree();
};

// =============== Executor=================
class Executor {
public:
    explicit Executor();
    ~Executor();

public:
    bool readConfigFile(std::string configFileName);

    bool initData();
    void setSSD(SSD *ssd);

    bool beginProcess();
    bool endProcess();

    void checkTime(double time);
    void checkTemper(int blockIndex, double temper);

public:
    void disspateTemper();
    void releaseTemper(int neighborPre, int phyBlock,
                       int neighborNext, double percent);
    void statisticTemper();

    void injectErr();
    void createErrForBlock(int blockIndex, int errCount);
    void statisticErr();

    bool processRequest(std::ifstream &inStream);
public:

    // stucture of SSD
    int m_bytePerPage,
        m_pagePerBlock,
        m_blockPerLine,
        m_linePerLevel,
        m_levelPerChip,
        m_minimumFreeBlock;

    int m_logicBlock,
        m_physicBlock;

    bool m_isRealData,
        m_dataFromFile;


    // time variation
    double m_readOobTime,
            m_writeOobTime,
            m_readPageTime,
            m_writePageTime,
            m_eraseBlockTime;

    // temperature dissipation percentage
    double m_lrTemperPercent,
            m_fbTemperPercent,
            m_udTemperPercent;

    // temperature raise (absolute value)
    double m_readPageTemper,
            m_writePageTemper,
            m_eraseBlockTemper;

    // time interval
    int m_scanTemperInterval,
        m_dissTemperInterval,
        m_createErrInterval,
        m_calErrInterval;

    std::map<double, int> m_temper2numBlockErr;
    int m_eccNumBit;
    // -------------------------------------------
    // input file
    std::vector<std::string> m_traceFileNameList;
    std::string m_entryTableFile;
    std::string m_replaceTableFile;
    std::string m_dataFilePrefix;
    std::string m_metaDataFile;
    std::string m_freeBlockListFile;


    // output filename
    std::string m_temperShotPrefix; // prefix_times_level
    std::string m_errorFilename;
    std::string m_summaryFilename;

    // -------------------------------------------
    SSD *m_ssd;

private:
    // time ticker
    double m_dissipateTemperTime,
            m_statisticTemperTime,
            m_injectErrTime,
            m_statisticErrTime;
};


