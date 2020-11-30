// Copyright (c) 2018-2020 SIN developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SIN_INFINITYNODEMETA_H
#define SIN_INFINITYNODEMEYA_H

#include <key.h>
#include <validation.h>
#include <script/standard.h>
#include <key_io.h>

using namespace std;

class CInfinitynodeMeta;
class CMetadata;
class CMetahisto;

extern CInfinitynodeMeta infnodemeta;

class CMetahisto
{
public:
    int nHeightHisto{0};
    std::string pubkeyHisto="";
    CService serviceHisto{};

    CMetahisto() = default;

    CMetahisto(int Height, std::string pubkey, CService cService):
    nHeightHisto(Height),
    pubkeyHisto(pubkey),
    serviceHisto(cService){}

    SERIALIZE_METHODS(CMetahisto, obj)
    {
        READWRITE(obj.nHeightHisto);
        READWRITE(obj.pubkeyHisto);
        READWRITE(obj.serviceHisto);
    }

};

class CMetadata
{
private:
    std::string metaID;
    std::string metadataPublicKey;
    CService metadataService;
    int nMetadataHeight;
    int activeBackupAddress;
    std::vector<CMetahisto> vHisto;

public:
    CMetadata() :
        metaID(),
        metadataPublicKey(),
        metadataService(),
        nMetadataHeight(0),
        activeBackupAddress(0),
        vHisto()
        {}

    CMetadata(std::string metaIDIn, std::string sPublicKey, CService cService, int nHeight, int nActive) :
        metaID(metaIDIn),
        metadataPublicKey(sPublicKey),
        metadataService(cService),
        nMetadataHeight(nHeight),
        activeBackupAddress(nActive)
    {
        CMetahisto histo(nHeight, sPublicKey, cService);
        vHisto.push_back(histo);
    }

    SERIALIZE_METHODS(CMetadata, obj)
    {
        READWRITE(obj.metaID);
        READWRITE(obj.metadataPublicKey);
        READWRITE(obj.metadataService);
        READWRITE(obj.nMetadataHeight);
        READWRITE(obj.activeBackupAddress);
        READWRITE(obj.vHisto);
    }

    std::string getMetaPublicKey(){return metadataPublicKey;}
    CService getService(){return metadataService;}
    int getMetadataHeight(){return nMetadataHeight;}
    int getFlagActiveBackupAddress(){return activeBackupAddress;}
    std::string getMetaID(){return metaID;}
    std::vector<CMetahisto> getHistory(){return vHisto;}
    int getHistoSize(){return (int)vHisto.size();}

    void setMetadataHeight(int inHeight){nMetadataHeight = inHeight;};
    void setMetaPublicKey(std::string inKey){metadataPublicKey = inKey;};
    void setService(CService inService){metadataService = inService;};
    void setBackupAddress(int nActive){activeBackupAddress = nActive;};
    void addHisto(CMetahisto inHisTo){vHisto.push_back(inHisTo);}
    void removeHisto(CMetahisto inHisTo);
    CMetahisto getLastHisto();
};

class CInfinitynodeMeta
{
private:
    static const std::string SERIALIZATION_VERSION_STRING;
    // critical section to protect the inner data structures
    mutable RecursiveMutex cs;
    // Keep track of current block height
    int nCachedBlockHeight;
public:
    std::map<std::string, CMetadata> mapNodeMetadata;

    CInfinitynodeMeta();

    SERIALIZE_METHODS(CInfinitynodeMeta, obj)
    {
        std::string strVersion;
        if(ser_action.ForRead()) {
            READWRITE(strVersion);
        }
        else {
            strVersion = SERIALIZATION_VERSION_STRING;
            READWRITE(strVersion);
        }
        READWRITE(obj.mapNodeMetadata);
    }

    void Clear();
    bool Add(CMetadata &meta);
    bool Remove(CMetadata &meta);
    bool Has(std::string  metaID);
    CMetadata Find(std::string  metaID);
    bool Get(std::string  nodePublicKey, CMetadata& meta);
    std::map<std::string, CMetadata> GetFullNodeMetadata() { LOCK(cs); return mapNodeMetadata; }

    bool RemoveMetaFromBlock(const CBlock& block, CBlockIndex* pindex, CCoinsViewCache& view, const CChainParams& chainparams);

    bool metaScan(int nHeight);
    bool setActiveBKAddress(std::string  metaID);

    std::string ToString() const;
    /// This is dummy overload to be used for dumping/loading mncache.dat
    void CheckAndRemove() {}
};
#endif // SIN_INFINITYNODERSV_H