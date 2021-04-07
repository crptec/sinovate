// Copyright (c) 2017-2019 The PIVX developers
// Copyright (c) 2021 The SINOVATE developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef PIVX_BLOCKSIGNATURE_H
#define PIVX_BLOCKSIGNATURE_H

#include "key.h"
#include "primitives/block.h"

#ifdef ENABLE_WALLET
#include "wallet/wallet.h"
#endif

bool SignBlockWithKey(CBlock& block, const CKey& key);
bool SignBlock(CBlock& block, CWallet* pwallet);
bool CheckBlockSignature(const CBlock& block);

#endif //PIVX_BLOCKSIGNATURE_H