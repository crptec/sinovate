// Copyright (c) 2018-2020 SIN developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SIN_INFINITYNODELRINFO_H
#define SIN_INFINITYNODELRINFO_H

#include <sinovate/infinitynode.h>
using namespace std;

class CLockRewardExtractInfo;
class CInfinitynodeLockInfo;

extern CInfinitynodeLockInfo infnodelrinfo;

class CLockRewardExtractInfo
{
public:
    int nBlockHeight{0}; //blockHeight read
    int nSINtype{0};
    int nRewardHeight{0};
    CScript scriptPubKey{};
    std::string sLRInfo="";

    CLockRewardExtractInfo() = default;
    CLockRewardExtractInfo(int nBlockHeightIn, int nSINtypeIn, int nRewardHeightIn, CScript nPayee, std::string sInfo):
    nBlockHeight(nBlockHeightIn),
    nSINtype(nSINtypeIn),
    nRewardHeight(nRewardHeightIn),
    scriptPubKey(nPayee),
    sLRInfo(sInfo)
    {}

    SERIALIZE_METHODS(CLockRewardExtractInfo, obj)
    {
        READWRITE(obj.nBlockHeight);
        READWRITE(obj.nSINtype);
        READWRITE(obj.nRewardHeight);
        READWRITE(obj.scriptPubKey);
        READWRITE(obj.sLRInfo);
    }
};

class CInfinitynodeLockInfo
{
private:
    static const std::string SERIALIZATION_VERSION_STRING;
    // critical section to protect the inner data structures
    mutable RecursiveMutex cs;
    // Keep track of current block height
    int nCachedBlockHeight;
public:
    std::vector<CLockRewardExtractInfo> vecLRInfo;

    CInfinitynodeLockInfo():
    cs(),
    vecLRInfo()
    {}

    SERIALIZE_METHODS(CInfinitynodeLockInfo, obj)
    {
        std::string strVersion;
        if(ser_action.ForRead()) {
            READWRITE(strVersion);
        }
        else {
            strVersion = SERIALIZATION_VERSION_STRING;
            READWRITE(strVersion);
        }
        READWRITE(obj.vecLRInfo);
    }

    void Clear();
    bool Add(CLockRewardExtractInfo &lrinfo);
    bool Remove(CLockRewardExtractInfo &lrinfo);
    bool Has(std::string  lrinfo);
    std::vector<CLockRewardExtractInfo> getFullLRInfo(){LOCK(cs); return vecLRInfo; }
    bool getLRInfo(int nRewardHeight, std::vector<CLockRewardExtractInfo>& vecLRRet);
    bool getLRInfoFromHeight(int nRewardHeight, std::vector<CLockRewardExtractInfo>& vecLRRet);
    bool ExtractLRFromBlock(const CBlock& block, CBlockIndex* pindex,
                  CCoinsViewCache& view, const CChainParams& chainparams, std::vector<CLockRewardExtractInfo>& vecLRRet);

    std::string ToString() const;
    /// This is dummy overload to be used for dumping/loading mncache.dat
    void CheckAndRemove() {}
};
#endif // SIN_INFINITYNODELRINFO_H