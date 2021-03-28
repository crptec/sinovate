// Copyright (c) 2018-2019 SIN developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SIN_INFINITYNODETIP_H
#define SIN_INFINITYNODETIP_H

#include <chain.h>
#include <net.h>

#include <univalue.h>

class CInfinitynodeTip;
extern CInfinitynodeTip infTip;

/*action at when download block from network*/
class CInfinitynodeTip
{
private:
    bool fFinished;
public:
    CInfinitynodeTip();

    bool IsFinished(){return fFinished;};
    void UpdatedBlockTip(const CBlockIndex *pindexNew, bool fInitialDownload, CConnman& connman);
};
#endif // SIN_INFINITYNODETIP_H