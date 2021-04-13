// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>

#include <chainparamsseeds.h>
#include <consensus/merkle.h>
#include <hash.h> // for signet block challenge hash
#include <tinyformat.h>
#include <util/system.h>
#include <util/strencodings.h>
#include <versionbitsinfo.h>

/** SIN specific */
#include <chainparamsbrokenblocks.h>

#include <assert.h>

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

// sinovate
#define NEVER 2000000000

static CBlock CreateGenesisBlock(const char* pszTimestamp, const CScript& genesisOutputScript, uint32_t nTime, uint32_t nNonce, uint32_t nBits, int32_t nVersion, const CAmount& genesisReward)
{
    CMutableTransaction txNew;
    txNew.nVersion = 1;
    txNew.vin.resize(1);
    txNew.vout.resize(1);
    txNew.vin[0].scriptSig = CScript() << 520159231 << CScriptNum(4) << std::vector<unsigned char>((const unsigned char*)pszTimestamp, (const unsigned char*)pszTimestamp + strlen(pszTimestamp));
    txNew.vout[0].nValue = genesisReward;
    txNew.vout[0].scriptPubKey = genesisOutputScript;

    CBlock genesis;
    genesis.nTime    = nTime;
    genesis.nBits    = nBits;
    genesis.nNonce   = nNonce;
    genesis.nVersion = nVersion;
    genesis.vtx.push_back(MakeTransactionRef(std::move(txNew)));
    genesis.hashPrevBlock.SetNull();
    genesis.hashMerkleRoot = BlockMerkleRoot(genesis);
    return genesis;
}

/**
 * Build the genesis block. Note that the output of its generation
 * transaction cannot be spent since it did not originally exist in the
 * database.
 *
 * CBlock(hash=000000000019d6, ver=1, hashPrevBlock=00000000000000, hashMerkleRoot=4a5e1e, nTime=1231006505, nBits=1d00ffff, nNonce=2083236893, vtx=1)
 *   CTransaction(hash=4a5e1e, ver=1, vin.size=1, vout.size=1, nLockTime=0)
 *     CTxIn(COutPoint(000000, -1), coinbase 04ffff001d0104455468652054696d65732030332f4a616e2f32303039204368616e63656c6c6f72206f6e206272696e6b206f66207365636f6e64206261696c6f757420666f722062616e6b73)
 *     CTxOut(nValue=50.00000000, scriptPubKey=0x5F1DF16B2B704C8A578D0B)
 *   vMerkleTree: 4a5e1e
 */
static CBlock CreateGenesisBlock(uint32_t nTime, uint32_t nNonce, uint32_t nBits, int32_t nVersion, const CAmount& genesisReward)
{
    const char* pszTimestamp = "The Guardian 27/06/18 One football pitch of forest lost every second in 2017";
    const CScript genesisOutputScript = CScript() << ParseHex("04678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef38c4f35504e51ec112de5c384df7ba0b8d578a4c702b6bf11d5f") << OP_CHECKSIG;
    return CreateGenesisBlock(pszTimestamp, genesisOutputScript, nTime, nNonce, nBits, nVersion, genesisReward);
}

int CChainParams::getNodeDelta(int nHeight) const {
    if (nHeight > nDeltaChangeHeight) {
        return 2000;
    } else {
        return 1000;
    }
}

/**
 * Main network
 */
class CMainParams : public CChainParams {
public:
    CMainParams() {
        /** Sinovate params START */

        // broken negative fee blocks
        brokenfeeblocksData = brokenfeeblocksDataMain;


        // legacy Dash, needs refac
        consensus.nMasternodeBurnSINNODE_1 = 100000;
        consensus.nMasternodeBurnSINNODE_5 = 500000;
        consensus.nMasternodeBurnSINNODE_10 = 1000000;

        // node number bounds
        consensus.nLimitSINNODE_1=375;
        consensus.nLimitSINNODE_5=375;
        consensus.nLimitSINNODE_10=375;

        // IN params
        consensus.nInfinityNodeBeginHeight=160000;
        consensus.nInfinityNodeGenesisStatement=250000;
        consensus.nInfinityNodeUpdateMeta=25;
        consensus.nInfinityNodeVoteValue=100;
        consensus.nInfinityNodeNotificationValue=1;
        consensus.nInfinityNodeCallLockRewardDeepth=50;
        consensus.nInfinityNodeCallLockRewardLoop=10; //in number of blocks
        consensus.nInfinityNodeLockRewardTop=16; //in number
        consensus.nInfinityNodeLockRewardSigners=4; //in number
        consensus.nInfinityNodeLockRewardSINType=10; //in number
        consensus.nSchnorrActivationHeight = 1350000; // wait for active
        consensus.nInfinityNodeExpireTime=262800;//720*365 days = 1 year

        /*Previously used as simple constants in validation */
        consensus.nINActivationHeight = 170000; // Activation of IN payments, should also be the same as nInfinityNodeBeginHeight in primitives/block.cpp
        consensus.nINEnforcementHeight = 178000; // Enforcement of IN payments
        consensus.nDINActivationHeight = 550000; // Activation of DIN 1.0 payments, and new dev fee address.

        // height at which we fork to X25X
        consensus.nX25XForkHeight = 170000;

        //LWMA diff algo params
        consensus.lwmaStartHeight = 262000;
        consensus.lwmaAveragingWindow = 96;

        // IN reorg bounds have been parameterised
        nMaxReorganizationDepth = 55; // 55 at 2 minute block timespan is +/- 120 minutes/2h.
        nDeltaChangeHeight = 617000;

        // proof-of-stake: activation and params
        consensus.nStartPoSHeight = 9999999;
        consensus.nStakeMinDepth = 14400;
        consensus.posLimit = uint256S("0000000000001fffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.fPoSNoRetargeting = false;
        consensus.nPoS_EMATargetTimespan = 30 * 60;
        consensus.nTimeSlotLength = 15;
        consensus.nPoSMinStakeValue = 1;
        
        // addresses
        consensus.devAddressPubKey = "841e6bf56b99a59545da932de2efb23ab93b4f44";
        consensus.devAddress = "SZLafuDjnjqh2tAfTrG9ZAGzbP8HkzNXvB";
        consensus.devAddress2PubKey = "c07290a27153f8adaf01e6f5817405a32f569f61";
        consensus.devAddress2 = "STEkkU29v5rjb6CMUdGciF1e4STZ6jx7aq";
        consensus.cBurnAddressPubKey = "ebaf5ec74cb2e2342dfda0229111738ff4dc742d";
        consensus.cBurnAddress = "SinBurnAddress123456789SuqaXbx3AMC";
        consensus.cMetadataAddress = "SinBurnAddressForMetadataXXXXEU2mj";
        consensus.cNotifyAddress = "SinBurnAddressForNotifyXXXXXc42TcT";
        consensus.cLockRewardAddress = "SinBurnAddressForLockRewardXTbeffB";
        consensus.cGovernanceAddress = "SinBurnAddressGovernanceVoteba5vkQ";

        /** Sinovate params END */
        strNetworkID = CBaseChainParams::MAIN;
        consensus.signet_blocks = false;
        consensus.signet_challenge.clear();
        consensus.nSubsidyHalvingInterval = 210000;
        // most activations have been deferred on mainnet (todo: re-enable them)
        consensus.BIP16Exception = uint256S("0x0000000000000000000000000000000000000000000000000000000000000000");
        consensus.BIP34Height = NEVER;
        consensus.BIP34Hash = uint256S("0x0000000000000000000000000000000000000000000000000000000000000000");
        consensus.BIP65Height = 1; // 000000000000000004c2b624ed5d7756c508d90fd0da2c7c679febfa6c4735f0
        consensus.BIP66Height = 1; // 00000000000000000379eaa19dce8c9b722d46ae6a57c2f1a988119488b50931
        consensus.CSVHeight = NEVER; // 000000000000000004a1b34462cb8aeebd5799177f7a29cf28f2d1961716b5b5
        consensus.SegwitHeight = NEVER; // 0000000000000000001c8018d9cb3b742ef25114f27563e3fc4a1902167f9893
        consensus.MinBIP9WarningHeight = NEVER; // segwit activation height + miner confirmation window
        consensus.powLimit = uint256S("0x0fffff0000000000000000000000000000000000000000000000000000000000");
        consensus.nPowTargetTimespan = 3600; // two weeks
        consensus.nPowTargetSpacing = 120;

        consensus.fPowAllowMinDifficultyBlocks = false;
        consensus.fPowNoRetargeting = false;
        consensus.nRuleChangeActivationThreshold = 1916; // 95% of 2016
        consensus.nMinerConfirmationWindow = 2016; // nPowTargetTimespan / nPowTargetSpacing
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = NEVER; // January 1, 2008
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = NEVER; // December 31, 2008

        // Deployment of Taproot (BIPs 340-342)
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].bit = 2;
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].nStartTime = NEVER; // January 1, 2008
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].nTimeout = NEVER; // December 31, 2008

        consensus.nMinimumChainWork = uint256S("0x0000000000000000000000000000000000000000000000000000000000000000");
        consensus.defaultAssumeValid = uint256S("0x0000000000000000000000000000000000000000000000000000000000000000"); // 654683

        /**
         * The message start string is designed to be unlikely to occur in normal data.
         * The characters are rarely used upper ASCII, not valid as UTF-8, and produce
         * a large 32-bit integer with any alignment.
         */
        pchMessageStart[0] = 0xf8;
        pchMessageStart[1] = 0xdd;
        pchMessageStart[2] = 0xd4;
        pchMessageStart[3] = 0xb8;
        nDefaultPort = 20970;
        nPruneAfterHeight = 100000;
        m_assumed_blockchain_size = 2;
        m_assumed_chain_state_size = 0.2;

        genesis = CreateGenesisBlock(1533029778, 1990615403, 0x1f00ffff, 1, 0 * COIN);
        consensus.hashGenesisBlock = genesis.GetHash();
        assert(consensus.hashGenesisBlock == uint256S("0x000032bd27c65ec42967b7854a49df222abdfae8d9350a61083af8eab2a25e03"));
        assert(genesis.hashMerkleRoot == uint256S("0xc3555790e3804130514a674f3374b451dce058407dad6b9e82e191e198012680"));

        // Note that of those which support the service bits prefix, most only support a subset of
        // possible options.
        // This is fine at runtime as we'll fall back to using them as an addrfetch if they don't support the
        // service bits we want, but we should get them updated to support all service bits wanted by any
        // release ASAP to avoid it where possible.
        vSeeds.emplace_back("seederdns.suqa.org"); // main seeder

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,63);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,5);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,191);
        base58Prefixes[EXT_PUBLIC_KEY] = {0x04, 0x88, 0xB2, 0x4E};
        base58Prefixes[EXT_SECRET_KEY] = {0x04, 0x88, 0xAD, 0xC4};

        bech32_hrp = "sin";

        vFixedSeeds = std::vector<SeedSpec6>(pnSeed6_main, pnSeed6_main + ARRAYLEN(pnSeed6_main));

        fDefaultConsistencyChecks = false;
        fRequireStandard = true;
        m_is_test_chain = false;
        m_is_mockable_chain = false;

        checkpointData = {
            {
                { 0, uint256S("0x000032bd27c65ec42967b7854a49df222abdfae8d9350a61083af8eab2a25e03")},
            }
        };

        chainTxData = ChainTxData{
            // Data from RPC: getchaintxstats 20480 367838b984f02b0bc4bb19337eebbb9e3e4e07d0737699538964e12c0ea58810
            /* nTime    */ 1606299715,
            /* nTxCount */ 48182,
            /* dTxRate  */ 0.01883198098900927,
        };
    }
};

/**
 * Testnet (v3)
 */
class CTestNetParams : public CChainParams {
public:
    CTestNetParams() {
        strNetworkID = CBaseChainParams::TESTNET;

        // IN params
        consensus.nInfinityNodeExpireTime=5040;//720*365 days = 1 year

        // legacy Dash, needs refac
        consensus.nMasternodeBurnSINNODE_1 = 100000;
        consensus.nMasternodeBurnSINNODE_5 = 500000;
        consensus.nMasternodeBurnSINNODE_10 = 1000000;

        // node number bounds
        consensus.nLimitSINNODE_1=6;
        consensus.nLimitSINNODE_5=6;
        consensus.nLimitSINNODE_10=6;

        // IN params
        consensus.nInfinityNodeBeginHeight=100;
        consensus.nInfinityNodeGenesisStatement=110;
        consensus.nInfinityNodeUpdateMeta=5;
        consensus.nInfinityNodeVoteValue=100;
        consensus.nInfinityNodeNotificationValue=1;
        consensus.nInfinityNodeCallLockRewardDeepth=12;
        consensus.nInfinityNodeCallLockRewardLoop=5; //in number of blocks
        consensus.nInfinityNodeLockRewardTop=20; //in number
        consensus.nInfinityNodeLockRewardSigners=3; //in number
        consensus.nInfinityNodeLockRewardSINType=10; //in number
        consensus.nSchnorrActivationHeight = 1350000; // wait for active
        consensus.nInfinityNodeExpireTime=5040;//720*365 days = 1 year

        /*Previously used as simple constants in validation */
        consensus.nINActivationHeight = 100; // Activation of IN payments, should also be the same as nInfinityNodeBeginHeight in primitives/block.cpp
        consensus.nINEnforcementHeight = 120; // Enforcement of IN payments
        consensus.nDINActivationHeight = 2880; // Activation of DIN 1.0 payments, and new dev fee address.

        // height at which we fork to X25X
        consensus.nX25XForkHeight = 170000;

        //LWMA diff algo params
        consensus.lwmaStartHeight = 150;
        consensus.lwmaAveragingWindow = 96;

        // IN reorg bounds have been parameterised
        nMaxReorganizationDepth = 14; // 55 at 2 minute block timespan is +/- 120 minutes/2h.
        nDeltaChangeHeight = 0;

        // proof-of-stake: activation and params
        consensus.nStartPoSHeight = 1000;
        consensus.nStakeMinDepth = 10;
        consensus.posLimit = uint256S("7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.fPoSNoRetargeting = false;
        consensus.nPoS_EMATargetTimespan = 30 * 60;
        consensus.nTimeSlotLength = 15;
        consensus.nPoSMinStakeValue = 1;

        consensus.signet_blocks = false;
        consensus.signet_challenge.clear();
        consensus.nSubsidyHalvingInterval = 210000;
        consensus.BIP16Exception = uint256S("0000000000000000000000000000000000000000000000000000000000000000");
        consensus.BIP34Height = 1400;
        consensus.BIP34Hash = uint256S("0000000000000000000000000000000000000000000000000000000000000000");
        consensus.BIP65Height = 1401; // 00000000007f6655f22f98e72ed80d8b06dc761d5da09df0fa1dc4be4f861eb6
        consensus.BIP66Height = 1402; // 000000002104c8c45e99a8853285a3b592602a3ccde2b832481da85e9e4ba182
        consensus.CSVHeight = 1403; // 00000000025e930139bac5c6c31a403776da130831ab85be56578f3fa75369bb
        consensus.SegwitHeight = 1404; // 00000000002b980fcd729daaa248fd9316a5200e9b367f4ff2c42453e84201ca
        consensus.MinBIP9WarningHeight = 3420; // segwit activation height + miner confirmation window
        consensus.powLimit = uint256S("0000ffff00000000000000000000000000000000000000000000000000000000");
        consensus.nPowTargetTimespan = 3600; // two weeks
        consensus.nPowTargetSpacing = 120;
        consensus.fPowAllowMinDifficultyBlocks = true;
        consensus.fPowNoRetargeting = false;
        consensus.nRuleChangeActivationThreshold = 1512; // 75% for testchains
        consensus.nMinerConfirmationWindow = 2016; // nPowTargetTimespan / nPowTargetSpacing
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 1199145601; // January 1, 2008
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = 1230767999; // December 31, 2008

        // Deployment of Taproot (BIPs 340-342)
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].bit = 2;
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].nStartTime = 1199145601; // January 1, 2008
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].nTimeout = 1230767999; // December 31, 2008

        consensus.nMinimumChainWork = uint256S("0000000000000000000000000000000000000000000000000000000000000000");
        consensus.defaultAssumeValid = uint256S("0000000000000000000000000000000000000000000000000000000000000000"); // 1864000

        consensus.devAddressPubKey = "841e6bf56b99a59545da932de2efb23ab93b4f44";
        consensus.devAddress = "SZLafuDjnjqh2tAfTrG9ZAGzbP8HkzNXvB";
        consensus.devAddress2PubKey = "c07290a27153f8adaf01e6f5817405a32f569f61";
        consensus.devAddress2 = "STEkkU29v5rjb6CMUdGciF1e4STZ6jx7aq";
        consensus.cBurnAddress = "SinBurnAddress123456789SuqaXbx3AMC";
        consensus.cBurnAddressPubKey = "ebaf5ec74cb2e2342dfda0229111738ff4dc742d";
        consensus.cMetadataAddress = "SinBurnAddressForMetadataXXXXEU2mj";
        consensus.cNotifyAddress = "SinBurnAddressForNotifyXXXXXc42TcT";
        consensus.cLockRewardAddress = "SinBurnAddressForLockRewardXTbeffB";
        consensus.cGovernanceAddress = "SinBurnAddressGovernanceVoteba5vkQ";

        pchMessageStart[0] = 0xb8;
        pchMessageStart[1] = 0xfd;
        pchMessageStart[2] = 0xf4;
        pchMessageStart[3] = 0xd8;
        nDefaultPort = 20980;
        nPruneAfterHeight = 1000;
        m_assumed_blockchain_size = 0;
        m_assumed_chain_state_size = 0;

        genesis = CreateGenesisBlock(1457163389, 2962201989, 0x1f00ffff, 1, 0 * COIN);
        consensus.hashGenesisBlock = genesis.GetHash();
        //assert(consensus.hashGenesisBlock == uint256S("0x000000000933ea01ad0ee984209779baaec3ced90fa3f408719526f8d77f4943"));
        //assert(genesis.hashMerkleRoot == uint256S("0x4a5e1e4baab89f3a32518a88c31bc87f618f76673e2cc77ab2127b7afdeda33b"));

        vFixedSeeds.clear();
        vSeeds.clear();
        // nodes with support for servicebits filtering should be at the top
        vSeeds.push_back("testnetseeder.suqa.org"); //Testnet SIN dns seeder

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,63);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,5);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,191);
        base58Prefixes[EXT_PUBLIC_KEY] = {0x04, 0x35, 0x87, 0xCF};
        base58Prefixes[EXT_SECRET_KEY] = {0x04, 0x35, 0x83, 0x94};

        bech32_hrp = "tsin";

        vFixedSeeds = std::vector<SeedSpec6>(pnSeed6_test, pnSeed6_test + ARRAYLEN(pnSeed6_test));

        fDefaultConsistencyChecks = false;
        fRequireStandard = false;
        m_is_test_chain = true;
        m_is_mockable_chain = false;

        // IN reorg bounds have been parameterised
        nMaxReorganizationDepth = 14; // 55 at 2 minute block timespan is +/- 120 minutes/2h.
        nDeltaChangeHeight = 0;

        checkpointData = {
        };

        chainTxData = ChainTxData{
        };
    }
};

/**
 * Signet
 */
class SigNetParams : public CChainParams {
public:
    explicit SigNetParams(const ArgsManager& args) {
        std::vector<uint8_t> bin;
        vSeeds.clear();

        if (!args.IsArgSet("-signetchallenge")) {
            bin = ParseHex("512103ad5e0edad18cb1f0fc0d28a3d4f1f3e445640337489abb10404f2d1e086be430210359ef5021964fe22d6f8e05b2463c9540ce96883fe3b278760f048f5189f2e6c452ae");
            vSeeds.emplace_back("178.128.221.177");
            vSeeds.emplace_back("2a01:7c8:d005:390::5");
            vSeeds.emplace_back("ntv3mtqw5wt63red.onion:38333");

            consensus.nMinimumChainWork = uint256S("0x00000000000000000000000000000000000000000000000000000019fd16269a");
            consensus.defaultAssumeValid = uint256S("0x0000002a1de0f46379358c1fd09906f7ac59adf3712323ed90eb59e4c183c020"); // 9434
            m_assumed_blockchain_size = 1;
            m_assumed_chain_state_size = 0;
            chainTxData = ChainTxData{
                // Data from RPC: getchaintxstats 4096 0000002a1de0f46379358c1fd09906f7ac59adf3712323ed90eb59e4c183c020
                /* nTime    */ 1603986000,
                /* nTxCount */ 9582,
                /* dTxRate  */ 0.00159272030651341,
            };
        } else {
            const auto signet_challenge = args.GetArgs("-signetchallenge");
            if (signet_challenge.size() != 1) {
                throw std::runtime_error(strprintf("%s: -signetchallenge cannot be multiple values.", __func__));
            }
            bin = ParseHex(signet_challenge[0]);

            consensus.nMinimumChainWork = uint256{};
            consensus.defaultAssumeValid = uint256{};
            m_assumed_blockchain_size = 0;
            m_assumed_chain_state_size = 0;
            chainTxData = ChainTxData{
                0,
                0,
                0,
            };
            LogPrintf("Signet with challenge %s\n", signet_challenge[0]);
        }

        if (args.IsArgSet("-signetseednode")) {
            vSeeds = args.GetArgs("-signetseednode");
        }

        strNetworkID = CBaseChainParams::SIGNET;
        consensus.signet_blocks = true;
        consensus.signet_challenge.assign(bin.begin(), bin.end());
        consensus.nSubsidyHalvingInterval = 210000;
        consensus.BIP16Exception = uint256{};
        consensus.BIP34Height = 1;
        consensus.BIP34Hash = uint256{};
        consensus.BIP65Height = 1;
        consensus.BIP66Height = 1;
        consensus.CSVHeight = 1;
        consensus.SegwitHeight = 1;
        consensus.nPowTargetTimespan = 14 * 24 * 60 * 60; // two weeks
        consensus.nPowTargetSpacing = 10 * 60;
        consensus.fPowAllowMinDifficultyBlocks = false;
        consensus.fPowNoRetargeting = false;
        consensus.nRuleChangeActivationThreshold = 1916; // 95% of 2016
        consensus.nMinerConfirmationWindow = 2016; // nPowTargetTimespan / nPowTargetSpacing
        consensus.MinBIP9WarningHeight = 0;
        consensus.powLimit = uint256S("00000377ae000000000000000000000000000000000000000000000000000000");
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 1199145601; // January 1, 2008
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = 1230767999; // December 31, 2008

        // Activation of Taproot (BIPs 340-342)
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].bit = 2;
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].nStartTime = Consensus::BIP9Deployment::ALWAYS_ACTIVE;
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;

        // message start is defined as the first 4 bytes of the sha256d of the block script
        CHashWriter h(SER_DISK, 0);
        h << consensus.signet_challenge;
        uint256 hash = h.GetHash();
        memcpy(pchMessageStart, hash.begin(), 4);

        nDefaultPort = 38333;
        nPruneAfterHeight = 1000;

        genesis = CreateGenesisBlock(1598918400, 52613770, 0x1e0377ae, 1, 50 * COIN);
        consensus.hashGenesisBlock = genesis.GetHash();
        //assert(consensus.hashGenesisBlock == uint256S("0x00000008819873e925422c1ff0f99f7cc9bbb232af63a077a480a3633bee1ef6"));
        //assert(genesis.hashMerkleRoot == uint256S("0x4a5e1e4baab89f3a32518a88c31bc87f618f76673e2cc77ab2127b7afdeda33b"));

        vFixedSeeds.clear();

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,111);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,196);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,239);
        base58Prefixes[EXT_PUBLIC_KEY] = {0x04, 0x35, 0x87, 0xCF};
        base58Prefixes[EXT_SECRET_KEY] = {0x04, 0x35, 0x83, 0x94};

        bech32_hrp = "tb";

        fDefaultConsistencyChecks = false;
        fRequireStandard = true;
        m_is_test_chain = true;
        m_is_mockable_chain = false;
    }
};

/**
 * Regression test
 */
class CRegTestParams : public CChainParams {
public:
    explicit CRegTestParams(const ArgsManager& args) {
        /** Sinovate params START */

        // broken negative fee blocks
        brokenfeeblocksData = brokenfeeblocksDataMain;

        // legacy Dash, needs refac
        consensus.nMasternodeBurnSINNODE_1 = 100000;
        consensus.nMasternodeBurnSINNODE_5 = 500000;
        consensus.nMasternodeBurnSINNODE_10 = 1000000;

        // node number bounds
        consensus.nLimitSINNODE_1=375;
        consensus.nLimitSINNODE_5=375;
        consensus.nLimitSINNODE_10=375;

        // IN params
        consensus.nInfinityNodeBeginHeight=100;
        consensus.nInfinityNodeGenesisStatement=110;
        consensus.nInfinityNodeUpdateMeta=5;
        consensus.nInfinityNodeNotificationValue=1;
        consensus.nInfinityNodeCallLockRewardDeepth=5;
        consensus.nInfinityNodeCallLockRewardLoop=2; //in number of blocks
        consensus.nInfinityNodeLockRewardTop=5; //in number
        consensus.nInfinityNodeLockRewardSigners=2; //in number
        consensus.nInfinityNodeLockRewardSINType=1; //in number
        consensus.nSchnorrActivationHeight = 1350000; // wait for active
        consensus.nInfinityNodeExpireTime=262800;//720*365 days = 1 year

        /*Previously used as simple constants in validation */
        consensus.nINActivationHeight = 5000; // Activation of IN payments, should also be the same as nInfinityNodeBeginHeight in primitives/block.cpp
        consensus.nINEnforcementHeight = 5500; // Enforcement of IN payments
        consensus.nDINActivationHeight = 550000; // Activation of DIN 1.0 payments, and new dev fee address.

        // height at which we fork to X25X
        consensus.nX25XForkHeight = 500;

        //LWMA diff algo params
        consensus.lwmaStartHeight = 130;
        consensus.lwmaAveragingWindow = 96;

        // IN reorg bounds have been parameterised
        nMaxReorganizationDepth = 55; // 55 at 2 minute block timespan is +/- 120 minutes/2h.
        nDeltaChangeHeight = 617000;

        // proof-of-stake: activation and params
        consensus.nStartPoSHeight = 250;
        consensus.nStakeMinDepth = 10;
        consensus.posLimit = uint256S("7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.fPoSNoRetargeting = true;
        consensus.nPoS_EMATargetTimespan = 30 * 60;
        consensus.nTimeSlotLength = 15;
        consensus.nPoSMinStakeValue = 1;

        consensus.devAddress2PubKey = "d63bf3a5822bb2f7ac9ced84ae2c1f319c4253e2";
        consensus.devAddress2 = "n13iidFw2jiVVoz86ouMqv31x7oEe5V4Wm";
        consensus.devAddressPubKey = "d63bf3a5822bb2f7ac9ced84ae2c1f319c4253e2";
        consensus.devAddress = "n13iidFw2jiVVoz86ouMqv31x7oEe5V4Wm";
        consensus.cBurnAddressPubKey = "76a9142be2e66836eda517af05e5b628eb9fedefcd669b88ac";
        consensus.cBurnAddress = "mjX1AbMEHU14PmHjG2wtSvoydnJ6RxYwC2";
        consensus.cMetadataAddress = "mueP7L3nMXdshqPEMZ3L5wJumKqhq5dFpm";
        consensus.cNotifyAddress = "mobk9h9A3QLYKsKw9xWSC4bqYSUsqEwnpk";
        consensus.cLockRewardAddress = "n3NZ5A6WKiKRMZWu4b1WiHxJjgjza1RMRk";
        consensus.cGovernanceAddress = "mgmp6o3V4z3kU83QFbNrdtRKGFS6T9yQyB";

        /** Sinovate params END */
        strNetworkID =  CBaseChainParams::REGTEST;
        consensus.signet_blocks = false;
        consensus.signet_challenge.clear();
        consensus.nSubsidyHalvingInterval = 150;
        consensus.BIP16Exception = uint256();
        consensus.BIP34Height = 500; // BIP34 activated on regtest (Used in functional tests)
        consensus.BIP34Hash = uint256();
        consensus.BIP65Height = 1351; // BIP65 activated on regtest (Used in functional tests)
        consensus.BIP66Height = 1251; // BIP66 activated on regtest (Used in functional tests)
        consensus.CSVHeight = 432; // CSV activated on regtest (Used in rpc activation tests)
        consensus.SegwitHeight = 0; // SEGWIT is always activated on regtest unless overridden
        consensus.MinBIP9WarningHeight = 0;
        consensus.powLimit = uint256S("7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.nPowTargetTimespan = 3600;
        consensus.nPowTargetSpacing = 120;
        consensus.fPowAllowMinDifficultyBlocks = true;
        consensus.fPowNoRetargeting = true;
        consensus.nRuleChangeActivationThreshold = 108; // 75% for testchains
        consensus.nMinerConfirmationWindow = 144; // Faster than normal for regtest (144 instead of 2016)
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].bit = 2;
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].nStartTime = Consensus::BIP9Deployment::ALWAYS_ACTIVE;
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;

        consensus.nMinimumChainWork = uint256{};
        consensus.defaultAssumeValid = uint256{};

        pchMessageStart[0] = 0xfa;
        pchMessageStart[1] = 0xbf;
        pchMessageStart[2] = 0xb5;
        pchMessageStart[3] = 0xda;
        nDefaultPort = 18444;
        nPruneAfterHeight = 1000;
        m_assumed_blockchain_size = 0;
        m_assumed_chain_state_size = 0;

        UpdateActivationParametersFromArgs(args);

        genesis = CreateGenesisBlock(1296688602, 3, 0x207fffff, 1, 50 * COIN);
        consensus.hashGenesisBlock = genesis.GetHash();
        assert(consensus.hashGenesisBlock == uint256S("0x1cf45e8c265c41a6c29e40a285cd635924c7658e2334c19829c3722777cd4823"));
        assert(genesis.hashMerkleRoot == uint256S("0x2fa6ca3a7c3115918d274574d4016a660e9d9dec86ea984d8815b68e956bb24a"));

        vFixedSeeds.clear(); //!< Regtest mode doesn't have any fixed seeds.
        vSeeds.clear();      //!< Regtest mode doesn't have any DNS seeds.

        fDefaultConsistencyChecks = true;
        fRequireStandard = true;
        m_is_test_chain = true;
        m_is_mockable_chain = true;

        checkpointData = {
            {
                {0, uint256S("1cf45e8c265c41a6c29e40a285cd635924c7658e2334c19829c3722777cd4823")},
            }
        };

        chainTxData = ChainTxData{
            0,
            0,
            0
        };

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,111);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,196);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,239);
        base58Prefixes[EXT_PUBLIC_KEY] = {0x04, 0x35, 0x87, 0xCF};
        base58Prefixes[EXT_SECRET_KEY] = {0x04, 0x35, 0x83, 0x94};

        bech32_hrp = "bcrt";
    }

    /**
     * Allows modifying the Version Bits regtest parameters.
     */
    void UpdateVersionBitsParameters(Consensus::DeploymentPos d, int64_t nStartTime, int64_t nTimeout)
    {
        consensus.vDeployments[d].nStartTime = nStartTime;
        consensus.vDeployments[d].nTimeout = nTimeout;
    }
    void UpdateActivationParametersFromArgs(const ArgsManager& args);
};

void CRegTestParams::UpdateActivationParametersFromArgs(const ArgsManager& args)
{
    if (args.IsArgSet("-segwitheight")) {
        int64_t height = args.GetArg("-segwitheight", consensus.SegwitHeight);
        if (height < -1 || height >= std::numeric_limits<int>::max()) {
            throw std::runtime_error(strprintf("Activation height %ld for segwit is out of valid range. Use -1 to disable segwit.", height));
        } else if (height == -1) {
            LogPrintf("Segwit disabled for testing\n");
            height = std::numeric_limits<int>::max();
        }
        consensus.SegwitHeight = static_cast<int>(height);
    }

    if (!args.IsArgSet("-vbparams")) return;

    for (const std::string& strDeployment : args.GetArgs("-vbparams")) {
        std::vector<std::string> vDeploymentParams;
        boost::split(vDeploymentParams, strDeployment, boost::is_any_of(":"));
        if (vDeploymentParams.size() != 3) {
            throw std::runtime_error("Version bits parameters malformed, expecting deployment:start:end");
        }
        int64_t nStartTime, nTimeout;
        if (!ParseInt64(vDeploymentParams[1], &nStartTime)) {
            throw std::runtime_error(strprintf("Invalid nStartTime (%s)", vDeploymentParams[1]));
        }
        if (!ParseInt64(vDeploymentParams[2], &nTimeout)) {
            throw std::runtime_error(strprintf("Invalid nTimeout (%s)", vDeploymentParams[2]));
        }
        bool found = false;
        for (int j=0; j < (int)Consensus::MAX_VERSION_BITS_DEPLOYMENTS; ++j) {
            if (vDeploymentParams[0] == VersionBitsDeploymentInfo[j].name) {
                UpdateVersionBitsParameters(Consensus::DeploymentPos(j), nStartTime, nTimeout);
                found = true;
                LogPrintf("Setting version bits activation parameters for %s to start=%ld, timeout=%ld\n", vDeploymentParams[0], nStartTime, nTimeout);
                break;
            }
        }
        if (!found) {
            throw std::runtime_error(strprintf("Invalid deployment (%s)", vDeploymentParams[0]));
        }
    }
}

static std::unique_ptr<const CChainParams> globalChainParams;

const CChainParams &Params() {
    assert(globalChainParams);
    return *globalChainParams;
}

std::unique_ptr<const CChainParams> CreateChainParams(const ArgsManager& args, const std::string& chain)
{
    if (chain == CBaseChainParams::MAIN) {
        return std::unique_ptr<CChainParams>(new CMainParams());
    } else if (chain == CBaseChainParams::TESTNET) {
        return std::unique_ptr<CChainParams>(new CTestNetParams());
    } else if (chain == CBaseChainParams::SIGNET) {
        return std::unique_ptr<CChainParams>(new SigNetParams(args));
    } else if (chain == CBaseChainParams::REGTEST) {
        return std::unique_ptr<CChainParams>(new CRegTestParams(args));
    }
    throw std::runtime_error(strprintf("%s: Unknown chain %s.", __func__, chain));
}

void SelectParams(const std::string& network)
{
    SelectBaseParams(network);
    globalChainParams = CreateChainParams(gArgs, network);
}
