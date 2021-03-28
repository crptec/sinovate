// Copyright (c) 2018-2019 SIN developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <sinovate/rpc/infinitynode.h>
#include <rpc/server.h>
#include <rpc/util.h>

static RPCHelpMan infinitynode()
{
    return RPCHelpMan{"infinitynode",
                "\nGet detailed information about infinitynode and sinovate network\n",
                {
                    {"strCommand", RPCArg::Type::STR, RPCArg::Optional::NO, "The command"},
                    {"strFilter", RPCArg::Type::STR, /* default */ "empty fiter", "The filter of command"},
                    {"strOption", RPCArg::Type::STR, /* default */ "empty option", "The option of command"},
                },
                RPCResult{RPCResult::Type::OBJ, "", "", {
                }},
                RPCExamples{
                    "\nCreate a new Private/Public key\n"
                    + HelpExampleCli("infinitynode", "keypair")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    std::string strCommand;
    std::string strFilter = "";
    std::string strOption = "";
    std::string strError;

    if (request.params.size() >= 1) {
        strCommand = request.params[0].get_str();
    }
    if (request.params.size() == 2) strFilter = request.params[1].get_str();
    if (request.params.size() == 3) {
        strFilter = request.params[1].get_str();
        strOption = request.params[2].get_str();
    }
    if (request.params.size() > 4)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Too many parameters");

    if (request.fHelp  ||
        (strCommand != "build-list" && strCommand != "show-lastscan" && strCommand != "show-infos" && strCommand != "stats"
                                    && strCommand != "show-lastpaid" && strCommand != "build-stm" && strCommand != "show-stm"
                                    && strCommand != "show-candidate" && strCommand != "show-script" && strCommand != "show-proposal"
                                    && strCommand != "scan-vote" && strCommand != "show-proposals" && strCommand != "keypair"
                                    && strCommand != "mypeerinfo" && strCommand != "checkkey" && strCommand != "scan-metadata"
                                    && strCommand != "show-metadata" && strCommand != "memory-lockreward"
                                    && strCommand != "show-lockreward" &&  strCommand != "check-lockreward"
                                    && strCommand != "show-all-infos" &&  strCommand != "getblockcount" && strCommand != "getrawblockcount"
                                    && strCommand != "show-metapubkey" &&  strCommand != "show-online"
        ))
            throw std::runtime_error(
                "infinitynode \"command\"...\n"
                "Set of commands to execute infinitynode related actions\n"
                "\nArguments:\n"
                "1. \"command\"        (string or set of strings, required) The command to execute\n"
                "\nAvailable commands:\n"
                "  keypair                     - Print a newly generated compressed key pair\n"
                "  checkkey                    - Print information about provided privatekey\n"
                "  check-lockreward            - Return the status of Register string\n"
                "  mypeerinfo                  - Print InfinityNode status\n"
                "  getblockcount               - InfinityNode current height\n"
                "  getrawblockcount            - InfinityNode current height from an atomic raw index\n"
                "  build-list                  - Build a list of all InfinityNodes from InfinityNode genesis block to current tip\n"
                "  build-stm                   - Build a list of all statements from the InfinityNode genesis block to current tip\n"
                "  show-infos                  - Print a list of all active InfinityNodes and latest information about them\n"
                "  show-all-infos              - Print a list of all non-matured InfinityNodes and latest information about them\n"
                "  show-lastscan               - Print last height the InfinityNode list was updated\n"
                "  show-lastpaid               - Print all last paid InfinityNode info\n"
                "  show-stm                    - Print last statement for each InfinityNode tier\n"
                "  show-metadata               - Print a list of all on-chain InfinityNode metadata\n"
                "  show-candidate nHeight      - Print which node is candidate for a reward at given height\n"
                "  scan-vote                   - Scan and update the chain for on-chain InfinityNode votes\n"
                "  show-lockreward             - Print LockReward information for current height\n"
                "  scan-metadata               - Build/update list of on-chain InfinityNode metadata\n"
                "  memory-lockreward           - Return how much memory is currently used by LockReward\n"
                );

    UniValue obj(UniValue::VOBJ);

    if (strCommand == "keypair")
    {
        CKey secret;
        secret.MakeNewKey(true);
        CPubKey pubkey = secret.GetPubKey();
        assert(secret.VerifyPubKey(pubkey));

        std::string sBase64 = EncodeBase64(pubkey);
        std::vector<unsigned char> tx_data = DecodeBase64(sBase64.c_str());
        CPubKey decodePubKey(tx_data.begin(), tx_data.end());
        CTxDestination dest = GetDestinationForKey(decodePubKey, DEFAULT_ADDRESS_TYPE);

        obj.pushKV("PrivateKey", EncodeSecret(secret));
        obj.pushKV("PublicKey", sBase64);
        obj.pushKV("DecodePublicKey", decodePubKey.GetID().ToString());
        obj.pushKV("Address", EncodeDestination(dest));
        obj.pushKV("isCompressed", pubkey.IsCompressed());


        return obj;
    }

    if (strCommand == "checkkey")
    {
        std::string strKey = request.params[1].get_str();
        CKey secret = DecodeSecret(strKey);
        if (!secret.IsValid()) throw JSONRPCError(RPC_INTERNAL_ERROR, "Not a valid key");

        CPubKey pubkey = secret.GetPubKey();
        assert(secret.VerifyPubKey(pubkey));

        std::string sBase64 = EncodeBase64(pubkey);
        std::vector<unsigned char> tx_data = DecodeBase64(sBase64.c_str());
        CPubKey decodePubKey(tx_data.begin(), tx_data.end());
        CTxDestination dest = GetDestinationForKey(decodePubKey, DEFAULT_ADDRESS_TYPE);

        obj.pushKV("PrivateKey", EncodeSecret(secret));
        obj.pushKV("PublicKey", sBase64);
        obj.pushKV("DecodePublicKey", decodePubKey.GetID().ToString());
        obj.pushKV("Address", EncodeDestination(dest));
        obj.pushKV("isCompressed", pubkey.IsCompressed());

        return obj;
    }

    if (strCommand == "check-lockreward")
    {
        if (request.params.size() < 4)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Correct usage is 'infinitynode check-lockreward \"BurnTxId\" \"index\" \"RegisterString\" '");

        uint256 txId;
        txId = uint256S(request.params[1].get_str());

        std::string index = request.params[2].get_str();
        int i = atoi(index);

        COutPoint outpointBurnTx = COutPoint(txId, i);

        std::string strRegisterInfo = request.params[3].get_str();
        std::string error = "";

        bool result = inflockreward.CheckLockRewardRegisterInfo(strRegisterInfo, error, outpointBurnTx);

        obj.pushKV("Result", result);
        obj.pushKV("Error string", error);

        return obj;
    }

    if (strCommand == "getblockcount")
    {
        if (!fInfinityNode)
            throw JSONRPCError(RPC_INTERNAL_ERROR, "This is not an InfinityNode");

        return infinitynodePeer.getCacheHeightInf();
    }

    if (strCommand == "getrawblockcount")
    {
        if (!fInfinityNode)
            throw JSONRPCError(RPC_INTERNAL_ERROR, "This is not an InfinityNode");
        int ret = nRawBlockCount;
        return ret;
    }

    if (strCommand == "mypeerinfo")
    {
        if (!fInfinityNode)
            throw JSONRPCError(RPC_INTERNAL_ERROR, "This is not an InfinityNode");

        NodeContext& node = EnsureNodeContext(request.context);
        if(!node.connman)
            throw JSONRPCError(RPC_CLIENT_P2P_DISABLED, "Error: Peer-to-peer functionality missing or disabled");

        UniValue infObj(UniValue::VOBJ);
        infinitynodePeer.ManageState(*node.connman);
        infObj.pushKV("MyPeerInfo", infinitynodePeer.GetMyPeerInfo());
        return infObj;
    }

    if (strCommand == "build-list")
    {
        if (request.params.size() == 1) {
            CBlockIndex* pindex = NULL;
            LOCK(cs_main); // make sure this scopes until we reach the function which needs this most, buildInfinitynodeListRPC()
            pindex = ::ChainActive().Tip();
            return infnodeman.buildInfinitynodeList(1, pindex->nHeight);
        }

        std::string strMode = request.params[1].get_str();

        if (strMode == "lastscan")
            return infnodeman.getLastScan();
    }

    if (strCommand == "build-stm")
    {
        CBlockIndex* pindex = NULL;
        {
                LOCK(cs_main);
                pindex = ::ChainActive().Tip();
        }
        bool updateStm = false;
        LOCK(cs_main);
        updateStm = infnodeman.buildInfinitynodeList(1, pindex->nHeight);
        obj.pushKV("Height", pindex->nHeight);
        obj.pushKV("Result", updateStm);
        return obj;
    }

    if (strCommand == "show-stm")
    {
        return infnodeman.getLastStatementString();
    }

    if (strCommand == "show-candidate")
    {
        if (request.params.size() != 2)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Correct usage is 'infinitynode show-candidate \"nHeight\"'");
        int nextHeight = 10;
        nextHeight = atoi(strFilter);

        if (nextHeight < Params().GetConsensus().nInfinityNodeGenesisStatement) {
            strError = strprintf("nHeight must be higher than the Genesis Statement height (%s)", Params().GetConsensus().nInfinityNodeGenesisStatement);
            throw JSONRPCError(RPC_INVALID_PARAMETER, strError);
        }

        CInfinitynode infBIG, infMID, infLIL;
        LOCK(infnodeman.cs);
        infnodeman.deterministicRewardAtHeight(nextHeight, 10, infBIG);
        infnodeman.deterministicRewardAtHeight(nextHeight, 5, infMID);
        infnodeman.deterministicRewardAtHeight(nextHeight, 1, infLIL);

        obj.pushKV("Candidate BIG: ", infBIG.getCollateralAddress());
        obj.pushKV("Candidate MID: ", infMID.getCollateralAddress());
        obj.pushKV("Candidate LIL: ", infLIL.getCollateralAddress());

        return obj;
    }

    if (strCommand == "show-lastscan")
    {
            return infnodeman.getLastScan();
    }

    if (strCommand == "show-lastpaid")
    {
        std::map<CScript, int>  mapLastPaid = infnodeman.GetFullLastPaidMap();
        for (auto& pair : mapLastPaid) {
            std::string scriptPublicKey = pair.first.ToString();
            obj.pushKV(scriptPublicKey, pair.second);
        }
        return obj;
    }

    if (strCommand == "show-infos")
    {
        std::map<COutPoint, CInfinitynode> mapInfinitynodes = infnodeman.GetFullInfinitynodeMap();
        std::map<std::string, CMetadata> mapInfMetadata = infnodemeta.GetFullNodeMetadata();
        for (auto& infpair : mapInfinitynodes) {
            std::string strOutpoint = infpair.first.ToStringFull();
            CInfinitynode inf = infpair.second;
            CMetadata meta = mapInfMetadata[inf.getMetaID()];
            std::string nodeAddress = "NodeAddress";

            if (meta.getMetaPublicKey() != "") nodeAddress = meta.getMetaPublicKey();

                std::ostringstream streamInfo;
                streamInfo << std::setw(8) <<
                               inf.getCollateralAddress() << " " <<
                               inf.getHeight() << " " <<
                               inf.getExpireHeight() << " " <<
                               inf.getRoundBurnValue() << " " <<
                               inf.getSINType() << " " <<
                               inf.getBackupAddress() << " " <<
                               inf.getLastRewardHeight() << " " <<
                               inf.getRank() << " " << 
                               infnodeman.getLastStatementSize(inf.getSINType()) << " " <<
                               inf.getMetaID() << " " <<
                               nodeAddress << " " <<
                               meta.getService().ToString()
                               ;
                std::string strInfo = streamInfo.str();
                obj.pushKV(strOutpoint, strInfo);
        }
        return obj;
    }

    if (strCommand == "show-all-infos")
    {
        std::map<COutPoint, CInfinitynode> mapInfinitynodes = infnodeman.GetFullInfinitynodeNonMaturedMap();
        for (auto& infpair : mapInfinitynodes) {
            std::string strOutpoint = infpair.first.ToStringFull();
            CInfinitynode inf = infpair.second;

                std::ostringstream streamInfo;
                streamInfo << std::setw(8) <<
                               inf.getCollateralAddress() << " " <<
                               inf.getHeight() << " " <<
                               inf.getExpireHeight() << " " <<
                               inf.getRoundBurnValue() << " " <<
                               inf.getSINType() << " "
                               ;
                std::string strInfo = streamInfo.str();
                obj.pushKV(strOutpoint, strInfo);
        }
        return obj;
    }

    if (strCommand == "show-online")
    {

        if (request.params.size() != 2)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Correct usage is 'infinitynode show-online \"nHeight\"'");
        int nextHeight = 550000;//forkHeight of DIN v1.0
        nextHeight = atoi(strFilter);

        std::map<COutPoint, CInfinitynode> mapInfinitynodes = infnodeman.GetFullInfinitynodeMap();
        for (auto& infpair : mapInfinitynodes) {
            std::string strOutpoint = infpair.first.ToStringFull();
            CInfinitynode inf = infpair.second;

            std::string sSINType = "NA";
            if(inf.getSINType() == 1) {sSINType="MINI";}
            if(inf.getSINType() == 5) {sSINType="MID";}
            if(inf.getSINType() == 10) {sSINType="BIG";}

            std::string sMetaInfo = "Missing";
            std::string sNodeAddress = "";
            CMetadata metaSender = infnodemeta.Find(inf.getMetaID());
            if (metaSender.getMetadataHeight() > 0){
                sMetaInfo = "NodeAddress:";
                std::string metaPublicKey = metaSender.getMetaPublicKey();
                std::vector<unsigned char> tx_data = DecodeBase64(metaPublicKey.c_str());
                CPubKey pubKey(tx_data.begin(), tx_data.end());

                    CTxDestination dest = GetDestinationForKey(pubKey, OutputType::LEGACY);
                    sNodeAddress = EncodeDestination(dest);
            }

            std::string sExpire = "Alive";
            if(inf.getExpireHeight() <= nextHeight) {
                    sExpire = "Expired";
            }

                std::ostringstream streamInfo;
                streamInfo << std::setw(8) <<
                               inf.getCollateralAddress() << " " <<
                               inf.getHeight() << " " <<
                               inf.getExpireHeight() << " " <<
                               inf.getRoundBurnValue() << " " <<
                               sSINType << " " <<
                               sExpire << " " <<
                               sMetaInfo << sNodeAddress << " "
                               ;
                std::string strInfo = streamInfo.str();
                obj.pushKV(strOutpoint, strInfo);
        }
        return obj;
    }

    if (strCommand == "show-script")
    {
        std::map<COutPoint, CInfinitynode> mapInfinitynodes = infnodeman.GetFullInfinitynodeMap();
        for (auto& infpair : mapInfinitynodes) {
            std::string strOutpoint = infpair.first.ToStringFull();
            CInfinitynode inf = infpair.second;
                std::ostringstream streamInfo;
                        std::vector<std::vector<unsigned char>> vSolutions;
                        const CScript& prevScript = inf.getScriptPublicKey();
                        TxoutType whichType = Solver(prevScript, vSolutions);
                        std::string backupAddresstmp(vSolutions[1].begin(), vSolutions[1].end());
                streamInfo << std::setw(8) <<
                               inf.getCollateralAddress() << " " <<
                               backupAddresstmp << " " <<
                               inf.getRoundBurnValue();
                std::string strInfo = streamInfo.str();
                obj.pushKV(strOutpoint, strInfo);
        }
        return obj;
    }

    if (strCommand == "show-proposal")
    {
        if (request.params.size() < 2)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Correct usage is 'infinitynode show-proposal \"ProposalId\" \"(Optional)Mode\" '");

        std::string proposalId  = strFilter;
        std::vector<CVote>* vVote = infnodersv.Find(proposalId);
        obj.pushKV("ProposalId", proposalId);
        if(vVote != NULL){
            obj.pushKV("Votes", (int)vVote->size());
        }else{
            obj.pushKV("Votes", "0");
        }
        int mode = 0;
        if (strOption == "public"){mode=0;}
        if (strOption == "node"){mode=1;}
        if (strOption == "all"){mode=2;}
        obj.pushKV("Yes", infnodersv.getResult(proposalId, true, mode));
        obj.pushKV("No", infnodersv.getResult(proposalId, false, mode));
        for (auto& v : *vVote){
            CTxDestination addressVoter;
            ExtractDestination(v.getVoter(), addressVoter);
            obj.pushKV(EncodeDestination(addressVoter), v.getOpinion());
        }
        return obj;
    }

    if (strCommand == "show-proposals")
    {
        std::map<std::string, std::vector<CVote>> mapCopy = infnodersv.GetFullProposalVotesMap();
        obj.pushKV("Proposal", (int)mapCopy.size());
        for (auto& infpair : mapCopy) {
            obj.pushKV(infpair.first, (int)infpair.second.size());
        }

        return obj;
    }

    if (strCommand == "show-metapubkey"){
        UniValue pubkeyList(UniValue::VARR);
        std::map<std::string, CMetadata>  mapCopy = infnodemeta.GetFullNodeMetadata();
        obj.pushKV("Metadata", (int)mapCopy.size());
        for (auto& infpair : mapCopy) {
            std::ostringstream streamInfo;
            std::vector<unsigned char> tx_data = DecodeBase64(infpair.second.getMetaPublicKey().c_str());

                CPubKey pubKey(tx_data.begin(), tx_data.end());
                CTxDestination nodeDest = GetDestinationForKey(pubKey, OutputType::LEGACY);
                streamInfo << std::setw(8) <<
                EncodeDestination(nodeDest)
                ;
                std::string strInfo = streamInfo.str();
                pubkeyList.push_back(strInfo);
        }
        return pubkeyList;
    }

    if (strCommand == "show-metadata")
    {
        std::map<std::string, CMetadata>  mapCopy = infnodemeta.GetFullNodeMetadata();
        obj.pushKV("Metadata", (int)mapCopy.size());
        for (auto& infpair : mapCopy) {
            std::ostringstream streamInfo;
            std::vector<unsigned char> tx_data = DecodeBase64(infpair.second.getMetaPublicKey().c_str());

                CPubKey pubKey(tx_data.begin(), tx_data.end());
                CTxDestination nodeDest = GetDestinationForKey(pubKey, OutputType::LEGACY);

                streamInfo << std::setw(8) <<
                               infpair.second.getMetaPublicKey() << " " <<
                               infpair.second.getService().ToString() << " " <<
                               infpair.second.getMetadataHeight() << " " <<
                               EncodeDestination(nodeDest)
                ;
                std::string strInfo = streamInfo.str();

            UniValue metaHisto(UniValue::VARR);
            for(auto& v : infpair.second.getHistory()){
                 std::ostringstream vHistoMeta;
                 std::vector<unsigned char> tx_data_h = DecodeBase64(v.pubkeyHisto.c_str());

                 CPubKey pubKey_h(tx_data_h.begin(), tx_data_h.end());
                 CTxDestination nodeDest_h = GetDestinationForKey(pubKey_h, OutputType::LEGACY);

                 vHistoMeta << std::setw(4) <<
                     v.nHeightHisto  << " " <<
                     v.pubkeyHisto << " " <<
                     v.serviceHisto.ToString() << " " <<
                     EncodeDestination(nodeDest_h)
                     ;
                 std::string strHistoMeta = vHistoMeta.str();
                 metaHisto.push_back(strHistoMeta);
            }
            obj.pushKV(infpair.first, strInfo);
            std::string metaHistStr = strprintf("History %s", infpair.first);
            obj.pushKV(metaHistStr, metaHisto);
        }
        return obj;
    }

    if (strCommand == "scan-vote")
    {
        CBlockIndex* pindex = NULL;
        {
                LOCK(cs_main);
                pindex = ::ChainActive().Tip();
        }

        bool result = infnodersv.rsvScan(pindex->nHeight);
        obj.pushKV("Result", result);
        obj.pushKV("Details", infnodersv.ToString());
        return obj;
    }

    if (strCommand == "scan-metadata")
    {
        CBlockIndex* pindex = NULL;
        {
                LOCK(cs_main);
                pindex = ::ChainActive().Tip();
        }

        bool result = infnodemeta.metaScan(pindex->nHeight);
        obj.pushKV("Result", result);
        return obj;
    }

    if (strCommand == "memory-lockreward")
    {
        obj.pushKV("LockReward", inflockreward.GetMemorySize());
        return obj;
    }

    if (strCommand == "show-lockreward")
    {
        CBlockIndex* pindex = NULL;

        LOCK(cs_main);
        pindex = ::ChainActive().Tip();

        std::vector<CLockRewardExtractInfo> vecLockRewardRet;
        if (request.params.size() < 2) vecLockRewardRet = infnodelrinfo.getFullLRInfo();
        if (request.params.size() == 2){
            int nBlockNumber  = atoi(strFilter);
            infnodelrinfo.getLRInfo(nBlockNumber, vecLockRewardRet);
        }

        obj.pushKV("Result", (int)vecLockRewardRet.size());
        obj.pushKV("Current height", pindex->nHeight);
        int i=0;
        for (auto& v : vecLockRewardRet) {
                std::ostringstream streamInfo;
                CTxDestination address;
                bool fValidAddress = ExtractDestination(v.scriptPubKey, address);

                std::string owner = "Unknow";
                if(fValidAddress) owner = EncodeDestination(address);

                streamInfo << std::setw(1) <<
                               v.nSINtype << " " <<
                               owner  << " " <<
                               v.sLRInfo;
                std::string strInfo = streamInfo.str();
                obj.pushKV(strprintf("%d-%d",v.nBlockHeight, i), strInfo);
            i++;
        }
        return obj;
    }

    return NullUniValue;
},
    };
}

/**
 * @xtdevcoin
 * this function help user burn correctly their funds to run infinity node
 */
static RPCHelpMan infinitynodeburnfund()
{
    return RPCHelpMan{"infinitynodeburnfund",
                "\nBurn funds to create Infinitynode.\n"
                "\nReturns JSON info or Null.\n",
                {
                    {"nodeowneraddress", RPCArg::Type::STR, RPCArg::Optional::NO, "Address of owner (will receive the reward)."},
                    {"amount", RPCArg::Type::AMOUNT, RPCArg::Optional::NO, "The amount in " + CURRENCY_UNIT + " to create Node (Example: 100000). "},
                    {"backupaddress", RPCArg::Type::STR, RPCArg::Optional::NO, "backup of owner address"},
                },
                {
                    RPCResult{RPCResult::Type::NONE, "", ""},
                    RPCResult{"",
                        RPCResult::Type::OBJ, "", "",
                        {
                            {RPCResult::Type::STR, "BURNADDRESS", "The BURNADDRESS of sinovate network"},
                            {RPCResult::Type::STR_HEX, "BURNPUBLICKEY", "The public key of owner"},
                            {RPCResult::Type::STR_HEX, "BURNSCRIPT", "The script of burn"},
                            {RPCResult::Type::STR_HEX, "BURNTX", "The transaction id"},
                            {RPCResult::Type::STR, "OWNERADDRESS", "The address of owner from which coins are burned and will receive the reward."},
                            {RPCResult::Type::STR, "BACKUPADDRESS", "The BACKUPADDRESS of owner (use in next feature)"},
                        },
                    },
                },
                RPCExamples{
                    "\nBurn 1 Milion SIN coins to create BIG Infinitynode\n"
                    + HelpExampleCli("infinitynodeburnfund", "NodeOwnerAddress 1000000 SINBackupAddress")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    if (!wallet) return NullUniValue;
    CWallet* const pwallet = wallet.get();

    // Make sure the results are valid at least up to the most recent block
    // the user could have gotten from another RPC command prior to now
    pwallet->BlockUntilSyncedToCurrentChain();

    // Grab locks here as BlockUntilSyncedToCurrentChain() handles them on its own, but we need them for most other funcs
    LOCK2(cs_main, pwallet->cs_wallet);

    std::string strError;
    std::vector<COutput> vPossibleCoins;
    pwallet->AvailableCoins(vPossibleCoins, true, NULL, false);

    UniValue results(UniValue::VARR);
    // Amount

    CTxDestination NodeOwnerAddress = DecodeDestination(request.params[0].get_str());
    if (!IsValidDestination(NodeOwnerAddress))
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid SIN address as NodeOwnerAddress");

    CAmount nAmount = AmountFromValue(request.params[1]);
    if (nAmount != Params().GetConsensus().nMasternodeBurnSINNODE_1 * COIN &&
        nAmount != Params().GetConsensus().nMasternodeBurnSINNODE_5 * COIN &&
        nAmount != Params().GetConsensus().nMasternodeBurnSINNODE_10 * COIN)
    {
        throw JSONRPCError(RPC_TYPE_ERROR, "Invalid amount to burn and run an InfinityNode");
    }

    CTxDestination BKaddress = DecodeDestination(request.params[2].get_str());
    if (!IsValidDestination(BKaddress))
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid SIN address as SINBackupAddress");

    std::map<COutPoint, CInfinitynode> mapInfinitynodes = infnodeman.GetFullInfinitynodeMap();
    int totalNode = 0, totalBIG = 0, totalMID = 0, totalLIL = 0, totalUnknown = 0;
    for (auto& infpair : mapInfinitynodes) {
        ++totalNode;
        CInfinitynode inf = infpair.second;
        int sintype = inf.getSINType();
        if (sintype == 10) ++totalBIG;
        else if (sintype == 5) ++totalMID;
        else if (sintype == 1) ++totalLIL;
        else ++totalUnknown;
    }

    // BurnAddress
    CTxDestination dest = DecodeDestination(Params().GetConsensus().cBurnAddress);
    CScript scriptPubKeyBurnAddress = GetScriptForDestination(dest);
    std::vector<std::vector<unsigned char> > vSolutions;
    TxoutType whichType = Solver(scriptPubKeyBurnAddress, vSolutions);;
    PKHash keyid = PKHash(uint160(vSolutions[0]));

    // Wallet comments
    std::set<CTxDestination> destinations;
    LOCK(pwallet->cs_wallet);
    for (COutput& out : vPossibleCoins) {
        CTxDestination address;
        const CScript& scriptPubKey = out.tx->tx->vout[out.i].scriptPubKey;
        bool fValidAddress = ExtractDestination(scriptPubKey, address);

        if (!fValidAddress || address != NodeOwnerAddress)
            continue;

        UniValue entry(UniValue::VOBJ);

        if (out.tx->tx->vout[out.i].nValue >= nAmount && out.nDepth >= 2) {
            /*check address is unique*/
            for (auto& infpair : mapInfinitynodes) {
                CInfinitynode inf = infpair.second;
                if(inf.getCollateralAddress() == EncodeDestination(address)){
                    strError = strprintf("Error: Address %s exist in list. Please use another address to make sure it is unique.", EncodeDestination(address));
                    throw JSONRPCError(RPC_TYPE_ERROR, strError);
                }
            }
            // Wallet comments
            mapValue_t mapValue;
            bool fSubtractFeeFromAmount = true;
            CCoinControl coin_control;
            coin_control.Select(COutPoint(out.tx->GetHash(), out.i));
            coin_control.destChange = NodeOwnerAddress;//fund go back to NodeOwnerAddress

            CScript script;
            script = GetScriptForBurn(keyid, request.params[2].get_str());

            CAmount nFeeRequired;
            FeeCalculation fee_calc_out;
            bilingual_str strErrorRet;

            std::vector<CRecipient> vecSend;
            int nChangePosRet = -1;
            CRecipient recipient = {script, nAmount, fSubtractFeeFromAmount};
            vecSend.push_back(recipient);


            CTransactionRef tx;
            if (!pwallet->CreateTransaction(vecSend, tx, nFeeRequired, nChangePosRet, strErrorRet, coin_control, fee_calc_out, true)) {
                throw JSONRPCError(RPC_WALLET_ERROR, strErrorRet.original);
            }

            pwallet->CommitTransaction(tx, std::move(mapValue), {} /* orderForm */);

            entry.pushKV("BURNADDRESS", EncodeDestination(dest));
            entry.pushKV("BURNPUBLICKEY", HexStr(keyid));
            entry.pushKV("BURNSCRIPT", HexStr(scriptPubKeyBurnAddress));
            entry.pushKV("BURNTX", tx->GetHash().GetHex());
            entry.pushKV("OWNER_ADDRESS",EncodeDestination(address));
            entry.pushKV("BACKUP_ADDRESS",EncodeDestination(BKaddress));
            //coins is good to burn
            results.push_back(entry);
            break; //immediat
        }
    }
    return results;
},
    };
}

static const CRPCCommand commands[] =
{ //  category              name                      actor (function)                              argNames
  //  --------------------- ------------------------  -----------------------                       ----------
  { "SIN",                  "infinitynode",           &infinitynode,                                {"command"}  },
  { "SIN",                  "infinitynodeburnfund",   &infinitynodeburnfund,                        {"owner_address", "amount", "backup_address"} },
  //  { "SIN",                "infinitynodeburnfund_external", &infinitynodeburnfund_external,        {"inputs array", "owner_address","amount","backup_address"} },
  //  { "SIN",                "infinitynodeupdatemeta", &infinitynodeupdatemeta,                      {"owner_address","node_address","IP"} },
  //  { "SIN",                "infinitynodeupdatemeta_external", &infinitynodeupdatemeta_external,    {"inputs array", "owner_address","node_address","IP"} },

  //  { "SIN",                "infinitynodevote",       &infinitynodevote,                            {"owner_address","proposalid","opinion"} }
};

void RegisterInfinitynodeRPCCommands(CRPCTable &t)
{
    for (const auto& c : commands) {
        t.appendCommand(c.name, &c);
    }
}