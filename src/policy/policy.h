// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin developers
// Copyright (c) 2015-2016 Silk Network
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php

#ifndef DARKSILK_POLICY_POLICY_H
#define DARKSILK_POLICY_POLICY_H

#include "consensus/consensus.h"
#include "script/interpreter.h"
#include "script/standard.h"

#include <string>

class CCoinsViewCache;

// Default for -blockprioritysize, maximum space for zero/low-fee transactions
static const unsigned int DEFAULT_BLOCK_PRIORITY_SIZE = 50000;
// The maximum size for transactions we're willing to relay/mine
static const unsigned int MAX_STANDARD_TX_SIZE = MAX_BLOCK_SIZE_GEN/5;
// Maxiumum number of signature check operations in an IsStandard() P2SH script
static const unsigned int MAX_P2SH_SIGOPS = 15;
// The maximum number of sigops we're willing to relay/mine in a single tx
static const unsigned int MAX_TX_SIGOPS = MAX_BLOCK_SIGOPS/5;

/**
 * Standard script verification flags that standard transactions will comply
 * with. However scripts violating these flags may still be present in valid
 * blocks and we must accept those blocks.
 */
static const unsigned int STANDARD_SCRIPT_VERIFY_FLAGS = MANDATORY_SCRIPT_VERIFY_FLAGS |
                                                         SCRIPT_VERIFY_DERSIG |
                                                         SCRIPT_VERIFY_STRICTENC |
                                                         SCRIPT_VERIFY_MINIMALDATA |
                                                         SCRIPT_VERIFY_NULLDUMMY |
                                                         SCRIPT_VERIFY_DISCOURAGE_UPGRADABLE_NOPS |
                                                         SCRIPT_VERIFY_CLEANSTACK |
                                                         SCRIPT_VERIFY_CHECKLOCKTIMEVERIFY |
                                                         SCRIPT_VERIFY_LOW_S;

/** For convenience, standard but not mandatory verify flags. */
static const unsigned int STANDARD_NOT_MANDATORY_VERIFY_FLAGS = STANDARD_SCRIPT_VERIFY_FLAGS & ~MANDATORY_SCRIPT_VERIFY_FLAGS;

/** Used as the flags parameter to CheckFinalTx() in non-consensus code */
static const unsigned int STANDARD_LOCKTIME_VERIFY_FLAGS = LOCKTIME_MEDIAN_TIME_PAST;

bool IsStandard(const CScript& scriptPubKey, txnouttype& whichType);
    /**
     * Check for standard transaction types
     * @return True if all outputs (scriptPubKeys) use only standard transaction forms
     */
bool IsStandardTx(const CTransaction& tx, std::string& reason);
    /**
     * Check for standard transaction types
     * @param[in] mapInputs    Map of previous transactions that have outputs we're spending
     * @return True if all inputs (scriptSigs) use only standard transaction forms
     */
bool AreInputsStandard(const CTransaction& tx, const CCoinsViewCache& mapInputs);

#endif // DARKSILK_POLICY_POLICY_H