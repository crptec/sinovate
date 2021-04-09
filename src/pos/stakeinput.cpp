// Copyright (c) 2017-2020 The PIVX developers
// Copyright (c) 2021 The SINOVATE developers
// Copyright (c) 2021 giaki3003
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://www.opensource.org/licenses/mit-license.php.

#include <pos/stakeinput.h>

#include <amount.h>
#include <chain.h>
#include <txdb.h>
#include <wallet/wallet.h>
#include <validation.h>
#include <chainparams.h>

CSinStake* CSinStake::NewSinStake(const CTxIn& txin)
{

    // Find the previous transaction in database
    uint256 hashBlock;
    CTransactionRef txPrev;
    if (!GetTransaction(txin.prevout.hash, txPrev, hashBlock)) {
        error("%s : INFO: read txPrev failed, tx id prev: %s", __func__, txin.prevout.hash.GetHex());
        return nullptr;
    }

    const CBlockIndex* pindexFrom = nullptr;
    // Find the index of the block of the previous transaction
    CBlockIndex* pindex = LookupBlockIndex(hashBlock);
    if (pindex) {
        if (::ChainActive().Contains(pindex)) {
            pindexFrom = pindex;
        }
    }
    // Check that the input is in the active chain
    if (!pindexFrom) {
        error("%s : Failed to find the block index for stake origin", __func__);
        return nullptr;
    }

    return new CSinStake(txPrev->vout[txin.prevout.n],
                         txin.prevout,
                         pindexFrom);
}

bool CSinStake::GetTxOutFrom(CTxOut& out) const
{
    out = outputFrom;
    return true;
}

bool CSinStake::CreateTxIn(CWallet* pwallet, CTxIn& txIn, uint256 hashTxOut)
{
    txIn = CTxIn(outpointFrom.hash, outpointFrom.n);
    return true;
}

CAmount CSinStake::GetValue() const
{
    return outputFrom.nValue;
}

bool CSinStake::CreateTxOuts(CWallet* pwallet, std::vector<CTxOut>& vout, CAmount nTotal)
{
    std::vector<std::vector<unsigned char>> vSolutions2D;
    CScript scriptPubKeyKernel = outputFrom.scriptPubKey;
    TxoutType whichType = Solver(scriptPubKeyKernel, vSolutions2D);

    if (whichType != TxoutType::PUBKEYHASH) {
        return error("%s: failed to parse kernel", __func__);
    }

    vout.emplace_back(0, scriptPubKeyKernel);

    return true;
}

CDataStream CSinStake::GetUniqueness() const
{
    //The unique identifier for a SIN stake is the outpoint
    CDataStream ss(SER_NETWORK, 0);
    ss << outpointFrom.n << outpointFrom.hash;
    return ss;
}

//The block that the UTXO was added to the chain
const CBlockIndex* CSinStake::GetIndexFrom() const
{
    // Sanity check, pindexFrom is set on the constructor.
    if (!pindexFrom) throw std::runtime_error("CSinStake: uninitialized pindexFrom");
    return pindexFrom;
}

// Verify stake contextual checks
bool CSinStake::ContextCheck(int nHeight)
{
    const Consensus::Params& consensusParams = Params().GetConsensus();
    // Get Stake input block time/height
    const CBlockIndex* pindexFrom = GetIndexFrom();
    if (!pindexFrom)
        return error("%s: unable to get previous index for stake input", __func__);
    const int nHeightBlockFrom = pindexFrom->nHeight;

    // Check that the stake has the required depth/age
    if (!consensusParams.HasStakeMinDepth(nHeight, nHeightBlockFrom))
        return error("%s : min depth violation - height=%d - time=%d, nHeightBlockFrom=%d",
                         __func__, nHeight, nHeightBlockFrom);
    // All good
    return true;
}

// Verify stake value checks
bool CSinStake::ValueCheck()
{
    const Consensus::Params& consensusParams = Params().GetConsensus();

    if (GetValue() < (consensusParams.nPoSMinStakeValue * COIN)) {
        return error("%s: stake input below min value threshold", __func__);
    }

    // All good
    return true;
}

