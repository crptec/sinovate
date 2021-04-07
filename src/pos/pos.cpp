// Copyright (c) 2011-2013 The PPCoin developers
// Copyright (c) 2013-2014 The NovaCoin Developers
// Copyright (c) 2014-2018 The BlackCoin Developers
// Copyright (c) 2015-2020 The PIVX developers
// Copyright (c) 2021 The SINOVATE developers
// Copyright (c) 2021 giaki3003
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <pos/pos.h>

#include <policy/policy.h>
#include <script/interpreter.h>
#include <pos/stakeinput.h>
#include <timedata.h>
#include <chainparams.h>

/**
 * CStakeKernel Constructor
 *
 * @param[in]   pindexPrev      index of the parent of the kernel block
 * @param[in]   stakeInput      input for the coinstake of the kernel block
 * @param[in]   nBits           target difficulty bits of the kernel block
 * @param[in]   nTimeTx         time of the kernel block
 */
CStakeKernel::CStakeKernel(const CBlockIndex* const pindexPrev, CStakeInput* stakeInput, unsigned int nBits, int nTimeTx):
    stakeUniqueness(stakeInput->GetUniqueness()),
    nTime(nTimeTx),
    nBits(nBits),
    stakeValue(stakeInput->GetValue())
{
    stakeModifier << pindexPrev->GetStakeModifier();
    const CBlockIndex* pindexFrom = stakeInput->GetIndexFrom();
    nTimeBlockFrom = pindexFrom->nTime;
}

// Return stake kernel hash
uint256 CStakeKernel::GetHash() const
{
    CDataStream ss(stakeModifier);
    ss << nTimeBlockFrom << stakeUniqueness << nTime;
    return HashS(ss.begin(), ss.end());
}

// Check that the kernel hash meets the target required
bool CStakeKernel::CheckKernelHash() const
{
    // Get weighted target
    arith_uint256 bnTarget;
    bnTarget.SetCompact(nBits);
    bnTarget *= (arith_uint256(stakeValue) / 100);

    // Check PoS kernel hash
    const arith_uint256& hashProofOfStake = UintToArith256(GetHash());
    const bool res = hashProofOfStake < bnTarget;

    LogPrint(BCLog::STAKING, "%s : Proof Of Stake:"
                        "\nssUniqueID=%s"
                        "\nnTimeTx=%d"
                        "\nhashProofOfStake=%s"
                        "\nnBits=%d"
                        "\nweight=%d"
                        "\nbnTarget=%s (res: %d)\n\n",
        __func__, HexStr(stakeUniqueness), nTime, hashProofOfStake.GetHex(),
        nBits, stakeValue, bnTarget.GetHex(), res);

    return res;
}


/*
 * PoS Validation
 */

// helper function for CheckProofOfStake and GetStakeKernelHash
bool LoadStakeInput(const CBlock& block, const CBlockIndex* pindexPrev, std::unique_ptr<CStakeInput>& stake)
{
    // If previous index is not provided, look for it in the blockmap
    if (!pindexPrev) {
        pindexPrev = LookupBlockIndex(block.hashPrevBlock);
        if (!pindexPrev) {
            return error("%s : couldn't find previous block", __func__);
        }
    } else {
        // check that is the actual parent block
        if (block.hashPrevBlock != pindexPrev->GetBlockHash())
            return error("%s : previous block mismatch", __func__);
    }

    // Check that this is a PoS block
    if (!block.IsProofOfStake())
        return error("called on non PoS block");

    // Construct the stakeinput object
    const CTxIn& txin = block.vtx[1]->vin[0];
    stake = std::unique_ptr<CStakeInput>(CSinStake::NewSinStake(txin));

    return stake && stake->InitFromTxIn(txin);
}

/*
 * Stake                Check if stakeInput can stake a block on top of pindexPrev
 *
 * @param[in]   pindexPrev      index of the parent block of the block being staked
 * @param[in]   stakeInput      input for the coinstake
 * @param[in]   nBits           target difficulty bits
 * @param[in]   nTimeTx         new blocktime
 * @return      bool            true if stake kernel hash meets target protocol
 */
bool Stake(const CBlockIndex* pindexPrev, CStakeInput* stakeInput, unsigned int nBits, int64_t& nTimeTx)
{
    // Double check stake input contextual checks
    const int nHeightTx = pindexPrev->nHeight + 1;
    if (!stakeInput || !stakeInput->ContextCheck(nHeightTx)) return false;

    // Get the new time slot (and verify it's not the same as previous block)
    const bool fRegTest = Params().NetworkIDString() == CBaseChainParams::REGTEST;
    nTimeTx = (fRegTest ? GetAdjustedTime() : GetCurrentTimeSlot());
    if (nTimeTx <= pindexPrev->nTime && !fRegTest) return false;

    // Verify Proof Of Stake
    CStakeKernel stakeKernel(pindexPrev, stakeInput, nBits, nTimeTx);
    return stakeKernel.CheckKernelHash();
}


/*
 * CheckProofOfStake    Check if block has valid proof of stake
 *
 * @param[in]   block           block being verified
 * @param[out]  strError        string error (if any, else empty)
 * @param[in]   pindexPrev      index of the parent block
 *                              (if nullptr, it will be searched in mapBlockIndex)
 * @return      bool            true if the block has a valid proof of stake
 */
bool CheckProofOfStake(const CBlock& block, BlockValidationState& state, const Consensus::Params& consensusParams, const CBlockIndex* pindexPrev)
{
    const int nHeight = pindexPrev->nHeight + 1;
    // Initialize stake input
    std::unique_ptr<CStakeInput> stakeInput;
    if (!LoadStakeInput(block, pindexPrev, stakeInput)) {
        return state.Invalid(BlockValidationResult::BLOCK_POS_BAD, "bad-pos-stakeinput", "cannot init stakeinput");
    }

    // Stake input contextual checks
    if (!stakeInput->ContextCheck(nHeight)) {
        return state.Invalid(BlockValidationResult::BLOCK_POS_BAD, "bad-pos-ctx", "stakeinput failing contextual checks");
    }

    // Verify Proof Of Stake
    CStakeKernel stakeKernel(pindexPrev, stakeInput.get(), block.nBits, block.nTime);
    if (!stakeKernel.CheckKernelHash()) {
        return state.Invalid(BlockValidationResult::BLOCK_POS_BAD, "bad-pos-kernel", "kernel failing hash check");
    }

    // Verify tx input signature
    CTxOut stakePrevout;
    if (!stakeInput->GetTxOutFrom(stakePrevout)) {
        return state.Invalid(BlockValidationResult::BLOCK_POS_BAD, "bad-pos-prevout", "cannot init prevout from stakeinput");
    }
    const auto& tx = block.vtx[1];
    const CTxIn& txin = tx->vin[0];
    ScriptError serror;
    if (!VerifyScript(txin.scriptSig, stakePrevout.scriptPubKey, nullptr, STANDARD_SCRIPT_VERIFY_FLAGS,
             TransactionSignatureChecker(tx.get(), 0, stakePrevout.nValue), &serror)) {
        return state.Invalid(BlockValidationResult::BLOCK_POS_BAD, "bad-pos-sig", serror ? ScriptErrorString(serror) : "signature failing");
    }

    // All good
    return true;
}


/*
 * GetStakeKernelHash   Return stake kernel of a block
 *
 * @param[out]  hashRet         hash of the kernel (set by this function)
 * @param[in]   block           block with the kernel to return
 * @param[in]   pindexPrev      index of the parent block
 *                              (if nullptr, it will be searched in mapBlockIndex)
 * @return      bool            false if kernel cannot be initialized, true otherwise
 */
bool GetStakeKernelHash(uint256& hashRet, const CBlock& block, const CBlockIndex* pindexPrev)
{
    // Initialize stake input
    std::unique_ptr<CStakeInput> stakeInput;
    if (!LoadStakeInput(block, pindexPrev, stakeInput))
        return error("%s : stake input initialization failed", __func__);

    CStakeKernel stakeKernel(pindexPrev, stakeInput.get(), block.nBits, block.nTime);
    hashRet = stakeKernel.GetHash();
    return true;
}

