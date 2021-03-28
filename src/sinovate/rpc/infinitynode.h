// Copyright (c) 2018-2019 SIN developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_RPC_INFINITYNODE_H
#define BITCOIN_RPC_INFINITYNODE_H

#include <init.h>
#include <netbase.h>
#include <key_io.h>
#include <core_io.h>
#include <validation.h>
#include <node/context.h>
#include <sinovate/infinitynodeman.h>
#include <sinovate/infinitynodersv.h>
#include <sinovate/infinitynodemeta.h>
#include <sinovate/infinitynodepeer.h>
#include <sinovate/infinitynodelockreward.h>
#include <rpc/util.h>
#include <rpc/blockchain.h>
#include <util/moneystr.h>
#include <util/translation.h>

#ifdef ENABLE_WALLET
#include <wallet/coincontrol.h>
#include <wallet/context.h>
#include <wallet/feebumper.h>
#include <wallet/load.h>
#include <wallet/wallet.h>
#include <wallet/walletdb.h>
#include <wallet/walletutil.h>
#include <wallet/rpcwallet.h>
#endif

#include <consensus/validation.h>

#include <secp256k1.h>
#include <secp256k1_schnorr.h>
#include <secp256k1_musigpk.h>

#include <fstream>
#include <iomanip>
class UniValue;
#endif