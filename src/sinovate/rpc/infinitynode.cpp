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
                    {"strFilter", RPCArg::Type::STR, RPCArg::Optional::OMITTED, "The filter of command"},
                    {"strOption", RPCArg::Type::STR, RPCArg::Optional::OMITTED, "The option of command"},
                },
                {
                    RPCResult{RPCResult::Type::NONE, "", ""},
                    RPCResult{RPCResult::Type::NUM, "", ""},
                    RPCResult{RPCResult::Type::STR, "", ""},
                    RPCResult{RPCResult::Type::BOOL, "", ""},
                    RPCResult{"keypair, checkkey command",
                        RPCResult::Type::OBJ, "", "",
                        {
                            {RPCResult::Type::STR_HEX, "PrivateKey", "The PrivateKey"},
                            {RPCResult::Type::STR_HEX, "PublicKey", "The PublicKey"},
                            {RPCResult::Type::STR_HEX, "DecodePublicKey", "DecodePublicKey"},
                            {RPCResult::Type::STR, "Address", "The Address"},
                            {RPCResult::Type::BOOL, "isCompressed", "isCompressed (true/false)"},
                        },
                    },
                    RPCResult{"mypeerinfo",
                        RPCResult::Type::OBJ, "", "",
                        {
                            {RPCResult::Type::STR, "MyPeerInfo", "The candidate of reward for BIG node"},
                        },
                    },
                    RPCResult{"build-stm",
                        RPCResult::Type::OBJ, "", "",
                        {
                            {RPCResult::Type::STR, "Height", "The candidate of reward for BIG node"},
                            {RPCResult::Type::STR, "Result", "The candidate of reward for MID node"},
                        },
                    },
                    RPCResult{"show-candidate command",
                        RPCResult::Type::OBJ, "", "",
                        {
                            {RPCResult::Type::STR, "CandidateBIG", "The candidate of reward for BIG node"},
                            {RPCResult::Type::STR, "CandidateMID", "The candidate of reward for MID node"},
                            {RPCResult::Type::STR, "CandidateLIL", "The candidate of reward for LIL node"},
                        },
                    },
                },
                RPCExamples{
                    "\nCreate a new Private/Public key\n"
                    + HelpExampleCli("infinitynode", "keypair")
                    + "\nCheck Private key\n"
                    + HelpExampleCli("infinitynode", "checkkey PRIVATEKEY")
                    + "\nInfinitynode: Get current block height\n"
                    + HelpExampleCli("infinitynode", "getrawblockcount")
                    + "\nInfinitynode: show peer info\n"
                    + HelpExampleCli("infinitynode", "mypeerinfo")
                    + "\nBuild statement of Infinitynode from height 1\n"
                    + HelpExampleCli("infinitynode", "build-stm")
                    + "\nShow current statement of Infinitynode\n"
                    + HelpExampleCli("infinitynode", "show-stm")
                    + "\nShow the candidates for Height\n"
                    + HelpExampleCli("infinitynode", "show-candiate height")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    const std::string strCommand = request.params[0].get_str();
    std::string strError;

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
        const std::string strKey = request.params[1].get_str();
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

        const std::string strFilter = request.params[1].get_str();

        int nextHeight = atoi(strFilter);

        if (nextHeight < Params().GetConsensus().nInfinityNodeGenesisStatement) {
            strError = strprintf("nHeight must be higher than the Genesis Statement height (%s)", Params().GetConsensus().nInfinityNodeGenesisStatement);
            throw JSONRPCError(RPC_INVALID_PARAMETER, strError);
        }

        CInfinitynode infBIG, infMID, infLIL;
        LOCK(infnodeman.cs);
        infnodeman.deterministicRewardAtHeight(nextHeight, 10, infBIG);
        infnodeman.deterministicRewardAtHeight(nextHeight, 5, infMID);
        infnodeman.deterministicRewardAtHeight(nextHeight, 1, infLIL);

        obj.pushKV("CandidateBIG: ", infBIG.getCollateralAddress());
        obj.pushKV("CandidateMID: ", infMID.getCollateralAddress());
        obj.pushKV("CandidateLIL: ", infLIL.getCollateralAddress());

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
                RPCResult{
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

    const std::string address = request.params[0].get_str();
    CTxDestination NodeOwnerAddress = DecodeDestination(address);
    if (!IsValidDestination(NodeOwnerAddress)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, std::string("Invalid Bitcoin address: ") + address);
    }

    CAmount nAmount = AmountFromValue(request.params[1]);
    if (nAmount != Params().GetConsensus().nMasternodeBurnSINNODE_1 * COIN &&
        nAmount != Params().GetConsensus().nMasternodeBurnSINNODE_5 * COIN &&
        nAmount != Params().GetConsensus().nMasternodeBurnSINNODE_10 * COIN)
    {
        throw JSONRPCError(RPC_TYPE_ERROR, "Invalid amount to burn and run an InfinityNode");
    }

    const std::string addressbk = request.params[2].get_str();
    CTxDestination BKaddress = DecodeDestination(addressbk);
    if (!IsValidDestination(BKaddress))
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid SIN address as SINBackupAddress");

    std::string strError;
    std::vector<COutput> vPossibleCoins;
    pwallet->AvailableCoins(vPossibleCoins, true, NULL, false);

    UniValue results(UniValue::VARR);

    // BurnAddress
    CTxDestination dest = DecodeDestination(Params().GetConsensus().cBurnAddress);
    CScript scriptPubKeyBurnAddress = GetScriptForDestination(dest);
    std::vector<std::vector<unsigned char> > vSolutions;
    TxoutType whichType = Solver(scriptPubKeyBurnAddress, vSolutions);;
    PKHash keyid = PKHash(uint160(vSolutions[0]));

    //Infinitynode info
    std::map<COutPoint, CInfinitynode> mapInfinitynodes = infnodeman.GetFullInfinitynodeMap();

    // Wallet comments
    std::set<CTxDestination> destinations;
    LOCK(pwallet->cs_wallet);
    for (COutput& out : vPossibleCoins) {
        CTxDestination addressCoin;
        const CScript& scriptPubKey = out.tx->tx->vout[out.i].scriptPubKey;
        bool fValidAddress = ExtractDestination(scriptPubKey, addressCoin);

        if (!fValidAddress || addressCoin != NodeOwnerAddress)
            continue;

        if (out.tx->tx->vout[out.i].nValue >= nAmount && out.nDepth >= 2) {
            /*check address is unique*/
            for (auto& infpair : mapInfinitynodes) {
                CInfinitynode inf = infpair.second;
                if(inf.getCollateralAddress() == EncodeDestination(addressCoin)){
                    strError = strprintf("Error: Address %s exist in list. Please use another address to make sure it is unique.", EncodeDestination(addressCoin));
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

            results.pushKV("BURNADDRESS", EncodeDestination(dest));
            results.pushKV("BURNPUBLICKEY", HexStr(keyid));
            results.pushKV("BURNSCRIPT", HexStr(scriptPubKeyBurnAddress));
            results.pushKV("BURNTX", tx->GetHash().GetHex());
            results.pushKV("OWNER_ADDRESS",EncodeDestination(NodeOwnerAddress));
            results.pushKV("BACKUP_ADDRESS",EncodeDestination(BKaddress));

            break; //immediat
        }
    }

    return results;
},
    };
}

void RegisterInfinitynodeRPCCommands(CRPCTable &t)
{
// clang-format off
static const CRPCCommand commands[] =
{ //  category              name                      actor (function)                              argNames
  //  --------------------- ------------------------  -----------------------                       ----------
  { "SIN",                  "infinitynode",           &infinitynode,                                {"strCommand", "strFilter", "strOption"} },
  { "SIN",                  "infinitynodeburnfund",   &infinitynodeburnfund,                        {"nodeowneraddress", "amount", "backupaddress"} },
};
// clang-format on
    for (const auto& c : commands) {
        t.appendCommand(c.name, &c);
    }
}