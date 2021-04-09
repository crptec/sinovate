// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CONSENSUS_PARAMS_H
#define BITCOIN_CONSENSUS_PARAMS_H

#include <uint256.h>
#include <limits>

namespace Consensus {

enum DeploymentPos
{
    DEPLOYMENT_TESTDUMMY,
    DEPLOYMENT_TAPROOT, // Deployment of Schnorr/Taproot (BIPs 340-342)
    // NOTE: Also add new deployments to VersionBitsDeploymentInfo in versionbits.cpp
    MAX_VERSION_BITS_DEPLOYMENTS
};

/**
 * Struct for each individual consensus rule change using BIP9.
 */
struct BIP9Deployment {
    /** Bit position to select the particular bit in nVersion. */
    int bit;
    /** Start MedianTime for version bits miner confirmation. Can be a date in the past */
    int64_t nStartTime;
    /** Timeout/expiry MedianTime for the deployment attempt. */
    int64_t nTimeout;

    /** Constant for nTimeout very far in the future. */
    static constexpr int64_t NO_TIMEOUT = std::numeric_limits<int64_t>::max();

    /** Special value for nStartTime indicating that the deployment is always active.
     *  This is useful for testing, as it means tests don't need to deal with the activation
     *  process (which takes at least 3 BIP9 intervals). Only tests that specifically test the
     *  behaviour during activation cannot use this. */
    static constexpr int64_t ALWAYS_ACTIVE = -1;
};

/**
 * Parameters that influence chain consensus.
 */
struct Params {

    /* Sinovate params START*/

    // Sinovate parameters for IN functionality
    int nMasternodeBurnSINNODE_1; // in coins
    int nMasternodeBurnSINNODE_5; // in coins
    int nMasternodeBurnSINNODE_10; // in coins
    int nLimitSINNODE_1;// integer
    int nLimitSINNODE_5;// integer
    int nLimitSINNODE_10;// integer
    int nInfinityNodeBeginHeight;// integer
    int nInfinityNodeGenesisStatement;// integer
    int nInfinityNodeUpdateMeta;// integer
    int nInfinityNodeVoteValue; // in coins
    int nInfinityNodeNotificationValue; // in coins
    int nInfinityNodeCallLockRewardDeepth; //in number of blocks
    int nInfinityNodeCallLockRewardLoop; //in number of blocks
    int nInfinityNodeLockRewardTop; //in number
    int nInfinityNodeLockRewardSigners; //in number
    int nInfinityNodeLockRewardSINType; //in number
    int nInfinityNodeExpireTime; //in number
    int nSchnorrActivationHeight; // block height (int)
    int nINActivationHeight; // block height (int)
    int nINEnforcementHeight; // block height (int)
    int nDINActivationHeight; // block height (int) - DIN switch height

    // different constant addresses we use 
    const char *devAddressPubKey;
    const char *devAddress;
    const char *devAddress2PubKey;
    const char *devAddress2;
    const char *cBurnAddress;
    const char *cBurnAddressPubKey;
    const char *cMetadataAddress;
    const char *cMetadataAddressPubKey;
    const char *cNotifyAddress;
    const char *cNotifyAddressPubKey;
    const char *cLockRewardAddress;
    const char *cLockRewardAddressPubKey;
    const char *cGovernanceAddress;

    // LWMA params
    int lwmaStartHeight;
    int lwmaAveragingWindow;

    //x25x hf
    int nX25XForkHeight;

    // proof-of-stake: activation and params
    int nStartPoSHeight;
    int nStakeMinDepth;
    int nTimeSlotLength;
    uint256 posLimit;
    bool fPoSNoRetargeting;
    int64_t nPoS_EMATargetTimespan;
    int nPoSMinStakeValue;

    // proof-of-stake: helper funcs

    int FutureBlockTimeDrift() const
    {
        // PoS (TimeV2): 14 seconds
        return nTimeSlotLength - 1;
    }

    bool IsValidBlockTimeStamp(const int64_t nTime) const
    {
        return (nTime % nTimeSlotLength) == 0;
    }

    bool HasStakeMinDepth(const int contextHeight,
            const int utxoFromBlockHeight) const
    {
        return (contextHeight - utxoFromBlockHeight >= nStakeMinDepth);
    }

    /* Sinovate params END */

    uint256 hashGenesisBlock;
    int nSubsidyHalvingInterval;
    /* Block hash that is excepted from BIP16 enforcement */
    uint256 BIP16Exception;
    /** Block height and hash at which BIP34 becomes active */
    int BIP34Height;
    uint256 BIP34Hash;
    /** Block height at which BIP65 becomes active */
    int BIP65Height;
    /** Block height at which BIP66 becomes active */
    int BIP66Height;
    /** Block height at which CSV (BIP68, BIP112 and BIP113) becomes active */
    int CSVHeight;
    /** Block height at which Segwit (BIP141, BIP143 and BIP147) becomes active.
     * Note that segwit v0 script rules are enforced on all blocks except the
     * BIP 16 exception blocks. */
    int SegwitHeight;
    /** Don't warn about unknown BIP 9 activations below this height.
     * This prevents us from warning about the CSV and segwit activations. */
    int MinBIP9WarningHeight;
    /**
     * Minimum blocks including miner confirmation of the total of 2016 blocks in a retargeting period,
     * (nPowTargetTimespan / nPowTargetSpacing) which is also used for BIP9 deployments.
     * Examples: 1916 for 95%, 1512 for testchains.
     */
    uint32_t nRuleChangeActivationThreshold;
    uint32_t nMinerConfirmationWindow;
    BIP9Deployment vDeployments[MAX_VERSION_BITS_DEPLOYMENTS];
    /** Proof of work parameters */
    uint256 powLimit;
    bool fPowAllowMinDifficultyBlocks;
    bool fPowNoRetargeting;
    int64_t nPowTargetSpacing;
    int64_t nPowTargetTimespan;
    int64_t DifficultyAdjustmentInterval() const { return nPowTargetTimespan / nPowTargetSpacing; }
    /** The best chain should have at least this much work */
    uint256 nMinimumChainWork;
    /** By default assume that the signatures in ancestors of this block are valid */
    uint256 defaultAssumeValid;

    /**
     * If true, witness commitments contain a payload equal to a Bitcoin Script solution
     * to the signet challenge. See BIP325.
     */
    bool signet_blocks{false};
    std::vector<uint8_t> signet_challenge;
};
} // namespace Consensus

#endif // BITCOIN_CONSENSUS_PARAMS_H
