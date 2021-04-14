// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 The Bitcoin Core developers
// Copyright (c) 2015-2020 The LUXCore developers
// Copyright (c) 2015-2020 The SINOVATE developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <pow.h>

#include <arith_uint256.h>
#include <chain.h>
#include <primitives/block.h>
#include <uint256.h>

// Function which returns the last PoS CBlockIndex when fProofOfStake is true, or last PoW CBlockIndex otherwise.
const CBlockIndex* GetLastBlockIndex(const CBlockIndex* pindex, bool fProofOfStake)
{
    while (pindex && pindex->pprev && (pindex->IsProofOfStake() != fProofOfStake))
        pindex = pindex->pprev;
    return pindex;
}

// Function which counts the last PoS CBlockIndex-es we have over nHeightScan.
int CountPoS(const CBlockIndex* pindex, int nHeightScan)
{
    int nFound = 0;
    while (pindex && pindex->pprev && (pindex->nHeight > nHeightScan)) {
        if (pindex->IsProofOfStake()) {
            nFound++;
        }
        pindex = pindex->pprev;
    }
    return nFound;
}

// Function which counts the last fProofOfStake blocks 
// if the index IsProofOfStake == true, or fProofOfWork blocks if
// the index is IsProofOfStake == false, over a specified  
// nHeightScan; subsequently, a map is built containing the sorted heights of all PoS or PoW blocks.
std::map<int, int> GetContextLWMA(const CBlockIndex* pindex, int nContextScope, bool fProofOfStake)
{
    std::map<int, int> mapRet;
    int nIdx = 0;
    while (fProofOfStake && pindex && pindex->pprev && nIdx <= nContextScope) {
        if (pindex->IsProofOfStake()) {
            nIdx++;
            mapRet.insert(std::pair<int, int>(nIdx, pindex->nHeight));
        }
        pindex = pindex->pprev;
    }
    while (!fProofOfStake && pindex && pindex->pprev && nIdx <= nContextScope) {
        if (pindex->IsProofOfWork()) {
            nIdx++;
            mapRet.insert(std::pair<int, int>(nIdx, pindex->nHeight));
        }
        pindex = pindex->pprev;
    }
    return mapRet;
}

// Sinovate used DGW before the LWMA-3 fork
unsigned int static DarkGravityWave(const CBlockIndex* pindexLast, const Consensus::Params& params)
{
    if (params.fPowNoRetargeting)
        return pindexLast->nBits;

    const CBlockIndex *BlockLastSolved = pindexLast;
    const CBlockIndex *BlockReading = pindexLast;
    int64_t nActualTimespan = 0;
    int64_t LastBlockTime = 0;
    int64_t PastBlocksMin = 24;
    int64_t PastBlocksMax = 24;
    int64_t CountBlocks = 0;
    arith_uint256 PastDifficultyAverage;
    arith_uint256 PastDifficultyAveragePrev;

    const arith_uint256 bnPowLimit = UintToArith256(params.powLimit);

    if (BlockLastSolved == NULL ||
        BlockLastSolved->nHeight < PastBlocksMin)
        return bnPowLimit.GetCompact();

    for (unsigned int i = 1; BlockReading && BlockReading->nHeight > 0; i++)
    {
        if (PastBlocksMax > 0 && i > PastBlocksMax)
        break;

        CountBlocks++;

        if(CountBlocks <= PastBlocksMin)
        {
            if (CountBlocks == 1)
        PastDifficultyAverage.SetCompact(BlockReading->nBits);
            else
        PastDifficultyAverage = ((PastDifficultyAveragePrev * CountBlocks) + (arith_uint256().SetCompact(BlockReading->nBits))) / (CountBlocks + 1);
            PastDifficultyAveragePrev = PastDifficultyAverage;
        }

        if(LastBlockTime > 0){
            int64_t Diff = (LastBlockTime - BlockReading->GetBlockTime());
            nActualTimespan += Diff;
        }

        LastBlockTime = BlockReading->GetBlockTime();

        if (BlockReading->pprev == NULL) { assert(BlockReading); break; }
        BlockReading = BlockReading->pprev;
    }

    arith_uint256 bnNew(PastDifficultyAverage);

    int64_t _nTargetTimespan = CountBlocks * params.nPowTargetSpacing;

    if (nActualTimespan < _nTargetTimespan/3)
        nActualTimespan = _nTargetTimespan/3;
    if (nActualTimespan > _nTargetTimespan*3)
        nActualTimespan = _nTargetTimespan*3;

    // Retarget
    bnNew *= nActualTimespan;
    bnNew /= _nTargetTimespan;

    if (bnNew > bnPowLimit)
        bnNew = bnPowLimit;

    return bnNew.GetCompact();
}

// LWMA-1 for BTC/Zcash clones
// LWMA has the best response*stability.  It rises slowly & drops fast when needed.
// Copyright (c) 2017-2018 The Bitcoin Gold developers
// Copyright (c) 2018-2019 Zawy
// Copyright (c) 2018 iamstenman (Microbitcoin)
// MIT License
// Algorithm by Zawy, a modification of WT-144 by Tom Harding
// For any changes, patches, updates, etc see
// https://github.com/zawy12/difficulty-algorithms/issues/3#issuecomment-442129791
//  FTL should be lowered to about N*T/20.
//  FTL in BTC clones is MAX_FUTURE_BLOCK_TIME in chain.h.
//  FTL in Ignition, Numus, and others can be found in main.h as DRIFT.
//  FTL in Zcash & Dash clones need to change the 2*60*60 here:
//  if (block.GetBlockTime() > nAdjustedTime + 2 * 60 * 60)
//  which is around line 3700 in main.cpp in ZEC and validation.cpp in Dash
//  If your coin uses network time instead of node local time, lowering FTL < about 125% of the
//  "revert to node time" rule (70 minutes in BCH, ZEC, & BTC) allows 33% Sybil attack,
//  so revert rule must be ~ FTL/2 instead of 70 minutes. See:
// https://github.com/zcash/zcash/issues/4021

unsigned int Lwma3CalculateNextWorkRequired(const CBlockIndex* pindexLast, const Consensus::Params& params, bool fProofOfStake)
{
    if (params.fPowNoRetargeting || params.fPoSNoRetargeting) {
        const CBlockIndex* noRetIdx = GetLastBlockIndex(pindexLast, fProofOfStake);
        return noRetIdx->nBits;
    }

    int nPoSBlocksCounter = 0;
    
    const int64_t T = params.nPowTargetSpacing;

   // For T=600, 300, 150 use approximately N=60, 90, 120
    const int64_t N = params.lwmaAveragingWindow;

    // Define a k that will be used to get a proper average after weighting the solvetimes.
    const int64_t k = N * (N + 1) * T / 2;

    const int64_t height = pindexLast->nHeight;
    const arith_uint256 ProofLimit = UintToArith256(fProofOfStake ? params.posLimit : params.powLimit);

    // New coins should just give away first N blocks before using this algorithm.
    if (height < N) { return ProofLimit.GetCompact(); }

    // Since we have hybrid consensus, lookup the last N blocks of the same proof, and index them as a context for diff calculations.
    std::map<int, int> mapContext = GetContextLWMA(pindexLast, N + 1, fProofOfStake);

    // Special rule for PoS activation, make sure we use LWMA only after ~300 (lwmaAveragingWindow) PoS blocks have been found
    if (fProofOfStake && mapContext.size() < (N + 1)) { // Check for this rule only if we don't have enough PoS blocks for LWMA.
        nPoSBlocksCounter = CountPoS(pindexLast, params.nStartPoSHeight);
        // Let the first N + 1 PoS blocks use ppcoin EMA
        if (nPoSBlocksCounter <= N + 1) { 
            const CBlockIndex* pindexPrev = GetLastBlockIndex(pindexLast, fProofOfStake);
            if (pindexPrev->pprev == nullptr) {
                return ProofLimit.GetCompact(); // first block
            }
            const CBlockIndex* pindexPrevPrev = GetLastBlockIndex(pindexPrev->pprev, fProofOfStake);
            if (pindexPrevPrev->pprev == nullptr) {
                return ProofLimit.GetCompact(); // second block
            }

            int64_t nActualSpacing = pindexPrev->GetBlockTime() - pindexPrevPrev->GetBlockTime();
            if (nActualSpacing < 0)
                nActualSpacing = 1;
            if (nActualSpacing > T * 10)
                nActualSpacing = T * 10;

            // ppcoin: target change every block
            // ppcoin: retarget with exponential moving toward target spacing
            arith_uint256 bnNew;
            bnNew.SetCompact(pindexLast->nBits);

            int64_t nInterval = params.nPoS_EMATargetTimespan / T;
            bnNew *= ((nInterval - 1) * T + nActualSpacing + nActualSpacing);
            bnNew /= ((nInterval + 1) * T);

            if (bnNew <= 0 || bnNew > ProofLimit)
                bnNew = ProofLimit;

            return bnNew.GetCompact();
        }
    }

    arith_uint256 avgTarget, nextTarget;
    int64_t thisTimestamp, previousTimestamp;
    int64_t sumWeightedSolvetimes = 0, j = 0;

    const CBlockIndex* blockPreviousTimestamp = pindexLast->GetAncestor(mapContext.at(N + 1));
    previousTimestamp = blockPreviousTimestamp->GetBlockTime();

    // Loop through N most recent blocks of the same proof type.
    // Thus means we may need more than N blocks, as we index proofs together.
    for (int64_t i = N; i > 0; i--) {
        const CBlockIndex* block = pindexLast->GetAncestor(mapContext.at(i));

        // Prevent solvetimes from being negative in a safe way. It must be done like this.
        // In particular, do not attempt anything like  if(solvetime < 0) {solvetime=0;}
        // The +1 ensures new coins do not calculate nextTarget = 0.
        thisTimestamp = (block->GetBlockTime() > previousTimestamp) ?
                            block->GetBlockTime() : previousTimestamp + 1;

       // A 6*T limit will prevent large drops in difficulty from long solvetimes.
        int64_t solvetime = std::min(6 * T, thisTimestamp - previousTimestamp);

       // The following is part of "preventing negative solvetimes".
        previousTimestamp = thisTimestamp;

       // Give linearly higher weight to more recent solvetimes.
        j++;
        sumWeightedSolvetimes += solvetime * j;

        arith_uint256 target;
        target.SetCompact(block->nBits);
        avgTarget += target / N / k; // Dividing by k here prevents an overflow below.
    }
   // Desired equation in next line was nextTarget = avgTarget * sumWeightSolvetimes / k
   // but 1/k was moved to line above to prevent overflow in new coins

    nextTarget = avgTarget * sumWeightedSolvetimes;

    if (nextTarget > ProofLimit) { nextTarget = ProofLimit; }

    return nextTarget.GetCompact();
}

unsigned int GetNextWorkRequired(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const Consensus::Params& params, bool fProofOfStake)
{
    
    if (pindexLast->nHeight + 1 < params.lwmaStartHeight) {
        return DarkGravityWave(pindexLast, params);
    } else {
        return Lwma3CalculateNextWorkRequired(pindexLast, params, fProofOfStake);
    }
}

bool CheckProofOfWork(uint256 hash, unsigned int nBits, const Consensus::Params& params)
{
    bool fNegative;
    bool fOverflow;
    arith_uint256 bnTarget;

    bnTarget.SetCompact(nBits, &fNegative, &fOverflow);

    // Check range
    if (fNegative || bnTarget == 0 || fOverflow || bnTarget > UintToArith256(params.powLimit))
        return false;

    // Check proof of work matches claimed amount
    if (UintToArith256(hash) > bnTarget)
        return false;

    return true;
}
