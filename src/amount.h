// Copyright (c) 2009-2016 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef DARKSILK_AMOUNT_H
#define DARKSILK_AMOUNT_H

#include "serialize.h"

#include <stdlib.h>
#include <string>
#include <limits>
 
typedef int64_t CAmount;

static const CAmount COIN = 100000000;
static const CAmount CENT = 1000000;

/// No amount larger than this (in DRKSLK) is valid
static const CAmount MAX_MONEY = std::numeric_limits<int64_t>::max();
inline bool MoneyRange(const CAmount& nValue) { return (nValue >= 0 && nValue <= MAX_MONEY); }

/// Type-safe wrapper class to for fee rates
/// (how much to pay based on transaction size)
class CFeeRate
{
private:
    CAmount nSatoshisPerK; // unit is satoshis-per-1,000-bytes
public:
    CFeeRate() : nSatoshisPerK(0) { }
    explicit CFeeRate(const CAmount& _nSatoshisPerK): nSatoshisPerK(_nSatoshisPerK) { }
    CFeeRate(const CAmount& nFeePaid, size_t nSize);
    CFeeRate(const CFeeRate& other) { nSatoshisPerK = other.nSatoshisPerK; }

    CAmount GetFee(size_t size) const; // unit returned is satoshis
    CAmount GetFeePerK() const { return GetFee(1000); } // satoshis-per-1000-bytes

    friend bool operator<(const CFeeRate& a, const CFeeRate& b) { return a.nSatoshisPerK < b.nSatoshisPerK; }
    friend bool operator>(const CFeeRate& a, const CFeeRate& b) { return a.nSatoshisPerK > b.nSatoshisPerK; }
    friend bool operator==(const CFeeRate& a, const CFeeRate& b) { return a.nSatoshisPerK == b.nSatoshisPerK; }
    friend bool operator<=(const CFeeRate& a, const CFeeRate& b) { return a.nSatoshisPerK <= b.nSatoshisPerK; }
    friend bool operator>=(const CFeeRate& a, const CFeeRate& b) { return a.nSatoshisPerK >= b.nSatoshisPerK; }
    std::string ToString() const;

    IMPLEMENT_SERIALIZE
    (
        READWRITE(nSatoshisPerK);
    )
};

#endif //  DARKSILK_AMOUNT_H
