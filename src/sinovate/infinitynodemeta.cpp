// Copyright (c) 2018-2020 SIN developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <sinovate/infinitynodemeta.h>
#include <sinovate/infinitynodeman.h>
#include <sinovate/infinitynodetip.h>
#include <sinovate/flat-database.h>

#include <script/standard.h>
#include <blockfilter.h>
#include <validation.h>

CInfinitynodeMeta infnodemeta;

const std::string CInfinitynodeMeta::SERIALIZATION_VERSION_STRING = "CInfinitynodeMeta-Version-1";

void CMetadata::removeHisto(CMetahisto inHisTo)
{
    for(auto it = vHisto.begin(); it != vHisto.end(); ){
        if( (*it).nHeightHisto == inHisTo.nHeightHisto && (*it).pubkeyHisto == inHisTo.pubkeyHisto && (*it).serviceHisto == inHisTo.serviceHisto ) {
            it = vHisto.erase(it);
        } else {
            ++it;
        }
    }
}

CMetahisto CMetadata::getLastHisto()
{
    int v_Height = 0;
    CMetahisto histo;
    for(auto& v : vHisto){
        if(v.nHeightHisto > v_Height){
            v_Height = v.nHeightHisto;
            histo = v;
        }
    }
    return histo;
}

CInfinitynodeMeta::CInfinitynodeMeta()
: cs(),
  mapNodeMetadata()
{}

void CInfinitynodeMeta::Clear()
{
    LOCK(cs);
    mapNodeMetadata.clear();
}

//call in connecttip
bool CInfinitynodeMeta::Add(CMetadata &meta)
{
    LOCK(cs);
    LogPrint(BCLog::INFINITYMETA,"CInfinitynodeMeta::Add() New Metadata from %s at height: %d\n", meta.getMetaID(), meta.getMetadataHeight());
    auto it = mapNodeMetadata.find(meta.getMetaID());
    if(it == mapNodeMetadata.end()){
        LogPrint(BCLog::INFINITYMETA,"CInfinitynodeMeta::Add() 1st metadata from %s\n", meta.getMetaID());
        mapNodeMetadata[meta.getMetaID()] = meta;
        return true;
    } else {
        CMetadata m = it->second;
        if(m.getMetaID() == meta.getMetaID() && meta.getMetadataHeight() >  m.getMetadataHeight()){
            LogPrint(BCLog::INFINITYMETA,"CInfinitynodeMeta::Add() New metadata %s, at height: %d\n", meta.getMetaID(),  meta.getMetadataHeight());
            //we have a new metadata. we need check the distant between 2 update befor add it in histo
            int nHeight = meta.getMetadataHeight();
            std::string sPublicKey = meta.getMetaPublicKey();
            CService cService = meta.getService();
            CAddress addMeta = CAddress(cService, NODE_NETWORK);

            if(nHeight < m.getMetadataHeight() + Params().MaxReorganizationDepth() * 2){
                int nWait = m.getMetadataHeight() + Params().MaxReorganizationDepth() * 2 - nHeight;
                LogPrint(BCLog::INFINITYMETA,"CInfinitynodeMeta::Add() Cannot update metadata now. Please update after %d blocks\n", nWait);
                return false;
            } else {
                int nPass = nHeight - m.getMetadataHeight() - Params().MaxReorganizationDepth() * 2;
                LogPrint(BCLog::INFINITYMETA,"CInfinitynodeMeta::Add() Update metadata now. %d blocks have passed from last update height: %d\n", nPass, m.getMetadataHeight());

                //make sure that PublicKey and IP are not using in network for different metaID
                bool fCheckExistant = false;
                if (Params().NetworkIDString() != CBaseChainParams::REGTEST) {
                    for (auto& infpair : mapNodeMetadata) {
                        CMetadata m = infpair.second;
                        CAddress add = CAddress(infpair.second.getService(), NODE_NETWORK);

                        if (m.getMetaID() != meta.getMetaID() && (m.getMetaPublicKey() == sPublicKey || addMeta.ToStringIP() == add.ToStringIP())) {
                            fCheckExistant = true;
                        }
                    }
                }

                if(fCheckExistant) {
                    LogPrint(BCLog::INFINITYMETA,"CInfinitynodeMeta::Add() Cannot update metadata now. PubKey or IP existant in network\n");
                    return false;
                } else {
                    CMetahisto histo(nHeight, sPublicKey, cService);
                    mapNodeMetadata[meta.getMetaID()].addHisto(histo);
                    mapNodeMetadata[meta.getMetaID()].setMetadataHeight(nHeight);
                    mapNodeMetadata[meta.getMetaID()].setMetaPublicKey(sPublicKey);
                    mapNodeMetadata[meta.getMetaID()].setService(cService);
                    return true;
                }
            }
        }else{
            LogPrint(BCLog::INFINITYMETA,"CInfinitynodeMeta::meta nHeight(%d) is lower than current height %d\n", meta.getMetadataHeight(), m.getMetadataHeight());
            return false;
        }
    }
}

//call in disconnecttip
bool CInfinitynodeMeta::Remove(CMetadata &meta)
{
    LOCK(cs);
    LogPrint(BCLog::INFINITYMETA,"CInfinitynodeMeta::remove Metadata %s %d\n", meta.getMetaID(), meta.getMetadataHeight());
    auto it = mapNodeMetadata.find(meta.getMetaID());
    if(it == mapNodeMetadata.end()){
        return true;
    } else {
        CMetadata m = it->second;
        if(m.getMetaID() == meta.getMetaID() && m.getHistoSize() == 1){
            //we have only 1 entry => remove
            mapNodeMetadata.erase(meta.getMetaID());
            return true;
        } else if (m.getMetaID() == meta.getMetaID() && m.getHistoSize() > 1) {
            //check if input meta is the last
            if(meta.getMetadataHeight() == m.getMetadataHeight() && meta.getMetaPublicKey() == m.getMetaPublicKey() && meta.getService() == m.getService())
            {
                int nHeight = meta.getMetadataHeight();
                std::string sPublicKey = meta.getMetaPublicKey();
                CService cService = meta.getService();
                CMetahisto histo(nHeight, sPublicKey, cService);
                mapNodeMetadata[meta.getMetaID()].removeHisto(histo);
                CMetahisto lastHisto = mapNodeMetadata[meta.getMetaID()].getLastHisto();
                mapNodeMetadata[meta.getMetaID()].setMetadataHeight(lastHisto.nHeightHisto);
                mapNodeMetadata[meta.getMetaID()].setMetaPublicKey(lastHisto.pubkeyHisto);
                mapNodeMetadata[meta.getMetaID()].setService(lastHisto.serviceHisto);
                return true;
            } else {
                LogPrint(BCLog::INFINITYMETA,"CInfinitynodeMeta:: input Metadata is not the last\n");
                return false;
            }
        } else {
            return false;
        }
    }
}

//call in disconnecttip
bool CInfinitynodeMeta::RemoveMetaFromBlock(const CBlock& block, CBlockIndex* pindex, CCoinsViewCache& view, const CChainParams& chainparams)
{
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
                                        if (check == 3){
                                            //Address payee: we known that there is only 1 input
                                            const Coin& coin = view.AccessCoin(tx.vin[0].prevout);

                                            CTxDestination addressBurnFund;
                                            if(!ExtractDestination(coin.out.scriptPubKey, addressBurnFund)){
                                                LogPrint(BCLog::INFINITYMAN,"CInfinitynodeMeta::metaScan -- False when extract payee from BurnFund tx.\n");
                                                return false;
                                            }

                                            std::ostringstream streamInfo;
                                            streamInfo << EncodeDestination(addressBurnFund) << "-" << burnTxID;

                                            LogPrint(BCLog::INFINITYMAN,"CInfinitynodeMeta:: meta update: %s, %s, %s\n", 
                                                         streamInfo.str(), publicKeyString, service.ToString());
                                            int avtiveBK = 0;
                                            CMetadata meta = CMetadata(streamInfo.str(), publicKeyString, service, pindex->nHeight, avtiveBK);
                                            Remove(meta);
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

bool CInfinitynodeMeta::Has(std::string  metaID)
{
    LOCK(cs);
    return mapNodeMetadata.find(metaID) != mapNodeMetadata.end();
}

CMetadata CInfinitynodeMeta::Find(std::string  metaID)
{
    LOCK(cs);
    CMetadata meta;
    auto it = mapNodeMetadata.find(metaID);
    if(it != mapNodeMetadata.end()){meta = it->second;}
    return meta;
}

bool CInfinitynodeMeta::Get(std::string  nodePublicKey, CMetadata& meta)
{
    bool res = false;
    LOCK(cs);
    for (auto& infpair : mapNodeMetadata) {
        CMetadata m = infpair.second;
        if(m.getMetaPublicKey() == nodePublicKey){
            meta = m;
            res = true;
        }
    }
    return res;
}

bool CInfinitynodeMeta::metaScan(int nBlockHeight)
{
    Clear();

    LOCK(cs_main);

    LogPrint(BCLog::INFINITYMETA,"CInfinitynodeMeta::metaScan -- Cleared map. Size is %d at height: %d\n", (int)mapNodeMetadata.size(), nBlockHeight);
    int lastHeight = ::ChainActive().Height();
    if (nBlockHeight <= Params().GetConsensus().nInfinityNodeGenesisStatement) return false;
    if (nBlockHeight > lastHeight) nBlockHeight = lastHeight;

    CBlockIndex* pindex  = ::ChainActive()[nBlockHeight];
    CBlockIndex* prevBlockIndex = pindex;

    while (prevBlockIndex->nHeight >= Params().GetConsensus().nInfinityNodeGenesisStatement)
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
                        //Send to Metadata
                        if (whichType == TxoutType::TX_BURN_DATA && Params().GetConsensus().cMetadataAddress == EncodeDestination(PKHash(uint160(vSolutions[0]))))
                        {
                          //Amount for UpdateMeta
                            if ( (Params().GetConsensus().nInfinityNodeUpdateMeta - 1) * COIN <= out.nValue
                                 && out.nValue <= (Params().GetConsensus().nInfinityNodeUpdateMeta) * COIN){
                                if (vSolutions.size() == 2){
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
                                        //1st position: publicKey
                                        if (i==0) {
                                            publicKeyString = s;
                                            std::vector<unsigned char> tx_data = DecodeBase64(publicKeyString.c_str());
                                            LogPrint(BCLog::INFINITYMETA,"CInfinitynodeMeta::metaScan -- publicKey: %s\n", publicKeyString);
                                            CPubKey decodePubKey(tx_data.begin(), tx_data.end());
                                            if (decodePubKey.IsValid()) {
                                                check++;
                                            }else{
                                                LogPrint(BCLog::INFINITYMETA,"CInfinitynodeMeta::metaScan -- ERROR: publicKey is not valid: %s\n", publicKeyString);
                                            }
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
                                        if (check == 3){
                                            //prevBlockIndex->nHeight
                                            const CTxIn& txin = tx->vin[0];
                                            int index = txin.prevout.n;

                                            CTransactionRef prevtx;
                                            uint256 hashblock;
                                            if(!GetTransaction(txin.prevout.hash, prevtx, hashblock)) {
                                                LogPrint(BCLog::INFINITYMETA,"CInfinitynodeMeta::metaScan -- PrevBurnFund tx is not in block.\n");
                                                return false;
                                            }

                                            CTxDestination addressBurnFund;
                                            if(!ExtractDestination(prevtx->vout[index].scriptPubKey, addressBurnFund)){
                                                LogPrint(BCLog::INFINITYMETA,"CInfinitynodeMeta::metaScan -- False when extract payee from BurnFund tx.\n");
                                                return false;
                                            }

                                            std::ostringstream streamInfo;
                                            streamInfo << EncodeDestination(addressBurnFund) << "-" << burnTxID;

                                            LogPrint(BCLog::INFINITYMETA,"CInfinitynodeMeta:: meta update: %s, %s, %s, %d\n", 
                                                         streamInfo.str(), publicKeyString, service.ToString(), prevBlockIndex->nHeight);
                                            int avtiveBK = 0;
                                            int nHeight = prevBlockIndex->nHeight;
                                            CMetadata meta = CMetadata(streamInfo.str(), publicKeyString, service, nHeight, avtiveBK);
                                            Add(meta);
                                        }
                                        i++;
                                    }
                                }
                            }
                        } // Send to metadata
                    }
                }
            }
        } else {
            LogPrint(BCLog::INFINITYNODE, "CInfinitynodeMeta::metaScan -- can not read block from disk\n");
            return false;
        }
        // continue with previous block
        prevBlockIndex = prevBlockIndex->pprev;
    }

    CFlatDB<CInfinitynodeMeta> flatdb7("infinitynodemeta.dat", "magicInfinityMeta");
    flatdb7.Dump(infnodemeta);

    return true;
}

bool CInfinitynodeMeta::setActiveBKAddress(std::string  metaID)
{
    LOCK(cs);
    auto it = mapNodeMetadata.find(metaID);
    if(it == mapNodeMetadata.end()){
        return false;
    } else {
        int active = 1;
        mapNodeMetadata[metaID].setBackupAddress(active);
        return true;
    }
}

std::string CInfinitynodeMeta::ToString() const
{
    std::ostringstream info;
    LOCK(cs);
    info << "Metadata: " << (int)mapNodeMetadata.size() << "\n";
    for (auto& infpair : mapNodeMetadata) {
        CMetadata m = infpair.second;
        info << " MetadataID: " << infpair.first << " PublicKey: " << m.getMetaPublicKey();
    }

    return info.str();
}
