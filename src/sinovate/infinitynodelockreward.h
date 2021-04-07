// Copyright (c) 2018-2019 SIN developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SIN_INFINITYNODELOCKREWARD_H
#define SIN_INFINITYNODELOCKREWARD_H

#include <sinovate/infinitynodeman.h>
#include <string>
#include <banman.h>

class CInfinityNodeLockReward;
class CLockRewardRequest;
class CVerifyRequest;
class CLockRewardCommitment;

extern CInfinityNodeLockReward inflockreward;

static const int MIN_INFINITYNODE_PAYMENT_PROTO_VERSION = 250003;
static const int LIMIT_MEMORY = 10; //nblocks

class CLockRewardRequest
{
public:
    int nRewardHeight{0};
    CTxIn burnTxIn{};
    int nSINtype{0};
    int nLoop{0};
    std::vector<unsigned char> vchSig{};
    bool fFinal = false;

    CLockRewardRequest();
    CLockRewardRequest(int Height, COutPoint burnTxIn, int nSINtype, int loop = 0);

    SERIALIZE_METHODS(CLockRewardRequest, obj)
    {
        READWRITE(obj.nRewardHeight);
        READWRITE(obj.burnTxIn);
        READWRITE(obj.nSINtype);
        READWRITE(obj.nLoop);
        READWRITE(obj.vchSig);
        READWRITE(obj.fFinal);
    }

    uint256 GetHash() const
    {
        CHashWriter ss(SER_GETHASH, PROTOCOL_VERSION);
        ss << nRewardHeight;
        ss << burnTxIn;
        ss << nSINtype;
        ss << nLoop;
        return ss.GetHash();
    }

    bool Sign(const CKey& keyInfinitynode, const CPubKey& pubKeyInfinitynode);
    bool CheckSignature(CPubKey& pubKeyInfinitynode, int& nDos) const;
    bool IsValid(CNode* pnode, int nValidationHeight, std::string& strError, CConnman& connman, int& nDos) const;
    void Relay(CConnman& connman);
};

class CVerifyRequest
{
public:
    CTxIn vin1{};
    CTxIn vin2{};
    CService addr{};
    int nBlockHeight{};
    uint256 nHashRequest{};
    std::vector<unsigned char> vchSig1{};
    std::vector<unsigned char> vchSig2{};

    CVerifyRequest() = default;

    CVerifyRequest(CService addrToConnect, COutPoint myPeerBurnTxIn, COutPoint candidateBurnTxIn, int nBlockHeight, uint256 nRequest) :
        vin1(CTxIn(myPeerBurnTxIn)),
        vin2(CTxIn(candidateBurnTxIn)),
        addr(addrToConnect),
        nBlockHeight(nBlockHeight),
        nHashRequest(nRequest)
    {}

    SERIALIZE_METHODS(CVerifyRequest, obj)
    {
        READWRITE(obj.vin1);
        READWRITE(obj.vin2);
        READWRITE(obj.addr);
        READWRITE(obj.nBlockHeight);
        READWRITE(obj.nHashRequest);
        READWRITE(obj.vchSig1);
        READWRITE(obj.vchSig2);
    }

    uint256 GetHash() const
    {
        CHashWriter ss(SER_GETHASH, PROTOCOL_VERSION);
        ss << vin1;
        ss << vin2;
        ss << addr;
        ss << nBlockHeight;
        ss << nHashRequest;
        return ss.GetHash();
    }

    void Relay(CConnman& connman) const
    {
        CInv inv(MSG_INFVERIFY, GetHash());
        connman.RelayInv(inv);
    }
};

class CLockRewardCommitment
{
public:
    CTxIn vin{};//Top node ID
    uint256 nHashRequest;//LockRewardRequest hash
    int nRewardHeight{};
    std::vector<unsigned char> vchSig{};
    CKey random;//r of schnorr Musig
    CPubKey pubkeyR;

    CLockRewardCommitment();
    CLockRewardCommitment(uint256 nRequest, int nRewardHeight, COutPoint myPeerBurnTxIn, CKey key);

    SERIALIZE_METHODS(CLockRewardCommitment, obj)
    {
        READWRITE(obj.vin);
        READWRITE(obj.nHashRequest);
        READWRITE(obj.pubkeyR);
        READWRITE(obj.nRewardHeight);
        READWRITE(obj.vchSig);
    }

    uint256 GetHash() const
    {
        CHashWriter ss(SER_GETHASH, PROTOCOL_VERSION);
        ss << vin;
        ss << nHashRequest;
        ss << nRewardHeight;
        return ss.GetHash();
    }

    bool Sign(const CKey& keyInfinitynode, const CPubKey& pubKeyInfinitynode);
    bool CheckSignature(CPubKey& pubKeyInfinitynode, int &nDos);
    void Relay(CConnman& connman);
};

class CGroupSigners
{
public:
    CTxIn vin{};//candidate ID
    uint256 nHashRequest;//LockRewardRequest hash
    int nGroup;
    int nRewardHeight{};
    std::string signersId;
    std::vector<unsigned char> vchSig{};

    CGroupSigners();
    CGroupSigners(COutPoint myPeerBurnTxIn, uint256 nRequest, int nGroup, int nRewardHeight, std::string signersId);

    SERIALIZE_METHODS(CGroupSigners, obj)
    {
        READWRITE(obj.vin);
        READWRITE(obj.nHashRequest);
        READWRITE(obj.nGroup);
        READWRITE(obj.nRewardHeight);
        READWRITE(obj.signersId);
        READWRITE(obj.vchSig);
    }

    uint256 GetHash() const
    {
        CHashWriter ss(SER_GETHASH, PROTOCOL_VERSION);
        ss << vin;
        ss << nHashRequest;
        ss << nGroup;
        ss << nRewardHeight;
        return ss.GetHash();
    }

    bool Sign(const CKey& keyInfinitynode, const CPubKey& pubKeyInfinitynode);
    bool CheckSignature(CPubKey& pubKeyInfinitynode, int &nDos);
    void Relay(CConnman& connman);
};

class CMusigPartialSignLR
{
public:
    CTxIn vin{};//Signer ID
    uint256 nHashGroupSigners;//LockRewardRequest hash
    int nRewardHeight{};
    std::vector<unsigned char> vchMusigPartialSign{};
    std::vector<unsigned char> vchSig{};

    CMusigPartialSignLR();
    CMusigPartialSignLR(COutPoint myPeerBurnTxIn, uint256 nGroupSigners, int inHeight, unsigned char *cMusigPartialSign);

    SERIALIZE_METHODS(CMusigPartialSignLR, obj)
    {
        READWRITE(obj.vin);
        READWRITE(obj.nHashGroupSigners);
        READWRITE(obj.nRewardHeight);
        READWRITE(obj.vchMusigPartialSign);
        READWRITE(obj.vchSig);
    }

    uint256 GetHash() const
    {
        CHashWriter ss(SER_GETHASH, PROTOCOL_VERSION);
        ss << vin;
        ss << nHashGroupSigners;
        ss << nRewardHeight;
        return ss.GetHash();
    }

    bool Sign(const CKey& keyInfinitynode, const CPubKey& pubKeyInfinitynode);
    bool CheckSignature(CPubKey& pubKeyInfinitynode, int &nDos);
    void Relay(CConnman& connman);
};

class CInfinityNodeLockReward
{
private:

    mutable RecursiveMutex cs;
    std::map<uint256, CLockRewardRequest> mapLockRewardRequest;
    std::map<uint256, CLockRewardCommitment> mapLockRewardCommitment;
    std::map<uint256, CGroupSigners> mapLockRewardGroupSigners;
    std::map<uint256, CMusigPartialSignLR> mapPartialSign;

    std::map<uint256, std::vector<COutPoint>> mapSigners; //list of signers for my request only, uint256 = currentLockRequestHash
    std::map<uint256, std::vector<CMusigPartialSignLR>> mapMyPartialSigns; //list of signers for my request only, uint256 = hashGroupSigners
    std::map<int, uint256> mapSigned; // signed Musig for nRewardHeight and hashGroupSigners
    std::vector<std::string> mapBadSignersConnection; //
    // Keep track of current block height
    int nCachedBlockHeight;
    // Keep track my current LockRequestHash and all related informations
    int nFutureRewardHeight;
    uint256 currentLockRequestHash;
    int nGroupSigners; //number of group signer found for currentLockRequest
    bool fMusigBuilt;

public:

    CInfinityNodeLockReward() : nCachedBlockHeight(0) {}

    SERIALIZE_METHODS(CInfinityNodeLockReward, obj)
    {
        READWRITE(obj.mapLockRewardRequest);
        READWRITE(obj.mapLockRewardCommitment);
    }

    void Clear();

    bool AlreadyHave(const uint256& hash);

    //LockRewardRequest
    bool AddLockRewardRequest(const CLockRewardRequest& lockRewardRequest);
    void RemoveLockRewardRequest(const CLockRewardRequest& lockRewardRequest);
    bool HasLockRewardRequest(const uint256& reqHash);
    bool GetLockRewardRequest(const uint256& reqHash, CLockRewardRequest& lockRewardRequestRet);

    //process consensus request message
    bool CheckLockRewardRequest(CNode* pfrom, const CLockRewardRequest& lockRewardRequestRet, CConnman& connman, int nBlockHeight, int& nDos);
    bool CheckMyPeerAndSendVerifyRequest(CNode* pfrom, const CLockRewardRequest& lockRewardRequestRet, CConnman& connman, int& nDos);

    //Verify node at IP
    bool SendVerifyReply(CNode* pnode, CVerifyRequest& vrequest, CConnman& connman, int& nDos);
    bool CheckVerifyReply(CNode* pnode, CVerifyRequest& vrequest, CConnman& connman, int& nDos);

    //commitment
    bool AddCommitment(const CLockRewardCommitment& commitment);
    bool SendCommitment(const uint256& reqHash, int nRewardHeight, CConnman& connman);
    bool CheckCommitment(CNode* pnode, const CLockRewardCommitment& commitment, int& nDos);
    bool GetLockRewardCommitment(const uint256& reqHash, CLockRewardCommitment& commitment);

    //group signer
    void AddMySignersMap(const CLockRewardCommitment& commitment);
    bool AddGroupSigners(const CGroupSigners& gs);
    bool GetGroupSigners(const uint256& reqHash, CGroupSigners& gsigners);
    bool FindAndSendSignersGroup(CConnman& connman);
    bool CheckGroupSigner(CNode* pnode, const CGroupSigners& gsigners, int& nDos);

    //Schnorr Musig
    bool MusigPartialSign(CNode* pnode, const CGroupSigners& gsigners, CConnman& connman);
    bool AddMusigPartialSignLR(const CMusigPartialSignLR& ps);
    bool GetMusigPartialSignLR(const uint256& psHash, CMusigPartialSignLR& ps);
    bool CheckMusigPartialSignLR(CNode* pnode, const CMusigPartialSignLR& ps, int& nDos);
    void AddMyPartialSignsMap(const CMusigPartialSignLR& ps);
    bool FindAndBuildMusigLockReward();

    //register LockReward by send tx if i am node of candidate
    bool AutoResigterLockReward(std::string sLR, std::string& strErrorRet, const COutPoint& infCheck);

    //Check CheckLockRewardRegisterInfo for candidate is OK or KO
    bool CheckLockRewardRegisterInfo(std::string sLR, std::string& strErrorRet, const COutPoint& infCheck);

    //remove unused data to avoid memory issue
    //call int init.cpp
    void CheckAndRemove(CConnman& connman);
    std::string GetMemorySize();

    //Connection
    void TryConnectToMySigners(int rewardHeight, CConnman& connman);
    //call in UpdatedBlockTip
    bool ProcessBlock(int nBlockHeight, CConnman& connman);
    //call in net_processing.cpp (PeerManager) when node receive INV
    void ProcessMessage(CNode* pfrom, const std::string& strCommand, CDataStream& vRecv, CConnman& connman, int& nDos);
    void ProcessDirectMessage(CNode* pfrom, const std::string& strCommand, CDataStream& vRecv, CConnman& connman);
    //call in dsnotificationinterface.cpp when node connect a new block
    void UpdatedBlockTip(const CBlockIndex *pindex, CConnman& connman);
};
// validation
bool LockRewardValidation(const int nBlockHeight, const CTransactionRef txNew);
// miner
void FillBlock(CMutableTransaction& txNew, int nBlockHeight, bool IsProofOfStake = false);

class ECCMusigHandle
{
    static int refcount;

public:
    ECCMusigHandle();
    ~ECCMusigHandle();
};

void ECC_MusigStart(void);
void ECC_MusigStop(void);
#endif // SIN_INFINITYNODELOCKREWARD_H
