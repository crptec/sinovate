// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Copyright (c) 2014-2015 The Dash developers
// Copyright (c) 2011-2013 The PPCoin developers
// Copyright (c) 2013-2014 The NovaCoin Developers
// Copyright (c) 2014-2018 The BlackCoin Developers
// Copyright (c) 2015-2020 The PIVX developers
// Copyright (c) 2015-2020 The SINOVATE developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or https://www.opensource.org/licenses/mit-license.php.

#include <pos/posminer.h>

#include <chainparams.h>
#include <miner.h>
#include <node/context.h>
#include <pos/pos.h>
#include <pos/stakeinput.h>
#include <pos/threadutils.h>
#include <policy/policy.h>
#include <rpc/blockchain.h>
#include <script/sign.h>
#include <shutdown.h>
#include <timedata.h>
#include <wallet/scriptpubkeyman.h>
#include <sinovate/infinitynodelockreward.h>
#include <validation.h>
#include <util/moneystr.h>

#ifdef ENABLE_WALLET
#include "wallet/wallet.h"
#endif

#ifdef ENABLE_WALLET

std::unique_ptr<CStakerStatus> pStakerStatus = nullptr;

StakerCtx::StakerCtx(CConnman& connman, ChainstateManager& chainman, CTxMemPool& pool)
    : m_connman(connman),
      m_chainman(chainman),
      m_mempool(pool)
{
}
//////////////////////////////////////////////////////////////////////////////
//
// Internal PoS miner
//
bool CreateCoinStake(CWallet* pwallet, 
        const CBlockIndex* pindexPrev,
        unsigned int nBits,
        CMutableTransaction& txNew,
        int64_t& nTxNewTime,
        std::vector<CStakeableOutput>* availableCoins,
        CStakerStatus* pStakerStatus) 
{

    int nHeight = pindexPrev->nHeight + 1;

    // Mark coin stake transaction
    txNew.vin.clear();
    txNew.vout.clear();
    txNew.vout.emplace_back(0, CScript());

    // update staker status (hash)
    pStakerStatus->SetLastTip(pindexPrev);
    pStakerStatus->SetLastCoins((int) availableCoins->size());

    // Kernel Search
    CAmount nCredit;
    CScript scriptPubKeyKernel;
    bool fKernelFound = false;
    int nAttempts = 0;
    CTxOut outProvider;
    for (auto it = availableCoins->begin(); it != availableCoins->end();) {
        COutPoint outPoint = COutPoint(it->tx->GetHash(), it->i);
        CSinStake stakeInput(it->tx->tx->vout[it->i],
                             outPoint,
                             it->pindex);

        // New block came in, move on
        {
            WAIT_LOCK(g_best_block_mutex, lock);
            if (g_best_block != pindexPrev->GetBlockHash() && g_best_block != uint256{0}) {
                return false;
            }
        }

        // Make sure the stake input hasn't been spent since last check
        
        if (WITH_LOCK(pwallet->cs_wallet, return pwallet->IsSpent(outPoint.hash, outPoint.n))) {
            // remove it from the available coins
            it = availableCoins->erase(it);
            continue;
        }

        nCredit = 0;

        nAttempts++;
        fKernelFound = Stake(pindexPrev, &stakeInput, nBits, nTxNewTime);

        // update staker status (time, attempts)
        pStakerStatus->SetLastTime(nTxNewTime);
        pStakerStatus->SetLastTries(nAttempts);

        if (!fKernelFound) {
            it++;
            continue;
        }

        // Found a kernel
        LogPrintf("CreateCoinStake : kernel found\n");
        nCredit += stakeInput.GetValue();

        // Add block reward to the credit
        nCredit += GetBlockSubsidy(nHeight, Params().GetConsensus(), true);

        // Create the output transaction(s)
        std::vector<CTxOut> vout;
        if (!stakeInput.CreateTxOuts(pwallet, vout, nCredit)) {
            LogPrintf("%s : failed to create output\n", __func__);
            it++;
            continue;
        }
        txNew.vout.insert(txNew.vout.end(), vout.begin(), vout.end());

        // Set output amount
        int outputs = (int) txNew.vout.size() - 1;
        CAmount nRemaining = nCredit;
        if (outputs > 1) {
            // Split the stake across the outputs
            CAmount nShare = nRemaining / outputs;
            for (int i = 1; i < outputs; i++) {
                // loop through all but the last one.
                txNew.vout[i].nValue = nShare;
                nRemaining -= nShare;
            }
        }
        // put the remaining on the last output (which all into the first if only one output)
        txNew.vout[outputs].nValue += nRemaining;

        // Limit size
        unsigned int nBytes = ::GetSerializeSize(txNew, PROTOCOL_VERSION);
        if (nBytes >= DEFAULT_BLOCK_MAX_WEIGHT / 5) {
            return error("%s : exceeded coinstake size limit", __func__);
        }

        // Dev fee
        if (nHeight <= Params().GetConsensus().nDINActivationHeight) {
            txNew.vout.push_back(CTxOut(GetDevCoin(nHeight, GetBlockSubsidy(nHeight, Params().GetConsensus(), true)), devScript));
        } else {
            txNew.vout.push_back(CTxOut(GetDevCoin(nHeight, GetBlockSubsidy(nHeight, Params().GetConsensus(), true)), devScript2));
        }

        // InfinityNode payment
        FillBlock(txNew, nHeight, true);

        const uint256& hashTxOut = txNew.GetHash();
        CTxIn in;
        if (!stakeInput.CreateTxIn(pwallet, in, hashTxOut)) {
            LogPrintf("%s : failed to create TxIn\n", __func__);
            txNew.vin.clear();
            txNew.vout.clear();
            it++;
            continue;
        }
        txNew.vin.emplace_back(in);

        if (!stakeInput.GetTxOutFrom(outProvider)) {
            return error("%s : failed to get CTxOut from stakeInput", __func__);
        }

        break;
    }

    LogPrint(BCLog::STAKING, "%s: attempted staking %d times\n", __func__, nAttempts);

    if (!fKernelFound)
        return false;

    // Sign it
    int nIn = 0;
    LegacyScriptPubKeyMan* provider = pwallet->GetLegacyScriptPubKeyMan();
    for (const CTxIn& txIn : txNew.vin) {
        const CWalletTx* wtx = pwallet->GetWalletTx(txIn.prevout.hash);
        if (!wtx || !SignSignature(*provider, *(wtx->tx), txNew, nIn++, SIGHASH_ALL))
            return error("%s : failed to sign coinstake", __func__);
    }

    LogPrint(BCLog::STAKING, "%s: stake found and signed, returning true\n", __func__);

    // Successfully generated coinstake
    return true;
}


bool ProcessBlockFound(ChainstateManager& chainman, const std::shared_ptr<const CBlock>& pblock, CWallet& wallet)
{
    LogPrintf("%s\n", pblock->ToString());
    LogPrintf("generated %s\n", FormatMoney(pblock->vtx[0]->vout[0].nValue));
    CChainParams chainparams(Params());

    // Found a solution
    {
        WAIT_LOCK(g_best_block_mutex, lock);
        if (pblock->hashPrevBlock != g_best_block && g_best_block != uint256{0})
            return error("PoSMiner : generated block is stale");
    }

    // Process this block the same as if we had received it from another node
    if (!chainman.ProcessNewBlock(chainparams, pblock, true, nullptr)) {
        return error("PoSMiner : ProcessNewBlock, block not accepted");
    }

    return true;
}

bool fStakeableCoins = false;

void StakerCtx::CheckForCoins(CWallet* pwallet, std::vector<CStakeableOutput>* availableCoins)
{

    if (!pwallet || !pStakerStatus) {
        return;
    }

    // control the amount of times the client will check for mintable coins (every block)
    {
        WAIT_LOCK(g_best_block_mutex, lock);
        if (g_best_block == pStakerStatus->GetLastHash() && g_best_block != uint256{0}) {
            return;
        }
    }
    fStakeableCoins = pwallet->StakeableCoins(availableCoins);
}

void InitStakerStatus()
{
    // Staker status (last hashed block and time)
    if (pStakerStatus) {
        pStakerStatus->SetNull();
    } else {
        pStakerStatus = MakeUnique<CStakerStatus>();
    }
}

void StakerCtx::StakerPipe()
{
    LogPrintf("%S : started\n", __func__);
    SetThreadPriority(THREAD_PRIORITY_LOWEST);
    const Consensus::Params& consensus = Params().GetConsensus();
    int64_t nAverageSpacing = consensus.nPowTargetSpacing / 2;

    // Available outputs
    std::vector<CStakeableOutput> availableCoins;

    // Available wallet(s), always use no 0.
    std::vector<std::shared_ptr<CWallet>> wallets = GetWallets();
    CWallet * const pwallet = (wallets.size() > 0) ? wallets[0].get() : nullptr;

    if (!pStakerStatus) {
        InitStakerStatus();
    }

    while (!g_posminer_interrupt) {
        CBlockIndex* pindexPrev = ::ChainActive().Tip();

        // Check for node sync
        if (::ChainstateActive().IsInitialBlockDownload()) {
            LogPrintf("%s : node not synced yet, checking again in %d seconds...\n", __func__, nAverageSpacing);
            if (!g_posminer_interrupt.sleep_for(std::chrono::seconds(nAverageSpacing))) {
                return;
            }
            continue;
        }

        // Check for a wallet
        if (!pwallet) {
            LogPrintf("%s : no wallet found, checking again in %d seconds...\n", __func__, nAverageSpacing);
            if (!g_posminer_interrupt.sleep_for(std::chrono::seconds(nAverageSpacing))) {
                return;
            }
            continue;
        }

        // Check for pointers
        if (!pindexPrev) {
            LogPrintf("%s : no chaintip yet, checking again in %d seconds...\n", __func__, nAverageSpacing);
            if (!g_posminer_interrupt.sleep_for(std::chrono::seconds(nAverageSpacing))) {
                return;
            }
            continue;
        }

        // Check activation bounds
        if (consensus.nStartPoSHeight > pindexPrev->nHeight + 1) {
            LogPrintf("%s : proof-of-stake hasn't started yet, checking again in %d seconds...\n", __func__, nAverageSpacing);
            if (!g_posminer_interrupt.sleep_for(std::chrono::seconds(nAverageSpacing))) {
                return;
            }
            continue;
        }

        // Check if we have any coins
        CheckForCoins(pwallet, &availableCoins);

        while ((Params().NetworkIDString() != CBaseChainParams::REGTEST && (m_connman.GetNodeCount(CConnman::CONNECTIONS_ALL) == 0))
                || pwallet->IsLocked() || !fStakeableCoins) {
            LogPrintf("%s : wallet needs atleast one connection and some stakeable coins to stake, checking again in %d seconds...\n", __func__, nAverageSpacing);
            if (!g_posminer_interrupt.sleep_for(std::chrono::seconds(nAverageSpacing))) {
                return;
            }
            // Do another check here to ensure fStakeableCoins is updated
            if (!fStakeableCoins) CheckForCoins(pwallet, &availableCoins);
        }
        
        //search our map of hashed blocks, see if bestblock has been hashed yet
        if (pStakerStatus &&
                pStakerStatus->GetLastHash() == pindexPrev->GetBlockHash() &&
                pStakerStatus->GetLastTime() >= GetCurrentTimeSlot()) {
            if (!g_posminer_interrupt.sleep_for(std::chrono::seconds(nAverageSpacing))) {
                return;
            }
            continue;
        }

        //
        // Create new block
        //
        std::unique_ptr<CBlockTemplate> pblocktemplate(BlockAssembler(m_mempool, Params()).CreateNewPoSBlock(pwallet, &availableCoins, pStakerStatus.get()));
        if (!pblocktemplate.get()) {
            continue;
        }
        CBlock *pblock = &pblocktemplate->block;

        // Block found: process it
        LogPrintf("%s : proof-of-stake block was signed %s \n", __func__, pblock->GetHash().ToString().c_str());
        std::shared_ptr<const CBlock> shared_pblock = std::make_shared<const CBlock>(pblocktemplate->block);
        SetThreadPriority(THREAD_PRIORITY_NORMAL);
        if (!ProcessBlockFound(m_chainman, shared_pblock, *pwallet)) {
            LogPrintf("%s: New block orphaned\n", __func__);
            continue;
        }
        SetThreadPriority(THREAD_PRIORITY_LOWEST);
        continue;
    }
}

void StakerCtx::StartStaker()
{
    if (!g_posminer_thread.joinable()) {
        assert(!g_posminer_interrupt);
        g_posminer_thread = std::thread(&TraceThread<std::function<void()> >, "staker", std::function<void()>(std::bind(&StakerCtx::StakerPipe, this)));
    }
}

void StakerCtx::InterruptStaker()
{
    LogPrintf("%s : interrupting staker\n", __func__);
    if(g_posminer_thread.joinable()) {
        g_posminer_interrupt();
    }
}

void StakerCtx::StopStaker()
{
    LogPrintf("%s : stopping staker\n", __func__);
    if(g_posminer_thread.joinable()) {
        g_posminer_thread.join();
        g_posminer_interrupt.reset();
    }
}

#endif // ENABLE_WALLET