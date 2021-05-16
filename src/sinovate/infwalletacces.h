// Copyright (c) 2018-2019 SIN developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SIN_INFWALLETACCES_H
#define SIN_INFWALLETACCES_H

#include <wallet/wallet.h>
#include <wallet/coincontrol.h>

#include <sinovate/infinitynodelockreward.h>

class CInfWalletAcces;

extern CInfWalletAcces infWalletAcces;

class CInfWalletAcces
{
private:
    int nCachedBlockHeight;
    mutable RecursiveMutex cs;
public:
    CInfWalletAcces() : nCachedBlockHeight(0) {}

    std::shared_ptr<CWallet> GetWalletAcces() const;
    bool IsWalletlocked(const CWallet* pwallet);
    bool IsMineNodeAddress(const CWallet* pwallet, CTxDestination dest);
    bool IsBalancePositive(const CWallet* pwallet);
    bool RegisterLROnchain();

    //call in infvalidationinterface.cpp when node connect a new block
    void UpdatedBlockTip(const CBlockIndex *pindex);
};

#endif // SIN_INFWALLETACCES_H
