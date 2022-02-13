// Copyright (c) 2018-2019 SIN developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <sinovate/infinitynodetip.h>
#include <sinovate/infinitynodeman.h>
#include <sinovate/infinitynodersv.h>
#include <sinovate/infinitynodepeer.h>
#include <sinovate/flat-database.h>
#include <chainparams.h>
#include <key_io.h>
#include <script/standard.h>
#include <netbase.h>
#include <node/blockstorage.h>


CInfinitynodeMan infnodeman;

const std::string CInfinitynodeMan::SERIALIZATION_VERSION_STRING = "CInfinitynodeMan-Version-1";

struct CompareIntValue
{
    bool operator()(const std::pair<int, CInfinitynode*>& t1,
                    const std::pair<int, CInfinitynode*>& t2) const
    {
        return (t1.first != t2.first) ? (t1.first < t2.first) : (t1.second->vinBurnFund < t2.second->vinBurnFund);
    }
};

struct CompareUnit256Value
{
    bool operator()(const std::pair<arith_uint256, CInfinitynode*>& t1,
                    const std::pair<arith_uint256, CInfinitynode*>& t2) const
    {
        return (t1.first != t2.first) ? (t1.first < t2.first) : (t1.second->vinBurnFund < t2.second->vinBurnFund);
    }
};

struct CompareNodeScore
{
    bool operator()(const std::pair<arith_uint256, CInfinitynode*>& t1,
                    const std::pair<arith_uint256, CInfinitynode*>& t2) const
    {
        return (t1.first != t2.first) ? (t1.first < t2.first) : (t1.second->vinBurnFund < t2.second->vinBurnFund);
    }
};

CInfinitynodeMan::CInfinitynodeMan()
: cs(),
  mapInfinitynodes(),
  nLastScanHeight(0)
{}

void CInfinitynodeMan::Clear()
{
    LOCK(cs);
    mapInfinitynodes.clear();
    mapInfinitynodesNonMatured.clear();
    mapLastPaid.clear();
    nLastScanHeight = 0;
    nCachedBlockHeight = 0;
}

bool CInfinitynodeMan::Add(CInfinitynode &inf)
{
    LOCK(cs);
    if (Has(inf.vinBurnFund.prevout)) return false;
    mapInfinitynodes[inf.vinBurnFund.prevout] = inf;
    return true;
}

CInfinitynode* CInfinitynodeMan::Find(const COutPoint &outpoint)
{
    LOCK(cs);
    auto it = mapInfinitynodes.find(outpoint);
    return it == mapInfinitynodes.end() ? NULL : &(it->second);
}

bool CInfinitynodeMan::Get(const COutPoint& outpoint, CInfinitynode& infinitynodeRet)
{
    // Theses mutexes are recursive so double locking by the same thread is safe.
    LOCK(cs);
    auto it = mapInfinitynodes.find(outpoint);
    if (it == mapInfinitynodes.end()) {
        return false;
    }

    infinitynodeRet = it->second;
    return true;
}

bool CInfinitynodeMan::Has(const COutPoint& outpoint)
{
    LOCK(cs);
    return mapInfinitynodes.find(outpoint) != mapInfinitynodes.end();
}

bool CInfinitynodeMan::HasPayee(CScript scriptPubKey)
{
    LOCK(cs_LastPaid);
    return mapLastPaid.find(scriptPubKey) != mapLastPaid.end();
}

int CInfinitynodeMan::Count()
{
    LOCK(cs);
    return mapInfinitynodes.size();
}

int CInfinitynodeMan::CountEnabled()
{
    LOCK(cs);

    if (mapInfinitynodes.empty()) {
        return 0;
    }
    int i = 0;
    // calculate scores for SIN type 10
    for (auto& infpair : mapInfinitynodes) {
        CInfinitynode inf = infpair.second;
        if (inf.getExpireHeight() >= nLastScanHeight) {
            i++;
        }
    }
    return i;
}

std::string CInfinitynodeMan::ToString() const
{
    std::ostringstream info;

    info << "InfinityNode: " << (int)mapInfinitynodes.size() <<
            ", nLastScanHeight: " << (int)nLastScanHeight;

    return info.str();
}

void CInfinitynodeMan::CheckAndRemove()
{
    /*this function is called in InfinityNode thread*/
    LOCK(cs); //cs_main needs to be called by the parent function

    if (nCachedBlockHeight == 0) {
        LogPrint(BCLog::INFINITYMAN,"CInfinitynodeMan::CheckAndRemove -- Node is just retarted. Waiting for new block\n");
        return;
    }

    //optimisation memory
    std::map<CScript, int>::iterator itLP = mapLastPaid.begin();
    while (itLP != mapLastPaid.end()) {
        if (itLP->second < nCachedBlockHeight - Params().MaxReorganizationDepth() * 2) {
            mapLastPaid.erase(itLP++);
        } else {
            ++itLP;
        }
    }

    std::map<COutPoint, CInfinitynode>::iterator itNonMatured =  mapInfinitynodesNonMatured.begin();
    while (itNonMatured != mapInfinitynodesNonMatured.end()) {
        CInfinitynode inf = itNonMatured->second;
        if (inf.nHeight < nCachedBlockHeight - Params().MaxReorganizationDepth() * 2) {
            mapInfinitynodesNonMatured.erase(itNonMatured++);
        } else {
            ++itNonMatured;
        }
    }

    return;
}

int CInfinitynodeMan::getRoi(int nSinType, int totalNode)
{
    LOCK(cs);
    int nBurnAmount = 0;
    if (nSinType == 10) nBurnAmount = Params().GetConsensus().nMasternodeBurnSINNODE_10;
    if (nSinType == 5) nBurnAmount = Params().GetConsensus().nMasternodeBurnSINNODE_5;
    if (nSinType == 1) nBurnAmount = Params().GetConsensus().nMasternodeBurnSINNODE_1;

    float nReward = GetInfinitynodePayment(nCachedBlockHeight, nSinType) / COIN;
    float roi = nBurnAmount / ((720 / (float)totalNode) * nReward) ;
    return (int) roi;
}

//pindex->nHeight is a new block. But we build list only for matured block (sup than limit reorg)
bool CInfinitynodeMan::buildNonMaturedListFromBlock(const CBlock& block, CBlockIndex* pindex,
                  CCoinsViewCache& view, const CChainParams& chainparams)
{
    if (Params().NetworkIDString() == CBaseChainParams::MAIN) {
        if(pindex->nHeight < Params().GetConsensus().nInfinityNodeBeginHeight){
            mapInfinitynodesNonMatured.clear();
            return true;
        }
    } else {
        if (pindex->nHeight <= 1) {
            mapInfinitynodesNonMatured.clear();
        }
    }

    vMetaNextBlock.clear();
    LOCK(cs);

    //update NON matured map
    for (unsigned int i = 0; i < block.vtx.size(); i++) {
        const CTransaction &tx = *(block.vtx[i]);
        //Not coinbase
        if (!tx.IsCoinBase()) {
                for (unsigned int i = 0; i < tx.vout.size(); i++) {
                        const CTxOut& out = tx.vout[i];
                        std::vector<std::vector<unsigned char>> vSolutions;
                        const CScript& prevScript = out.scriptPubKey;
                        TxoutType whichType = Solver(prevScript, vSolutions);
                        //Send to BurnAddress
                        if (whichType == TxoutType::TX_BURN_DATA && Params().GetConsensus().cBurnAddress == EncodeDestination(PKHash(uint160(vSolutions[0]))))
                        {
                            //Amount for InfnityNode
                            if (
                            ((Params().GetConsensus().nMasternodeBurnSINNODE_1 - 1) * COIN < out.nValue && out.nValue <= Params().GetConsensus().nMasternodeBurnSINNODE_1 * COIN) ||
                            ((Params().GetConsensus().nMasternodeBurnSINNODE_5 - 1) * COIN < out.nValue && out.nValue <= Params().GetConsensus().nMasternodeBurnSINNODE_5 * COIN) ||
                            ((Params().GetConsensus().nMasternodeBurnSINNODE_10 - 1) * COIN < out.nValue && out.nValue <= Params().GetConsensus().nMasternodeBurnSINNODE_10 * COIN)
                            ) {
                                COutPoint outpoint(tx.GetHash(), i);
                                CInfinitynode inf(PROTOCOL_VERSION, outpoint);
                                inf.setHeight(pindex->nHeight);
                                inf.setBurnValue(out.nValue);

                                if (vSolutions.size() == 2) {
                                    std::string backupAddress(vSolutions[1].begin(), vSolutions[1].end());
                                    CTxDestination NodeAddress = DecodeDestination(backupAddress);
                                    if (IsValidDestination(NodeAddress)) {
                                        inf.setBackupAddress(backupAddress);
                                    }
                                }
                                //SINType
                                CAmount nBurnAmount = out.nValue / COIN + 1; //automaticaly round
                                inf.setSINType(nBurnAmount / 100000);

                                //Address payee: we known that there is only 1 input
                                const Coin& coin = view.AccessCoin(tx.vin[0].prevout);

                                CTxDestination addressBurnFund;
                                if (!ExtractDestination(coin.out.scriptPubKey, addressBurnFund)) {
                                    LogPrint(BCLog::INFINITYMAN,"CInfinitynodeMan::updateInfinityNodeInfo -- False when extract payee from BurnFund tx.\n");
                                    return false;
                                }

                                inf.setCollateralAddress(EncodeDestination(addressBurnFund));
                                inf.setScriptPublicKey(coin.out.scriptPubKey);

                                //we have all infos. Then add in mapNonMatured
                                if (mapInfinitynodesNonMatured.find(inf.vinBurnFund.prevout) != mapInfinitynodesNonMatured.end()) {
                                    //exist
                                    continue;
                                } else {
                                    //non existe
                                    mapInfinitynodesNonMatured[inf.vinBurnFund.prevout] = inf;
                                }
                            }
                        }
                        //Amount to update Metadata
                        if (whichType == TxoutType::TX_BURN_DATA && Params().GetConsensus().cMetadataAddress == EncodeDestination(PKHash(uint160(vSolutions[0]))))
                        {
                            //Amount for UpdateMeta
                            if ((Params().GetConsensus().nInfinityNodeUpdateMeta - 1) * COIN <= out.nValue && out.nValue <= (Params().GetConsensus().nInfinityNodeUpdateMeta) * COIN) {
                                if (vSolutions.size() == 2) {
                                    std::string metadata(vSolutions[1].begin(), vSolutions[1].end());
                                    string s;
                                    stringstream ss(metadata);
                                    int i=0;
                                    int check=0;
                                    std::string publicKeyString;
                                    CService service;
                                    std::string burnTxID;
                                    while (getline(ss, s,';')) {
                                        CTxDestination NodeAddress;
                                        //1st position: Node Address
                                        if (i==0) {
                                            publicKeyString = s;
                                            std::vector<unsigned char> tx_data = DecodeBase64(publicKeyString.c_str());
                                            CPubKey decodePubKey(tx_data.begin(), tx_data.end());
                                            if (decodePubKey.IsValid()) {check++;}
                                        }
                                        //2nd position: Node IP
                                        if (i==1) {
                                            if (Lookup(s.c_str(), service, 0, false)) {
                                                check++;
                                            }
                                        }
                                        //3th position: 16 character from Infinitynode BurnTx
                                        if (i==2 && s.length() >= 16) {
                                            check++;
                                            burnTxID = s.substr(0, 16);
                                        }
                                        //Update node metadata if nHeight is bigger
                                        if (check == 3) {
                                            //Address payee: we known that there is only 1 input
                                            const Coin& coin = view.AccessCoin(tx.vin[0].prevout);

                                            CTxDestination addressBurnFund;
                                            if (!ExtractDestination(coin.out.scriptPubKey, addressBurnFund)) {
                                                LogPrint(BCLog::INFINITYMAN,"CInfinitynodeMeta::metaScan -- False when extract payee from BurnFund tx.\n");
                                                return false;
                                            }

                                            std::ostringstream streamInfo;
                                            streamInfo << EncodeDestination(addressBurnFund) << "-" << burnTxID;

                                            LogPrint(BCLog::INFINITYMAN,"CInfinitynodeMeta:: meta update: %s, %s, %s\n", 
                                                         streamInfo.str(), publicKeyString, service.ToString());
                                            int avtiveBK = 0;
                                            CMetadata meta = CMetadata(streamInfo.str(), publicKeyString, service, pindex->nHeight, avtiveBK);

                                            //use new method to update metadata with cached info
                                            if (pindex->nHeight < Params().GetConsensus().nINMetaUpdateCachedNextBlock) {
                                                infnodemeta.Add(meta);
                                            } else {
                                                vMetaNextBlock.push_back(meta);
                                            }
                                        }
                                        i++;
                                    }
                                }
                            }
                        }
                }
        }
    }

    return true;
}

bool CInfinitynodeMan::updateFinalList(CBlockIndex* pindex, CCoinsViewCache& view)
{
    LOCK(cs);

    // move matured node to final list
    nLastScanHeight = pindex->nHeight - Params().MaxReorganizationDepth();

    // make sure that DIN node in matured block is not changed because of reorg
    for (auto& infpair : mapInfinitynodesNonMatured) {
        if(infpair.second.nHeight == nLastScanHeight) {
            CInfinitynode inf = infpair.second;
            //make sure that BurnTx is on block and no reorg possible.
            Coin coin = view.AccessCoin(inf.getBurntxOutPoint());
            if (!coin.IsSpent() && coin.nHeight == infpair.second.nHeight) {
                Add(inf);
            }
        }
    }

    // use new method to update metadata with cached info
    if (pindex->nHeight >= Params().GetConsensus().nINMetaUpdateCachedNextBlock) {
        for (auto& v : vMetaNextBlock) {
            infnodemeta.Add(v);
        }
    }

    nCachedBlockHeight = pindex->nHeight;

    bool updateStm=false;
    if (nCachedBlockHeight < Params().GetConsensus().nDINActivationHeight) {
        // do nothing
    } else if (nCachedBlockHeight == Params().GetConsensus().nDINActivationHeight) {
        // rebuild Stm at fork Height
        for (int i = Params().GetConsensus().nInfinityNodeGenesisStatement; i < Params().GetConsensus().nDINActivationHeight; i++) {
            updateStm=calculStatementOnValidation(i);
        }
    } else {
        // update Stm map
        updateStm=calculStatementOnValidation(nCachedBlockHeight);
    }

    return true;
}

bool CInfinitynodeMan::removeNonMaturedList(CBlockIndex* pindex)
{
    LOCK(cs);

    std::map<COutPoint, CInfinitynode>::iterator it =  mapInfinitynodesNonMatured.begin();
    while (it != mapInfinitynodesNonMatured.end()) {
        CInfinitynode inf = it->second;
        // Currently not used
        //COutPoint txOutPoint = it->first;
        if (inf.nHeight == pindex->nHeight) {
            mapInfinitynodesNonMatured.erase(it++);
            LogPrint(BCLog::INFINITYMAN,"CInfinitynodeMan::removeNonMaturedList -- remove node: %s (height: %d) from nonmatured list.\n",inf.getMetaID(), inf.getHeight());
        } else {
            ++it;
        }
    }

    return true;
}

/*
 * LOCK (cs_main) before call this function
 */
bool CInfinitynodeMan::ExtractLockReward(int nBlockHeight, int depth, std::vector<CLockRewardExtractInfo>& vecLRRet, CChainState& chainstate)
{
    vecLRRet.clear();

    if (nBlockHeight < Params().GetConsensus().nInfinityNodeBeginHeight) {
        return true;
    }

    AssertLockHeld(cs_main);

    CBlockIndex* pindex  = chainstate.m_chain[nBlockHeight]; 
    CBlockIndex* prevBlockIndex = pindex;

    //read back 55 blocks and find lockreward for height
    while (prevBlockIndex->nHeight >= (nBlockHeight - depth))
    {
        CBlock blockReadFromDisk;
        if (ReadBlockFromDisk(blockReadFromDisk, prevBlockIndex, Params().GetConsensus()))
        {
            for (const CTransactionRef& tx : blockReadFromDisk.vtx) {
                //Not coinbase
                if (!tx->IsCoinBase()) {
                    for (unsigned int i = 0; i < tx->vout.size(); i++) {
                        const CTxOut& out = tx->vout[i];
                        std::vector<std::vector<unsigned char>> vSolutions;

                        const CScript& prevScript = out.scriptPubKey;
                        TxoutType whichType = Solver(prevScript, vSolutions);

                        if (whichType == TxoutType::TX_BURN_DATA && Params().GetConsensus().cLockRewardAddress == EncodeDestination(PKHash(uint160(vSolutions[0])))) {
                            //LogPrint(BCLog::INFINITYMAN,"CInfinitynodeMan::ExtractLockReward -- block Height: %d, info size: %d\n", prevBlockIndex->nHeight, (int)vSolutions.size());
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
                                if (i==0) {nRewardHeight = atoi(s);}
                                if (i==1) {nSINtype = atoi(s);}
                                if (i==2) {signature = s;}
                                if (i>=3 && i < registerNbInfos) {
                                    signerIndexes[i-3] = atoi(s);
                                }
                                i++;
                            }

                            free(signerIndexes);

                            if (nRewardHeight != nBlockHeight +1) {continue;}
                            LogPrint(BCLog::INFINITYMAN,"CInfinitynodeMan::ExtractLockReward -- LR: %s.\n", stringLRRegister);

                            const CTxIn& txin = tx->vin[0];
                            int index = txin.prevout.n;

                            CTransactionRef prevtx;
                            uint256 hashblock;
                            if (!GetTransaction(txin.prevout.hash, prevtx, hashblock)) {
                                LogPrint(BCLog::INFINITYMAN,"CInfinitynodeMan::ExtractLockReward -- PrevBurnFund tx is not in block\n");
                                return false;
                            }

                            CLockRewardExtractInfo lrinfo(prevBlockIndex->nHeight, nSINtype, nRewardHeight, prevtx->vout[index].scriptPubKey, stringLRRegister);

                            vecLRRet.push_back(lrinfo);
                        }
                    }
                }
            }
        } else {
            LogPrint(BCLog::INFINITYNODE, "CInfinitynodeMan::ExtractLockReward -- can not read block from disk\n");
            return false;
        }
        // continue with previous block
        prevBlockIndex = prevBlockIndex->pprev;
    }
    return true;
}

/*make sure that LOCK sc_main before call this function*/
bool CInfinitynodeMan::getLRForHeight(int height, std::vector<CLockRewardExtractInfo>& vecLockRewardRet, CChainState& chainstate)
{
    vecLockRewardRet.clear();
    return ExtractLockReward(height, Params().GetConsensus().nInfinityNodeCallLockRewardDeepth * 3, vecLockRewardRet, chainstate);
}

bool CInfinitynodeMan::GetInfinitynodeInfo(std::string nodePublicKey, infinitynode_info_t& infInfoRet)
{
    CMetadata meta;
    if (!infnodemeta.Get(nodePublicKey, meta)) {
        LogPrint(BCLog::INFINITYNODE, "CInfinitynodeMan::GetInfinitynodeInfo -- can not read metadata from Key: %s\n", nodePublicKey);
        return false;
    }
    LOCK(cs);
    for (auto& infpair : mapInfinitynodes) {
        if (infpair.second.getMetaID() == meta.getMetaID()) {
            infInfoRet = infpair.second.GetInfo();
            return true;
        }
    }
    LogPrint(BCLog::INFINITYNODE, "CInfinitynodeMan::GetInfinitynodeInfo -- can not find NodeID in list: %s\n", meta.getMetaID());
    return false;
}

bool CInfinitynodeMan::GetInfinitynodeInfo(const COutPoint& outpoint, infinitynode_info_t& infInfoRet)
{
    LOCK(cs);
    auto it = mapInfinitynodes.find(outpoint);
    if (it == mapInfinitynodes.end()) {
        return false;
    }
    infInfoRet = it->second.GetInfo();
    return true;
}

bool CInfinitynodeMan::calculStatementOnValidation(int nHeight)
{
    if (nHeight < Params().GetConsensus().nInfinityNodeGenesisStatement) {
        {
            LOCK(cs);
            mapStatementLIL.clear();
            mapStatementMID.clear();
            mapStatementBIG.clear();
        }
        return true;
    }

    int nBIG = 0, nMID = 0, nLIL = 0;

    if (nHeight == Params().GetConsensus().nInfinityNodeGenesisStatement) {
        std::map<COutPoint, CInfinitynode> copyMapInfinitynodes;
        {
            LOCK(cs);
            mapStatementLIL.clear();
            mapStatementMID.clear();
            mapStatementBIG.clear();
            copyMapInfinitynodes = mapInfinitynodes;
        }
        //calcul number of nodes at nHeight
        for (auto& infpair : copyMapInfinitynodes) {
            if (infpair.second.getHeight() < nHeight && nHeight <= infpair.second.getExpireHeight()) {
                if (infpair.second.getSINType() == 10) nBIG++;
                if (infpair.second.getSINType() == 5) nMID++;
                if (infpair.second.getSINType() == 1) nLIL++;
            }
        }
        {
            LOCK(cs);
            mapStatementBIG.insert(mapStatementBIG.begin(), make_pair(nHeight, nBIG));
            mapStatementMID.insert(mapStatementMID.begin(), make_pair(nHeight, nMID));
            mapStatementLIL.insert(mapStatementLIL.begin(), make_pair(nHeight, nLIL));
        }

        return true;
    }

    if (nHeight > Params().GetConsensus().nInfinityNodeGenesisStatement) {
        std::map<COutPoint, CInfinitynode> copyMapInfinitynodes;
        std::map<int, int> copyMapStatementBIG;
        std::map<int, int> copyMapStatementMID;
        std::map<int, int> copyMapStatementLIL;
        {
            LOCK(cs);
            copyMapStatementBIG = mapStatementBIG;
            copyMapStatementMID = mapStatementMID;
            copyMapStatementLIL = mapStatementLIL;
            copyMapInfinitynodes = mapInfinitynodes;
        }
        int nextHeight = nHeight + 1;

        std::map<int, int>::iterator itBIG = copyMapStatementBIG.upper_bound(nextHeight);
        std::map<int, int>::iterator itMID = copyMapStatementMID.upper_bound(nextHeight);
        std::map<int, int>::iterator itLIL = copyMapStatementLIL.upper_bound(nextHeight);

        std::map<int, int>::iterator itLastBIG = --itBIG;
        std::map<int, int>::iterator itLastMID = --itMID;
        std::map<int, int>::iterator itLastLIL = --itLIL;

        int nEndOfStmBIG = itLastBIG->first + itLastBIG->second - nextHeight;
        int nEndOfStmMID = itLastMID->first + itLastMID->second - nextHeight;
        int nEndOfStmLIL = itLastLIL->first + itLastLIL->second - nextHeight;

        nBIGLastStmHeight = itLastBIG->first;
        nBIGLastStmSize = itLastBIG->second;

        nMIDLastStmHeight = itLastMID->first;
        nMIDLastStmSize = itLastMID->second;

        nLILLastStmHeight = itLastLIL->first;
        nLILLastStmSize = itLastLIL->second;

        int nBIGNextStm = 0, nMIDNextStm = 0, nLILNextStm = 0;
        int nBIGHeight = 0, nMIDHeight = 0, nLILHeight = 0;

        //calcul number of nodes at nHeight
        for (auto& infpair : copyMapInfinitynodes) {
            if (infpair.second.getSINType() == 10 && infpair.second.getHeight() < itLastBIG->first && itLastBIG->first <= infpair.second.getExpireHeight()) nBIG++;
            if (infpair.second.getSINType() == 5 && infpair.second.getHeight() < itLastMID->first && itLastMID->first <= infpair.second.getExpireHeight()) nMID++;
            if (infpair.second.getSINType() == 1 && infpair.second.getHeight() < itLastLIL->first && itLastLIL->first <= infpair.second.getExpireHeight()) nLIL++;

            if (infpair.second.getHeight() < nHeight && nHeight <= infpair.second.getExpireHeight()) {
                if(infpair.second.getSINType() == 10) nBIGHeight++;
                if(infpair.second.getSINType() == 5) nMIDHeight++;
                if(infpair.second.getSINType() == 1) nLILHeight++;
            }

            if (infpair.second.getHeight() < nextHeight && nextHeight <= infpair.second.getExpireHeight()) {
                if(infpair.second.getSINType() == 10) nBIGNextStm++;
                if(infpair.second.getSINType() == 5) nMIDNextStm++;
                if(infpair.second.getSINType() == 1) nLILNextStm++;
            }
        }

        //begin of network, so not at end of Stm

        if (itLastBIG->second == 0 || itLastBIG->second == 1) copyMapStatementBIG.insert(itBIG, make_pair(nHeight, nBIGHeight));
        if (itLastMID->second == 0 || itLastMID->second == 1) copyMapStatementMID.insert(itMID, make_pair(nHeight, nMIDHeight));
        if (itLastLIL->second == 0 || itLastLIL->second == 1) copyMapStatementLIL.insert(itLIL, make_pair(nHeight, nLILHeight));

        //end of Stm
        if (nEndOfStmBIG == 0 || nEndOfStmMID == 0 || nEndOfStmLIL == 0 ||
           (nEndOfStmLIL > 0 && nEndOfStmLIL <= Params().MaxReorganizationDepth()) ||
           (nEndOfStmMID > 0 && nEndOfStmMID <= Params().MaxReorganizationDepth()) ||
           (nEndOfStmBIG > 0 && nEndOfStmBIG <= Params().MaxReorganizationDepth())
          ) {


            if (nEndOfStmBIG == 0) {
                itLastBIG->second = nBIG;
                copyMapStatementBIG.insert(itBIG, make_pair(nextHeight, nBIGNextStm));
            } else if(nEndOfStmBIG > 0 && nEndOfStmBIG <= Params().MaxReorganizationDepth()) {
                itLastBIG->second = nBIG;
                nBIGLastStmSize = nBIG;
            }

            if (nEndOfStmMID == 0) {
                itLastMID->second = nMID;
                copyMapStatementMID.insert(itMID, make_pair(nextHeight, nMIDNextStm));
            } else if(nEndOfStmMID > 0 && nEndOfStmMID <= Params().MaxReorganizationDepth()) {
                itLastMID->second = nMID;
                nMIDLastStmSize = nMID;
            }

            if (nEndOfStmLIL == 0) {
                itLastLIL->second = nLIL;
                copyMapStatementLIL.insert(itLIL, make_pair(nextHeight, nLILNextStm));
            } else if(nEndOfStmLIL > 0 && nEndOfStmLIL <= Params().MaxReorganizationDepth()){
                itLastLIL->second = nLIL;
                nLILLastStmSize = nLIL;
            }
        }

        //After working on copies, take lock back, update structures, gracefully leave lock and exit
        {
            LOCK(cs);
            mapStatementBIG = copyMapStatementBIG;
            mapStatementMID = copyMapStatementMID;
            mapStatementLIL = copyMapStatementLIL;

        }
        return true;
    }
    return false;
}

std::pair<int, int> CInfinitynodeMan::getLastStatementBySinType(int nSinType)
{
    if (nSinType == 10) return std::make_pair(nBIGLastStmHeight, nBIGLastStmSize);
    else if (nSinType == 5) return std::make_pair(nMIDLastStmHeight, nMIDLastStmSize);
    else if (nSinType == 1) return std::make_pair(nLILLastStmHeight, nLILLastStmSize);
    else return std::make_pair(0, 0);
}

std::string CInfinitynodeMan::getLastStatementString() const
{
    std::ostringstream info;
    info << nCachedBlockHeight << " "
            << "BIG: [" << mapStatementBIG.size() << " / " << nBIGLastStmHeight << ":" << nBIGLastStmSize << "] - "
            << "MID: [" << mapStatementMID.size() << " / " << nMIDLastStmHeight << ":" << nMIDLastStmSize << "] - "
            << "LIL: [" << mapStatementLIL.size() << " / " << nLILLastStmHeight << ":" << nLILLastStmSize << "]";

    return info.str();
}

std::string CInfinitynodeMan::getLastStatement()
{
    LOCK (cs);
    std::ostringstream info;
    info << " Height: ";
    if (nCachedBlockHeight < Params().GetConsensus().nDINActivationHeight) {return info.str();}
    info << nCachedBlockHeight;

    std::map<int, int>::iterator itBIG = mapStatementBIG.upper_bound(nCachedBlockHeight);
    std::map<int, int>::iterator itMID = mapStatementMID.upper_bound(nCachedBlockHeight);
    std::map<int, int>::iterator itLIL = mapStatementLIL.upper_bound(nCachedBlockHeight);


    if(nCachedBlockHeight == Params().GetConsensus().nDINActivationHeight) {
            info << " BIG:" << itBIG->first << "/" << itBIG->second << " " <<
                    " MID:" << itMID->first << "/" << itMID->second << " " <<
                    " LIL:" << itLIL->first << "/" << itLIL->second << " "
                    ;
    } else if (nCachedBlockHeight > Params().GetConsensus().nDINActivationHeight) {

            std::map<int, int>::iterator itLastBIG = --itBIG;
            std::map<int, int>::iterator itLastMID = --itMID;
            std::map<int, int>::iterator itLastLIL = --itLIL;

            info << " BIG:" << itBIG->first << "/" << itBIG->second << " " <<
                    " MID:" << itMID->first << "/" << itMID->second << " " <<
                    " LIL:" << itLIL->first << "/" << itLIL->second << " "
                    ;
    }
    return info.str();
}
/**
* Rank = 0 when node is expired
* Rank > 0 node is not expired, order by nHeight and
*
* called in CheckAndRemove
*/
std::map<int, CInfinitynode> CInfinitynodeMan::calculInfinityNodeRank(int nBlockHeight, int nSinType, bool updateList, bool flagExtCall)
{
    if (!flagExtCall)  AssertLockHeld(cs);
    else LOCK(cs);

    std::vector<std::pair<int, CInfinitynode*> > vecCInfinitynodeHeight;
    std::map<int, CInfinitynode> retMapInfinityNodeRank;

    for (auto& infpair : mapInfinitynodes) {
        CInfinitynode inf = infpair.second;
        //reinitial Rank to 0 all nodes of nSinType
        if (inf.getSINType() == nSinType && updateList == true) infpair.second.setRank(0);
        //put valid node in vector
        if (inf.getSINType() == nSinType && inf.getExpireHeight() >= nBlockHeight && inf.getHeight() < nBlockHeight)
        {
            vecCInfinitynodeHeight.push_back(std::make_pair(inf.getHeight(), &infpair.second));
        }
    }

    // Sort them low to high
    sort(vecCInfinitynodeHeight.begin(), vecCInfinitynodeHeight.end(), CompareIntValue());
    //update Rank at nBlockHeight
    int rank=1;
    for (std::pair<int, CInfinitynode*>& s : vecCInfinitynodeHeight) {
        auto it = mapInfinitynodes.find(s.second->vinBurnFund.prevout);
        if(updateList == true) it->second.setRank(rank);
        retMapInfinityNodeRank[rank] = *s.second;
        rank = rank + 1;
    }

    return retMapInfinityNodeRank;
}

/*
 * @return 0 or nHeight of reward
 */
int CInfinitynodeMan::isPossibleForLockReward(COutPoint burntx)
{
    LOCK(cs);

    CInfinitynode inf;
    if (!Get(infinitynodePeer.burntx, inf)) {
        LogPrint(BCLog::INFINITYLOCK,"CInfinityNodeLockReward::ProcessBlock -- Can not identify mypeer in list: %s\n", infinitynodePeer.burntx.ToStringFull());
        return false;
    }

    //not candidate => false
    {
        int nNodeSINtype = inf.getSINType();
        int nLastStmBySINtype = 0;
        int nLastStmSizeBySINtype = 0;
        if (nNodeSINtype == 10) {nLastStmBySINtype = nBIGLastStmHeight; nLastStmSizeBySINtype = nBIGLastStmSize;}
        if (nNodeSINtype == 5) {nLastStmBySINtype = nMIDLastStmHeight; nLastStmSizeBySINtype = nMIDLastStmSize;}
        if (nNodeSINtype == 1) {nLastStmBySINtype = nLILLastStmHeight; nLastStmSizeBySINtype = nLILLastStmSize;}
        LogPrint(BCLog::INFINITYMAN,"CInfinitynodeMan::isPossibleForLockReward -- info, SIN type: %d, Stm Height: %d, Stm size: %d, current Height: %d, node rank: %d\n",
             nNodeSINtype, nBIGLastStmHeight, nLastStmSizeBySINtype, nCachedBlockHeight, inf.getRank());
        //size of statement is not enough for call LockReward => false
        if (nLastStmSizeBySINtype <= Params().GetConsensus().nInfinityNodeCallLockRewardDeepth) {
            LogPrint(BCLog::INFINITYMAN,"CInfinitynodeMan::isPossibleForLockReward -- No, number node is not enough: %d, for SIN type: %d\n", nLastStmSizeBySINtype, nNodeSINtype);
            return 0;
        }
        else
        {
            //Case1: not receive reward in this Stm
            int nHeightReward = nLastStmBySINtype + inf.getRank() - 1;
            if (nCachedBlockHeight <= nHeightReward)
            {
                if ((nHeightReward - nCachedBlockHeight) <= Params().GetConsensus().nInfinityNodeCallLockRewardDeepth) {
                //if(nHeightReward == nCachedBlockHeight){
                    LogPrint(BCLog::INFINITYMAN,"CInfinitynodeMan::isPossibleForLockReward -- Yes, Stm height: %d, reward height: %d, current height: %d\n", nLastStmBySINtype, nHeightReward, nCachedBlockHeight);
                    return nHeightReward;
                }
                else{
                    LogPrint(BCLog::INFINITYMAN,"CInfinitynodeMan::isPossibleForLockReward -- No, Stm height: %d, reward height: %d, current height: %d\n", nLastStmBySINtype, nHeightReward, nCachedBlockHeight);
                    return 0;
                }
            }
            //Case2: received reward in this Stm
            else
            {
                //expired at end of this Stm => false
                if (inf.isRewardInNextStm(nLastStmBySINtype + nLastStmSizeBySINtype)) {
                    LogPrint(BCLog::INFINITYMAN,"CInfinitynodeMan::isPossibleForLockReward -- No, node expire in next STM: %d, expired height: %d\n", nLastStmBySINtype + nLastStmSizeBySINtype, inf.getExpireHeight());
                    return 0;
                }
                else
                {
                    //try to get rank for next Stm
                    int nextStmRank = 0;
                    std::map<int, CInfinitynode> mapInfinityNodeRank;
                    mapInfinityNodeRank = calculInfinityNodeRank(nLastStmBySINtype + nLastStmSizeBySINtype, nNodeSINtype, false);
                    for (std::pair<int, CInfinitynode> s : mapInfinityNodeRank) {
                        if (s.second.getBurntxOutPoint() == inf.getBurntxOutPoint()) {
                            nextStmRank = s.first;
                        }
                    }
                    int nHeightRewardNextStm = nLastStmBySINtype + nLastStmSizeBySINtype + nextStmRank - 1;
                    int call_temp = nHeightRewardNextStm - nCachedBlockHeight - Params().GetConsensus().nInfinityNodeCallLockRewardDeepth;
                    LogPrint(BCLog::INFINITYMAN,"CInfinitynodeMan::isPossibleForLockReward -- call for LockReward in %d block, next STM reward height: %d, current height: %d, next rank: %d\n",
                                 call_temp, nHeightRewardNextStm, nCachedBlockHeight, nextStmRank);
                    if ((nHeightRewardNextStm - nCachedBlockHeight) <= Params().GetConsensus().nInfinityNodeCallLockRewardDeepth) {
                        return nHeightRewardNextStm;
                    } else {
                        return 0;
                    }
                }
            }
        }
    }
}

bool CInfinitynodeMan::deterministicRewardAtHeightOnValidation(int nBlockHeight, int nSinType, CInfinitynode& infinitynodeRet)
{
    if (nBlockHeight < Params().GetConsensus().nInfinityNodeGenesisStatement) return false;

    //step1: copy mapStatement for nSinType
    std::map<int, int> mapStatementSinType = getStatementMap(nSinType);

    if (mapStatementSinType.size() == 0) {
        LogPrint(BCLog::INFINITYMAN,"CInfinitynodeMan::deterministicRewardAtHeight -- we've just start node. map of Stm is not built yet\n");
        return false;
    }

    int lastStatement = 0;
    int lastStatementSize = 0;

    std::map<int, int>::iterator it = mapStatementSinType.upper_bound(nBlockHeight);
    std::map<int, int>::iterator itLast = --it;

    lastStatement = itLast->first;
    lastStatementSize = itLast->second;

//>Sinovate spec
    int nDelta = Params().getNodeDelta(nBlockHeight);
    if (nBlockHeight <= Params().getDeltaChangeHeight() && nSinType==1 &&
       lastStatementSize > nDelta && ((nBlockHeight - lastStatement) >= nDelta)) return false;
//<Sinovate spec

    //update rank of node
    std::map<int, CInfinitynode> rankOfStatement = calculInfinityNodeRank(lastStatement, nSinType, true);
    if (rankOfStatement.empty()) {
        LogPrint(BCLog::INFINITYMAN,"CInfinitynodeMan::deterministicRewardAtHeightOnValidation -- can not calculate rank at %d for SinType: %d\n", lastStatement, nSinType);
        return false;
    }

    /*at the begin of network, lastStatementSize=1, each block is begin/end of Stm at the same time*/
    /*Rank begin at 1, not 0*/
    if (rankOfStatement.size() == 1) {
        //make sure that node is not expired at height
        if (rankOfStatement[1].getExpireHeight() < nBlockHeight) return false;

        infinitynodeRet = rankOfStatement[1];
        return true;
    }

    if ((nBlockHeight < lastStatement) || (rankOfStatement.size() < (nBlockHeight - lastStatement + 1))) {
        LogPrint(BCLog::INFINITYMAN,"CInfinitynodeMan::deterministicRewardAtHeightOnValidation -- out of range at lastStatement: %d, nBlockHeight: %d, size: %d\n", lastStatement, nBlockHeight, rankOfStatement.size());
        return false;
    }

    //make sure that node is not expired at height
    if (rankOfStatement[nBlockHeight - lastStatement + 1].getExpireHeight() < nBlockHeight) return false;
    infinitynodeRet = rankOfStatement[nBlockHeight - lastStatement + 1];
    return true;
}

bool CInfinitynodeMan::deterministicRewardAtHeight_V2(int nBlockHeight, int nSinType, CInfinitynode& infinitynodeRet)
{
    if (nBlockHeight < Params().GetConsensus().nInfinityNodeGenesisStatement) {
        return false;
    }

    //case 1: nBlockHeight is in pass, nBlockHeight < nCachedBlockHeight
    //case 2: nBlockHeight is in future, nBlockHeight > nCachedBlockHeight
    //Cond: lastStatementSize > 2 * nMaxReorganizationDepth and lastStatementSize > 2 * nInfinityNodeCallLockRewardDeepth
    //     with the condition above, the candidate
    if (nBlockHeight >= (nCachedBlockHeight + Params().MaxReorganizationDepth())) {
        LogPrint(BCLog::INFINITYMAN,"CInfinitynodeMan::deterministicRewardAtHeight_V2 -- cannot calcul the candidate too far in future, height: %d, current + reorg: %d\n",
                                    nBlockHeight, (nCachedBlockHeight + Params().MaxReorganizationDepth()));
        return false;
    }

    //step1: copy mapStatement for nSinType
    std::map<int, int> mapStatementSinType = getStatementMap(nSinType);

    if (mapStatementSinType.size() == 0) {
        LogPrint(BCLog::INFINITYMAN,"CInfinitynodeMan::deterministicRewardAtHeight_V2 -- we've just start node. map of Stm is not built yet\n");
        return false;
    }

    std::map<int,int>::iterator it;
    it = mapStatementSinType.upper_bound(nBlockHeight);
    --it;
    int lastStatement = it->first;
    int lastStatementSize = it->second;

    //find future candidate and for next STM
    if (nBlockHeight >= (lastStatement + lastStatementSize) && nBlockHeight >= nCachedBlockHeight) {
        lastStatement = lastStatement + lastStatementSize;
    }

    std::map<int, CInfinitynode> rankOfStatement = calculInfinityNodeRank(lastStatement, nSinType, false);
    if (rankOfStatement.empty()) {
        LogPrint(BCLog::INFINITYMAN,"CInfinitynodeMan::deterministicRewardAtHeight_V2 -- can not calculate rank at %d\n", lastStatement);
        return false;
    }

    /*at the begin of network, lastStatementSize=1, each block is begin/end of Stm at the same time*/
    if (rankOfStatement.size() == 1) {
        infinitynodeRet = rankOfStatement[1];
        return true;
    }

    if (rankOfStatement.size() < (nBlockHeight - lastStatement + 1)) {
        LogPrint(BCLog::INFINITYMAN,"CInfinitynodeMan::deterministicRewardAtHeight_V2 -- out of range at lastStatement:%d, nBlockHeight:%d, size:%d\n", lastStatement, nBlockHeight, rankOfStatement.size());
        return false;
    }

    infinitynodeRet = rankOfStatement[nBlockHeight - lastStatement + 1];
    return true;
}

/*
 * get Vector score of all NON EXPIRED SINtype for given nBlockHash
 */
bool CInfinitynodeMan::getScoreVector(const uint256& nBlockHash, int nSinType, int nBlockHeight, CInfinitynodeMan::score_pair_vec_t& vecScoresRet)
{
    vecScoresRet.clear();

    AssertLockHeld(cs);

    if (mapInfinitynodes.empty()) {
        LogPrint(BCLog::INFINITYMAN,"CInfinitynodeMan::getScoreVector -- Infinitynode map is empty.\n");
        return false;
    }

    // calculate scores for SIN type 10
    for (auto& infpair : mapInfinitynodes) {
        CInfinitynode inf = infpair.second;
        if (inf.getSINType() == nSinType  && inf.getExpireHeight() >= nBlockHeight && inf.getHeight() < nBlockHeight) {
            vecScoresRet.push_back(std::make_pair(inf.CalculateScore(nBlockHash), &infpair.second));
        }
    }

    sort(vecScoresRet.rbegin(), vecScoresRet.rend(), CompareNodeScore());
    return vecScoresRet.size() > 0;
}

/*
 * get score classement of given NODE SINtype
 */
bool CInfinitynodeMan::getNodeScoreAtHeight(const COutPoint& outpoint, int nSinType, int nBlockHeight, int& nScoreRet, ChainstateManager& chainman)
{
    nScoreRet = -1;

    LOCK(cs);

    uint256 nBlockHash = uint256();
    CBlockIndex* pindex  = chainman.ActiveChain()[nBlockHeight];
    nBlockHash = pindex->GetBlockHash();

    score_pair_vec_t vecScores;

    if (!getScoreVector(nBlockHash, nSinType, nBlockHeight, vecScores))
        return false;

    int nRank = 0;
    for (auto& scorePair : vecScores) {
        nRank++;
        if (scorePair.second->vinBurnFund.prevout == outpoint) {
            nScoreRet = nRank;
            return true;
        }
    }

    return false;
}

bool CInfinitynodeMan::getTopNodeScoreAtHeight(int nSinType, int nBlockHeight, int nTop, std::vector<CInfinitynode>& vecInfRet, ChainstateManager& chainman)
{
    vecInfRet.clear();

    LOCK(cs);

    uint256 nBlockHash = uint256();
    CBlockIndex* pindex  = chainman.ActiveChain()[nBlockHeight];
    nBlockHash = pindex->GetBlockHash();

    score_pair_vec_t vecScores;

    if (!getScoreVector(nBlockHash, nSinType, nBlockHeight, vecScores))
        return false;

    int count=0;
    for (auto& scorePair : vecScores) {
        if (count < nTop) {
            vecInfRet.push_back(*scorePair.second);
        }
        count++;
    }

    return true;
}

bool CInfinitynodeMan::isValidTopNode(const std::vector<COutPoint>& vOutpoint, int nSinType, int nRewardHeight, int nTop, CChainState& chainstate)
{
    LOCK(cs);

    uint256 nBlockHash = uint256();
    CBlockIndex* pindex  = chainstate.m_chain[nRewardHeight];
    nBlockHash = pindex->GetBlockHash();
    score_pair_vec_t vecScores;

    if (!getScoreVector(nBlockHash, nSinType, nRewardHeight, vecScores))
        return false;

    int count=0;
    int nValid=0;
    std::ostringstream streamInfo;
    for (auto& scorePair : vecScores) {
        count++;
        if (count > nTop) break;
        for (const auto &outpoint : vOutpoint)
        {
            if (scorePair.second->vinBurnFund.prevout == outpoint) nValid++;
        }
    }

    if (nValid == vOutpoint.size()) return true;
    else return false;
}

/*
 * get Rank classement of a Vector of Node at given Height
 */
std::string CInfinitynodeMan::getVectorNodeRankAtHeight(const std::vector<COutPoint>  &vOutpoint, int nSinType, int nBlockHeight)
{
    std::string ret="";

    LOCK(cs);

    std::vector<std::pair<int, CInfinitynode*> > vecCInfinitynodeHeight;
    std::map<int, CInfinitynode> retMapInfinityNodeRank;

    for (auto& infpair : mapInfinitynodes) {
        CInfinitynode inf = infpair.second;
        //put valid node in vector
        if (inf.getSINType() == nSinType && inf.getExpireHeight() >= nBlockHeight && inf.getHeight() < nBlockHeight)
        {
            vecCInfinitynodeHeight.push_back(std::make_pair(inf.getHeight(), &infpair.second));
        }
    }

    // Sort them low to high
    sort(vecCInfinitynodeHeight.begin(), vecCInfinitynodeHeight.end(), CompareIntValue());
    //update Rank at nBlockHeight

    std::ostringstream streamInfo;
    for (const auto &outpoint : vOutpoint)
    {
        LogPrint(BCLog::INFINITYMAN,"CInfinityNodeLockReward::%s -- find rank for %s\n", __func__, outpoint.ToStringFull());
        int nRank = 1;
        for (std::pair<int, CInfinitynode*>& s : vecCInfinitynodeHeight) {
            if (s.second->vinBurnFund.prevout == outpoint)
            {
                LogPrint(BCLog::INFINITYMAN,"CInfinityNodeLockReward::%s -- rank: %d\n", __func__, nRank);
                streamInfo << nRank << ";";
                break;
            }
            nRank++;
        }
    }

    ret = streamInfo.str();
    return ret;
}

void CInfinitynodeMan::UpdatedBlockTip(const CBlockIndex *pindex)
{
    //update the last Stm when new block connected
    {
        nCachedBlockHeight = pindex->nHeight;
        LogPrint(BCLog::INFINITYLOCK,"CInfinitynodeMan::UpdatedBlockTip -- nCachedBlockHeight=%d\n", nCachedBlockHeight);
    }
}

void CInfinitynodeMan::UpdateChainActiveHeight(int number)
{
    nCachedBlockHeight = number;
}

void CInfinitynodeMan::FlushStateToDisk()
{
    if (fReachedLastBlock) {
        CFlatDB<CInfinitynodeMan> flatdb5("infinitynode.dat", "magicInfinityNodeCache");
        flatdb5.Dump(infnodeman);
        CFlatDB<CInfinitynodeMeta> flatdb7("infinitynodemeta.dat", "magicInfinityMeta");
        flatdb7.Dump(infnodemeta);
    }
}
