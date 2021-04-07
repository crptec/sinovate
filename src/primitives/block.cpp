// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2019 The Bitcoin Core developers
// Copyright (c) 2015-2020 The SINOVATE developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <primitives/block.h>

#include <hash.h>
#include <tinyformat.h>

// sin macros
#define BEGIN(a)            ((char*)&(a))
#define END(a)              ((char*)&((&(a))[1]))

uint256 CBlockHeader::GetHash() const
{
    /* TODO @giaki3003 we currently use x22i hashes for everything except work,
     * a future version/versionbits fork should be needed to complete the fork
     * and avoid x22i usage. Current workaround is to abstract validation with
     * ::GetValidationHash() which uses historical timestamps for hash switching 
     */
    return HashX22I(BEGIN(nVersion), END(nNonce));
}

uint256 CBlockHeader::GetValidationHash() const
{
    if (nTime < 1559373346) {
        return HashX22I(BEGIN(nVersion), END(nNonce));
    } else {
        return HashX25X(BEGIN(nVersion), END(nNonce));
    }
}

std::string CBlock::ToString() const
{
    std::stringstream s;
    s << strprintf("CBlock(hash=%s, ver=0x%08x, hashPrevBlock=%s, hashMerkleRoot=%s, nTime=%u, nBits=%08x, nNonce=%u, proofType=%s, vtx=%u)\n",
        GetHash().ToString(),
        nVersion,
        hashPrevBlock.ToString(),
        hashMerkleRoot.ToString(),
        nTime, nBits, nNonce,
        IsProofOfStake() ? "PoS" : "PoW",
        vtx.size());
    for (const auto& tx : vtx) {
        s << "  " << tx->ToString() << "\n";
    }
    return s.str();
}
