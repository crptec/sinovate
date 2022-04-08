// Copyright (c) 2018-2019 SIN developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <sinovate/infvalidationinterface.h>
#include <sinovate/infinitynodetip.h>
#include <sinovate/infinitynodeman.h>
#include <sinovate/infinitynodepeer.h>
#include <sinovate/infinitynodelockreward.h>
#include <sinovate/infwalletaccess.h>

InfValidationInterface::InfValidationInterface(CConnman& connmanIn, ChainstateManager& chainmanIn): connman(connmanIn), chainman(chainmanIn)
{
    fRegister = true;
}

InfValidationInterface::~InfValidationInterface()
{
    fRegister = false;
}

void InfValidationInterface::UpdatedBlockTip(const CBlockIndex *pindexNew, const CBlockIndex *pindexFork, bool fInitialDownload)
{
    if (pindexNew == pindexFork) // blocks were disconnected without any new ones
        return;

    if (fInitialDownload)
        return;

    /*notification for infnodeman when sync is finised*/
    infTip.UpdatedBlockTip(pindexNew, fInitialDownload, connman);
    /*notification update block*/
    infinitynodePeer.UpdatedBlockTip(pindexNew);
    inflockreward.UpdatedBlockTip(pindexNew, connman, chainman);
    infWalletAccess.UpdatedBlockTip(pindexNew);
    /*nCachedBlockHeight of infnodeman is updated in updateFinalList which is called on validation*/
    //infnodeman.UpdatedBlockTip(pindexNew);
}

InfValidationInterface* g_inf_validation_interface = NULL;

