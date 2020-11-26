// Copyright (c) 2018-2019 SIN developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <sinovate/infinitynodetip.h>
#include <sinovate/infinitynodeman.h>

#include <validation.h>

CInfinitynodeTip infTip;

CInfinitynodeTip::CInfinitynodeTip()
: fFinished(false)
{}

/*
 * each block (event when sync from block 0), when reached fReachedBestHeader => update stm and node rank
 */
void CInfinitynodeTip::UpdatedBlockTip(const CBlockIndex *pindexNew, bool fInitialDownload, CConnman& connman)
{
    static bool fReachedBestHeader = false;
    bool fReachedBestHeaderNew = pindexNew->GetBlockHash() == pindexBestHeader->GetBlockHash();

    if (fReachedBestHeader && !fReachedBestHeaderNew) {
        fReachedBestHeader = false;
        return;
    }

    fReachedBestHeader = fReachedBestHeaderNew;

    LogPrintf("CInfinitynodeTip::UpdatedBlockTip -- pindexNew->nHeight: %d pindexBestHeader->nHeight: %d fInitialDownload=%d fReachedBestHeader=%d\n",
                pindexNew->nHeight, pindexBestHeader->nHeight, fInitialDownload, fReachedBestHeader);

    if (fReachedBestHeader) {
        infnodeman.setSyncStatus(true);
        // We must be at the tip already
        fFinished = true;
    }
}
