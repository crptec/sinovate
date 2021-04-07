// Copyright (c) 2017-2020 The PIVX developers
// Copyright (c) 2021 The SINOVATE developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "blocksig.h"

#include "script/standard.h"

typedef std::vector<unsigned char> valtype;

bool SignBlockWithKey(CBlock& block, const CKey& key)
{
    if (!key.Sign(block.GetHash(), block.vchBlockSig))
        return error("%s: failed to sign block hash with key", __func__);

    return true;
}

bool SignBlock(CBlock& block, CWallet* pwallet)
{
    CKeyID keyID;
    if (block.IsProofOfWork()) {
        bool fFoundID = false;
        for (const CTxOut& txout : block.vtx[0]->vout) {
            if (!txout.GetKeyIDFromUTXO(keyID))
                continue;
            fFoundID = true;
            break;
        }
        if (!fFoundID)
            return error("%s: failed to find key for PoW", __func__);
    } else {
        if (!block.vtx[1]->vout[1].GetKeyIDFromUTXO(keyID))
            return error("%s: failed to find key for PoS", __func__);
    }

    CKey key;
    LegacyScriptPubKeyMan* provider = pwallet->GetLegacyScriptPubKeyMan();
    // if P2PKH or P2CS check that we have the input private key
    if (!provider->GetKey(keyID, key)) {
        return error("%s: Unable to get key from keystore", __func__);
    }

    return SignBlockWithKey(block, key);
}

bool CheckBlockSignature(const CBlock& block)
{
    if (block.IsProofOfWork())
        return block.vchBlockSig.empty();

    if (block.vchBlockSig.empty())
        return error("%s: vchBlockSig is empty!", __func__);

    /** Each block is signed by the private key of the input that is staked.
     *  The public key that signs must match the public key associated with the first utxo of the coinstake tx.
     */
    CPubKey pubkey;
    std::vector<valtype> vSolutions;
    const CTxOut& txout = block.vtx[1]->vout[1];
    TxoutType whichType = Solver(txout.scriptPubKey, vSolutions);
    if (whichType == TxoutType::PUBKEYHASH) {
        const CTxIn& txin = block.vtx[1]->vin[0];
        int start = 1 + (int) *txin.scriptSig.begin(); // skip sig
        pubkey = CPubKey(txin.scriptSig.begin()+ start + 1, txin.scriptSig.end());
    }

    if (!pubkey.IsValid())
        return error("%s: invalid pubkey %s", __func__, HexStr(pubkey));

    return pubkey.Verify(block.GetHash(), block.vchBlockSig);
}
