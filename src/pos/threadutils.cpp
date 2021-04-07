// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Copyright (c) 2020-2021 The SINOVATE developers @giaki3003
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://www.opensource.org/licenses/mit-license.php.

#include <pos/threadutils.h>

// for priority on linux
#include <sys/resource.h>

void SetThreadPriority(int nPriority)
{
#ifdef WIN32
    SetThreadPriority(GetCurrentThread(), nPriority);
#else // WIN32
#ifdef PRIO_THREAD
    setpriority(PRIO_THREAD, 0, nPriority);
#else  // PRIO_THREAD
    setpriority(PRIO_PROCESS, 0, nPriority);
#endif // PRIO_THREAD
#endif // WIN32
}