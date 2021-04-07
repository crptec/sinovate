// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Copyright (c) 2020-2021 The SINOVATE developers @giaki3003
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or https://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_THREADUTILS_H
#define BITCOIN_THREADUTILS_H

// Priority value macros
#ifndef WIN32
// PRIO_MAX is not defined on Solaris
#ifndef PRIO_MAX
#define PRIO_MAX 20
#endif
#define THREAD_PRIORITY_LOWEST PRIO_MAX
#define THREAD_PRIORITY_BELOW_NORMAL 2
#define THREAD_PRIORITY_NORMAL 0
#define THREAD_PRIORITY_ABOVE_NORMAL (-2)
#endif

/**
 * Set a threads system-wide priority.
 * @param nPriority The priority value to be set as integer (e.g. "2")
 */
void SetThreadPriority(int nPriority);

#endif // BITCOIN_THREADUTILS_H