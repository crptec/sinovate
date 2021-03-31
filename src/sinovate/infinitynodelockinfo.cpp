// Copyright (c) 2018-2020 SIN developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <sinovate/infinitynodelockinfo.h>
#include <sinovate/infinitynodeman.h>
#include <sinovate/flat-database.h>
#include <core_io.h>

CInfinitynodeLockInfo infnodelrinfo;

const std::string CInfinitynodeLockInfo::SERIALIZATION_VERSION_STRING = "CInfinitynodeLockInfo-Version-1";

void CInfinitynodeLockInfo::Clear()
{
    LOCK(cs);
    vecLRInfo.clear();
}

bool CInfinitynodeLockInfo::Has(std::string  sInfo)
{
    LOCK(cs);
    for(auto& vinfo : vecLRInfo){
        if(vinfo.sLRInfo == sInfo){return true;}
    }
    return false;
}

bool CInfinitynodeLockInfo::Add(CLockRewardExtractInfo &lrinfo){
    LOCK(cs);
    for(auto& vinfo : vecLRInfo){
        if(vinfo.sLRInfo == lrinfo.sLRInfo){return false;}
    }
    vecLRInfo.push_back(lrinfo);
    return true;
}

bool CInfinitynodeLockInfo::Remove(CLockRewardExtractInfo &lrinfo){
    LOCK(cs);
    for(auto it = vecLRInfo.begin(); it != vecLRInfo.end(); ){
        if((*it).sLRInfo == lrinfo.sLRInfo) {
            it = vecLRInfo.erase(it);
        } else {
            ++it;
        }
    }
    return false;
}

bool CInfinitynodeLockInfo::getLRInfo(int nRewardHeight, std::vector<CLockRewardExtractInfo>& vecLRRet)
{
    vecLRRet.clear();
    LOCK(cs);
    for(auto& vinfo : vecLRInfo){
        if(vinfo.nRewardHeight == nRewardHeight){
            vecLRRet.push_back(vinfo);
        }
    }
    return true;
}

bool CInfinitynodeLockInfo::getLRInfoFromHeight(int nRewardHeight, std::vector<CLockRewardExtractInfo>& vecLRRet)
{
    vecLRRet.clear();
    LOCK(cs);
    for(auto& vinfo : vecLRInfo){
        if(vinfo.nRewardHeight >= nRewardHeight){
            vecLRRet.push_back(vinfo);
        }
    }
    return true;
}

bool CInfinitynodeLockInfo::ExtractLRFromBlock(const CBlock& block, CBlockIndex* pindex,
                  CCoinsViewCache& view, const CChainParams& chainparams, std::vector<CLockRewardExtractInfo>& vecLRRet)
{
    vecLRRet.clear();

    for (unsigned int i = 0; i < block.vtx.size(); i++) {
        const CTransaction &tx = *(block.vtx[i]);
        //Not coinbase
        if (!tx.IsCoinBase()) {
                    for (unsigned int i = 0; i < tx.vout.size(); i++) {
                        const CTxOut& out = tx.vout[i];
                        std::vector<std::vector<unsigned char>> vSolutions;
                        const CScript& prevScript = out.scriptPubKey;
                        TxoutType whichType = Solver(prevScript, vSolutions);
                        //Burncoin for LockReward
                        if (whichType == TxoutType::TX_BURN_DATA && Params().GetConsensus().cLockRewardAddress == EncodeDestination(PKHash(uint160(vSolutions[0])))) {
                            if (vSolutions.size() != 2) {continue;}
                            std::string stringLRRegister(vSolutions[1].begin(), vSolutions[1].end());

                            std::string s;
                            stringstream ss(stringLRRegister);
                            //verify the height of registration info
                            int i=0;
                            int nRewardHeight = 0;
                            int nSINtype = 0;
                            std::string signature = "";
                            int *signerIndexes;
                            // Currently not used
                            //size_t N_SIGNERS = (size_t)Params().GetConsensus().nInfinityNodeLockRewardSigners;
                            int registerNbInfos = Params().GetConsensus().nInfinityNodeLockRewardSigners + 3;
                            signerIndexes = (int*) malloc(Params().GetConsensus().nInfinityNodeLockRewardSigners * sizeof(int));

                            while (getline(ss, s,';')) {
                                if(i==0){nRewardHeight = atoi(s);}
                                if(i==1){nSINtype = atoi(s);}
                                if(i==2){signature = s;}
                                if(i>=3 && i < registerNbInfos){
                                    signerIndexes[i-3] = atoi(s);
                                }
                                i++;
                            }

                            //identify owner of tx
                            const Coin& coin = view.AccessCoin(tx.vin[0].prevout);
                            if (!coin.IsSpent()) {
                                CLockRewardExtractInfo lrinfo(pindex->nHeight, nSINtype, nRewardHeight, coin.out.scriptPubKey, stringLRRegister);
                                vecLRRet.push_back(lrinfo);
                            }
                            free(signerIndexes);
                        }
                    }
        }
    }
    return true;
}