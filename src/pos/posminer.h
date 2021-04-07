// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Copyright (c) 2016-2020 The PIVX developers
// Copyright (c) 2015-2020 The SINOVATE developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or https://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_POSMINER_H
#define BITCOIN_POSMINER_H

#include <chain.h>
#include <threadinterrupt.h>
#include <node/context.h>
#include <util/time.h>

#include <stdint.h>


class CBlockIndex;
class CWallet;
class CStakeableOutput;
class ScriptPubKeyMan;

// proof-of-stake

/** Record info about last stake attempt:
 *  - tipBlock       index of the block on top of which last stake attempt was made
 *  - nTime          time slot of last attempt
 *  - nTries         number of UTXOs hashed during last attempt
 *  - nCoins         number of stakeable utxos during last attempt
**/
class CStakerStatus
{
private:
    const CBlockIndex* tipBlock{nullptr};
    int64_t nTime{0};
    int nTries{0};
    int nCoins{0};

public:
    // Get
    const CBlockIndex* GetLastTip() const { return tipBlock; }
    uint256 GetLastHash() const { return (GetLastTip() == nullptr ? uint256() : GetLastTip()->GetBlockHash()); }
    int GetLastHeight() const { return (GetLastTip() == nullptr ? 0 : GetLastTip()->nHeight); }
    int GetLastCoins() const { return nCoins; }
    int GetLastTries() const { return nTries; }
    int64_t GetLastTime() const { return nTime; }
    // Set
    void SetLastCoins(const int coins) { nCoins = coins; }
    void SetLastTries(const int tries) { nTries = tries; }
    void SetLastTip(const CBlockIndex* lastTip) { tipBlock = lastTip; }
    void SetLastTime(const uint64_t lastTime) { nTime = lastTime; }
    void SetNull()
    {
        SetLastCoins(0);
        SetLastTries(0);
        SetLastTip(nullptr);
        SetLastTime(0);
    }
    // Check whether staking status is active (last attempt earlier than 30 seconds ago)
    bool IsActive() const { return (nTime + 30) >= GetTime(); }
};

// Staker status (last hashed block and time)
extern std::unique_ptr<CStakerStatus> pStakerStatus;

#ifdef ENABLE_WALLET
// Class for keeping node ctx refs, storing connman, chainman and pool.
class StakerCtx
{
public:
    StakerCtx(CConnman& connman, ChainstateManager& chainman, CTxMemPool& pool);

    void CheckForCoins(CWallet* pwallet, std::vector<CStakeableOutput>* availableCoins);

    void StakerPipe();
    void StartStaker();
    void InterruptStaker();
    void StopStaker();

private:
    CConnman& m_connman;
    ChainstateManager& m_chainman;
    CTxMemPool& m_mempool;

    CThreadInterrupt g_posminer_interrupt;
    std::thread g_posminer_thread;

};
#endif // ENABLE_WALLET

//! proof-of-stake: Creates a coinstake if any UTXO it gets fed meets protocol
bool CreateCoinStake(CWallet* pwallet, const CBlockIndex* pindexPrev,
                    unsigned int nBits,
                    CMutableTransaction& txNew,
                    int64_t& nTxNewTime,
                    std::vector<CStakeableOutput>* availableCoins,
                    CStakerStatus* pStakerStatus);

void InitStakerStatus();

#endif // BITCOIN_POSMINER_H
