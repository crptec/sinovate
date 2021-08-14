// Copyright (c) 2018-2019 SIN developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SIN_INFINITYNODEMAN_H
#define SIN_INFINITYNODEMAN_H

#include <sinovate/infinitynode.h>
#include <sinovate/infinitynodemeta.h>
#include <sinovate/infinitynodelockinfo.h>

#include <logging.h>

using namespace std;

class CInfinitynodeMan;
class CConnman;

extern CInfinitynodeMan infnodeman;

class CInfinitynodeMan
{
public:
    typedef std::pair<arith_uint256, CInfinitynode*> score_pair_t;
    typedef std::vector<score_pair_t> score_pair_vec_t;
    typedef std::pair<CScript, std::string> lockreward_pair_t; //how send LR and signature string
    typedef std::vector<lockreward_pair_t> lockreward_pair_vec_t;

    // critical section to protect the inner data structures
    mutable RecursiveMutex cs;
    // map to hold all INFs
    std::map<COutPoint, CInfinitynode> mapInfinitynodes;
    std::map<COutPoint, CInfinitynode> mapInfinitynodesNonMatured;
    // map to hold tempo metadata of next block
    std::vector<CMetadata> vMetaNextBlock;
    // map to hold all reward statement
    std::map<int, int> mapStatementBIG;
    std::map<int, int> mapStatementMID;
    std::map<int, int> mapStatementLIL;
    // map to hold payee and lastPaid Height
    std::map<CScript, int> mapLastPaid;
    //cache for last situation per block
    int nBIGLastStmHeight;
    int nMIDLastStmHeight;
    int nLILLastStmHeight;
    int nBIGLastStmSize;
    int nMIDLastStmSize;
    int nLILLastStmSize;

private:
    static const std::string SERIALIZATION_VERSION_STRING;
    // Keep track of current block height and first download block
    int nCachedBlockHeight;
    //
    bool fReachedLastBlock = false;
    mutable RecursiveMutex cs_LastPaid;
public:

    CInfinitynodeMan();

    int64_t nLastScanHeight;//last verification from blockchain

    SERIALIZE_METHODS(CInfinitynodeMan, obj)
    {
        std::string strVersion;
        if(ser_action.ForRead()) {
            READWRITE(strVersion);
        }
        else {
            strVersion = SERIALIZATION_VERSION_STRING;
            READWRITE(strVersion);
        }

        READWRITE(obj.mapInfinitynodes);
        READWRITE(obj.mapInfinitynodesNonMatured);
        READWRITE(obj.mapLastPaid);
        READWRITE(obj.nLastScanHeight);
        READWRITE(obj.mapStatementBIG);
        READWRITE(obj.mapStatementMID);
        READWRITE(obj.mapStatementLIL);
        READWRITE(obj.nBIGLastStmHeight);
        READWRITE(obj.nMIDLastStmHeight);
        READWRITE(obj.nLILLastStmHeight);
        READWRITE(obj.nBIGLastStmSize);
        READWRITE(obj.nMIDLastStmSize);
        READWRITE(obj.nLILLastStmSize);
    }

    std::string ToString() const;

    bool Add(CInfinitynode &inf);
    bool AddUpdateLastPaid(CScript scriptPubKey, int nHeightLastPaid);
    /// Find an entry
    CInfinitynode* Find(const COutPoint& outpoint);

    bool GetInfinitynodeInfo(std::string nodePublicKey, infinitynode_info_t& infInfoRet);
    bool GetInfinitynodeInfo(const COutPoint& outpoint, infinitynode_info_t& infInfoRet);

    /// Clear InfinityNode vector
    void Clear();
    /// Versions of Find that are safe to use from outside the class
    bool Get(const COutPoint& outpoint, CInfinitynode& infinitynodeRet);
    bool Has(const COutPoint& outpoint);
    bool HasPayee(CScript scriptPubKey);
    int Count();
    int CountEnabled();
    std::map<COutPoint, CInfinitynode> GetFullInfinitynodeMap() { LOCK(cs); return mapInfinitynodes; }
    std::map<COutPoint, CInfinitynode> GetFullInfinitynodeNonMaturedMap() { LOCK(cs); return mapInfinitynodesNonMatured; }
    std::map<int, int> getStatementMap(int nSinType){
        LOCK(cs);
        std::map<int, int> nullmap = {{0,0}};
        if(nSinType == 10) return mapStatementBIG;
        else if(nSinType == 5) return mapStatementMID;
        else if(nSinType == 1) return mapStatementLIL;
        else return nullmap;
    }
    int getLastStatement(int nSinType){
        LOCK(cs);
        if(nSinType == 10) return nBIGLastStmHeight;
        if(nSinType == 5) return nMIDLastStmHeight;
        if(nSinType == 1) return nLILLastStmHeight;
        return 0;
    }
    int getLastStatementSize(int nSinType){
        LOCK(cs);
        if(nSinType == 10) return nBIGLastStmSize;
        if(nSinType == 5) return nMIDLastStmSize;
        if(nSinType == 1) return nLILLastStmSize;
        return 0;
    }
    int getCacheHeightInf(){LOCK(cs); return nCachedBlockHeight;};

    //call in infinitynodetip
    void setSyncStatus(bool flag){LOCK(cs); fReachedLastBlock=flag;}
    bool isReachedLastBlock(){LOCK(cs); return fReachedLastBlock;}

    std::map<CScript, int> GetFullLastPaidMap() { return mapLastPaid; }
    int64_t getLastScan(){return nLastScanHeight;}
    int64_t getLastScanWithLimit(){return nLastScanHeight/* + INF_MATURED_LIMIT*/;} // We'll need to move this to functions who actually use it and match it with our max reorg depth
    //build DIN map immediate when connect block, before validation of block. updateFinalList OR removeNonMaturedList will be called if block is valid
    bool buildNonMaturedListFromBlock(const CBlock& block, CBlockIndex* pindex,
                  CCoinsViewCache& view, const CChainParams& chainparams); //call in validation.cpp
    bool updateFinalList(CBlockIndex* pindex, CCoinsViewCache& view); // call when block is valid
    bool removeNonMaturedList(CBlockIndex* pindex); //call when block is invalid or disconnecttip

    //LR read back
    bool ExtractLockReward(int nBlockHeight, int depth, std::vector<CLockRewardExtractInfo>& vecLRRet);
    bool getLRForHeight(int height, std::vector<CLockRewardExtractInfo>& vecLockRewardRet);

    //this function build the map of STM from genesis
    bool calculStatementOnValidation(int nHeight);
    bool deterministicRewardAtHeightOnValidation(int nBlockHeight, int nSinType, CInfinitynode& infinitynodeRet);

    bool deterministicRewardAtHeight(int nBlockHeight, int nSinType, CInfinitynode& infinitynodeRet);
    std::map<int, CInfinitynode> calculInfinityNodeRank(int nBlockHeight, int nSinType, bool updateList=false, bool flagExtCall = false);
    std::pair<int, int> getLastStatementBySinType(int nSinType);
    std::string getLastStatementString() const;
    int getRoi(int nSinType, int totalNode);

    int isPossibleForLockReward(COutPoint burntx);
    bool getScoreVector(const uint256& nBlockHash, int nSinType, int nBlockHeight, CInfinitynodeMan::score_pair_vec_t& vecScoresRet);
    bool getNodeScoreAtHeight(const COutPoint& outpoint, int nSinType, int nBlockHeight, int& nRankRet);
    bool getTopNodeScoreAtHeight(int nSinType, int nBlockHeight, int nTop, std::vector<CInfinitynode>& vecInfRet);
    bool isValidTopNode(const std::vector<COutPoint>  &vOutpoint, int nSinType, int nBlockHeight, int nTop);

    std::string getVectorNodeRankAtHeight(const std::vector<COutPoint>& vOutpoint, int nSinType, int nBlockHeight);

    void CheckAndRemove();
    void UpdatedBlockTip(const CBlockIndex *pindex);
    void UpdateChainActiveHeight(int number);

    // save the state of infinitynode and metadata to disk
    void FlushStateToDisk();
};
#endif // SIN_INFINITYNODEMAN_H
