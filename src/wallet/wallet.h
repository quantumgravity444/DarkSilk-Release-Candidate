// Copyright (c) 2009-2016 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Developers
// Copyright (c) 2015-2016 Silk Network
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef DARKSILK_WALLET_H
#define DARKSILK_WALLET_H

#include <string>
#include <vector>
#include <stdlib.h>

#include "wallet/walletdb.h"
#include "wallet/wallet_ismine.h"
#include "primitives/block.h"
#include "primitives/transaction.h"
#include "crypter.h"
#include "main.h"
#include "key.h"
#include "keystore.h"
#include "script/script.h"
#include "ui_interface.h"
#include "util.h"
#include "utilstrencodings.h"
#include "anon/stealth/stealth.h"

extern CWallet* pwalletMain;

// Settings
extern CAmount nTransactionFee;
extern CAmount nReserveBalance;
extern CAmount nMinimumInputValue;
extern bool fWalletUnlockStakingOnly;
extern bool fConfChange;

const CAmount MIN_TX_FEE = 10000; // 0.00001 DRKSLK Minimum Transaction Fee
// Minimum Transaction Fee of 0.00001 DRKSLK, Fees smaller than this are considered zero fee (for transaction creation)
static const double MIN_FEE = 0.00001;
/// Fees smaller than this (in satoshi) are considered zero fee (for relaying)
const CAmount MIN_RELAY_TX_FEE = MIN_TX_FEE;
//! -keypool default
static const unsigned int DEFAULT_KEYPOOL_SIZE = 1000;

extern const char * DEFAULT_WALLET_DAT;

class CAccountingEntry;
class CCoinControl;
class CWalletTx;
class CReserveKey;
class COutput;
class CWalletDB;

typedef std::map<CKeyID, CStealthKeyMetadata> StealthKeyMetaMap;
typedef std::map<std::string, std::string> mapValue_t;

/** (client) version numbers for particular wallet features */
enum WalletFeature
{
    FEATURE_BASE = 10500, // the earliest version new wallets supports (only useful for getinfo's clientversion output)
    
    FEATURE_WALLETCRYPT = 40000, // wallet encryption
    
    FEATURE_COMPRPUBKEY = 60800, // compressed public keys
    FEATURE_LATEST = 60800
};

enum AvailableCoinsType
{
    ALL_COINS = 1,
    ONLY_DENOMINATED = 2,
    ONLY_NONDENOMINATED = 3,
    ONLY_NONDENOMINATED_NOTSN = 4 // ONLY_NONDENOMINATED and not 1000 DRKSLK at the same time
};

/** A key pool entry */
class CKeyPool
{
public:
    int64_t nTime;
    CPubKey vchPubKey;

    CKeyPool()
    {
        nTime = GetTime();
    }

    CKeyPool(const CPubKey& vchPubKeyIn)
    {
        nTime = GetTime();
        vchPubKey = vchPubKeyIn;
    }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        if (!(nType & SER_GETHASH))
            READWRITE(nVersion);
            READWRITE(nTime);
            READWRITE(vchPubKey);
    }
};

/** A CWallet is an extension of a keystore, which also maintains a set of transactions and balances,
 * and provides the ability to create new transactions.
 */
class CWallet : public CCryptoKeyStore, public CWalletInterface
{
private:
    bool SelectCoinsForStaking(const CAmount& nTargetValue, unsigned int nSpendTime, std::set<std::pair<const CWalletTx*,unsigned int> >& setCoinsRet, CAmount& nValueRet) const;
    bool SelectCoins(const CAmount& nTargetValue, unsigned int nSpendTime, set<pair<const CWalletTx*,unsigned int> >& setCoinsRet, CAmount& nValueRet, const CCoinControl* coinControl, AvailableCoinsType coin_type, bool useIX) const;
    CWalletDB *pwalletdbEncryption;

    // the current wallet version: clients below this version are not able to load the wallet
    int nWalletVersion;

    // the maximum wallet format version: memory-only variable that specifies to what version this wallet may be upgraded
    int nWalletMaxVersion;

    // Used to keep track of spent outpoints, and
    // detect and report conflicts (double-spends or
    // mutated transactions where the mutant gets mined).
    typedef std::multimap<COutPoint, uint256> TxSpends;
    TxSpends mapTxSpends;
    void AddToSpends(const COutPoint& outpoint, const uint256& wtxid);
    void AddToSpends(const uint256& wtxid);

    void SyncMetaData(std::pair<TxSpends::iterator, TxSpends::iterator>);

    int GetRealInputSandstormRounds(CTxIn in, int rounds) const;

public:
    /// Main wallet lock.
    /// This lock protects all the fields added by CWallet
    ///   except for:
    ///      fFileBacked (immutable after instantiation)
    ///      strWalletFile (immutable after instantiation)
    mutable CCriticalSection cs_wallet;

    bool SelectCoinsDark(CAmount nValueMin, CAmount nValueMax, std::vector<CTxIn>& setCoinsRet, CAmount& nValueRet, int nSandstormRoundsMin, int nSandstormRoundsMax) const;
    bool SelectCoinsByDenominations(int nDenom, CAmount nValueMin, CAmount nValueMax, std::vector<CTxIn>& vCoinsRet, std::vector<COutput>& vCoinsRet2, CAmount& nValueRet, int nSandstormRoundsMin, int nSandstormRoundsMax);
    bool SelectCoinsDarkDenominated(CAmount nTargetValue, std::vector<CTxIn>& setCoinsRet, CAmount& nValueRet) const;
    bool SelectCoinsStormnode(CTxIn& vin, CAmount& nValueRet, CScript& pubScript) const;
    bool HasCollateralInputs() const;
    bool IsCollateralAmount(CAmount nInputAmount) const;
    int  CountInputsWithAmount(CAmount nInputAmount);

    const CWalletTx* GetWalletTx(const uint256& hash) const;

    bool SelectCoinsCollateral(std::vector<CTxIn>& setCoinsRet, CAmount& nValueRet) const ;
    bool SelectCoinsWithoutDenomination(const CAmount& nTargetValue, std::set<std::pair<const CWalletTx*,unsigned int> >& setCoinsRet, CAmount& nValueRet) const;
    bool GetTransaction(const uint256 &hashTx, CWalletTx& wtx);

    bool fFileBacked;
    bool fWalletUnlockAnonymizeOnly;
    std::string strWalletFile;

    std::set<int64_t> setKeyPool;
    std::map<CKeyID, CKeyMetadata> mapKeyMetadata;

    std::set<CStealthAddress> stealthAddresses;
    StealthKeyMetaMap mapStealthKeyMeta;
        
    int nLastFilteredHeight;
    
    uint32_t nStealth, nFoundStealth; // for reporting, zero before use

    typedef std::map<unsigned int, CMasterKey> MasterKeyMap;
    MasterKeyMap mapMasterKeys;
    unsigned int nMasterKeyMaxID;

    int GetInputSandstormRounds(CTxIn in) const;

    CWallet()
    {
        SetNull();
    }
    CWallet(std::string strWalletFileIn)
    {
        SetNull();

        strWalletFile = strWalletFileIn;
        fFileBacked = true;
    }
    
    void SetNull()
    {
        nWalletVersion = FEATURE_BASE;
        nWalletMaxVersion = FEATURE_BASE;
        fFileBacked = false;
        nMasterKeyMaxID = 0;
        pwalletdbEncryption = NULL;
        nOrderPosNext = 0;
        nTimeFirstKey = 0;
        nLastFilteredHeight = 0;
        fWalletUnlockAnonymizeOnly = false;
    }

    std::map<uint256, CWalletTx> mapWallet;
    int64_t nOrderPosNext;
    std::map<uint256, int> mapRequestCount;

    std::map<CTxDestination, std::string> mapAddressBook;

    CPubKey vchDefaultKey;

    std::set<COutPoint> setLockedCoins;

    int64_t nTimeFirstKey;

    // check whether we are allowed to upgrade (or already support) to the named feature
    bool CanSupportFeature(enum WalletFeature wf) { AssertLockHeld(cs_wallet); return nWalletMaxVersion >= wf; }

    void AvailableCoinsForStaking(std::vector<COutput>& vCoins, unsigned int nSpendTime) const;
    void AvailableCoins(std::vector<COutput>& vCoins, bool fOnlyConfirmed=true, const CCoinControl *coinControl = NULL, AvailableCoinsType coin_type=ALL_COINS, bool useIX = false) const;    
    bool SelectCoinsMinConf(const CAmount& nTargetValue, unsigned int nSpendTime, int nConfMine, int nConfTheirs, vector<COutput> vCoins, set<pair<const CWalletTx*,unsigned int> >& setCoinsRet, CAmount& nValueRet) const;
    bool SelectCoinsMinConfByCoinAge(const CAmount& nTargetValue, unsigned int nSpendTime, int nConfMine, int nConfTheirs, vector<COutput> vCoins, set<pair<const CWalletTx*,unsigned int> >& setCoinsRet, CAmount& nValueRet) const;

    bool IsSpent(const uint256& hash, unsigned int n) const;

    bool IsLockedCoin(uint256 hash, unsigned int n) const;
    void LockCoin(COutPoint& output);
    void UnlockCoin(COutPoint& output);
    void UnlockAllCoins();
    void ListLockedCoins(std::vector<COutPoint>& vOutpts);
    CAmount GetTotalValue(std::vector<CTxIn> vCoins);

    // keystore implementation
    // Generate a new key
    CPubKey GenerateNewKey();
    // Adds a key to the store, and saves it to disk.
    bool AddKeyPubKey(const CKey& key, const CPubKey &pubkey);
    // Adds a key to the store, without saving it to disk (used by LoadWallet)
    bool LoadKey(const CKey& key, const CPubKey &pubkey) { return CCryptoKeyStore::AddKeyPubKey(key, pubkey); }
    // Load metadata (used by LoadWallet)
    bool LoadKeyMetadata(const CPubKey &pubkey, const CKeyMetadata &metadata);

    bool LoadMinVersion(int nVersion) { AssertLockHeld(cs_wallet); nWalletVersion = nVersion; nWalletMaxVersion = std::max(nWalletMaxVersion, nVersion); return true; }

    // Adds an encrypted key to the store, and saves it to disk.
    bool AddCryptedKey(const CPubKey &vchPubKey, const std::vector<unsigned char> &vchCryptedSecret);
    // Adds an encrypted key to the store, without saving it to disk (used by LoadWallet)
    bool LoadCryptedKey(const CPubKey &vchPubKey, const std::vector<unsigned char> &vchCryptedSecret);
    bool AddCScript(const CScript& redeemScript);
    bool LoadCScript(const CScript& redeemScript);

    // Adds a watch-only address to the store, and saves it to disk.
    bool AddWatchOnly(const CScript &dest);
    // Removes a watch-only address from the store.
    bool RemoveWatchOnly(const CScript &dest);
    // Adds a watch-only address to the store, without saving it to disk (used by LoadWallet)
    bool LoadWatchOnly(const CScript &dest);

    bool Lock();
    bool Unlock(const SecureString& strWalletPassphrase, bool anonimizeOnly = false);
    bool ChangeWalletPassphrase(const SecureString& strOldWalletPassphrase, const SecureString& strNewWalletPassphrase);
    bool EncryptWallet(const SecureString& strWalletPassphrase);

    void GetKeyBirthTimes(std::map<CKeyID, int64_t> &mapKeyBirth) const;


    /** Increment the next transaction order id
        @return next transaction order id
     */
    int64_t IncOrderPosNext(CWalletDB *pwalletdb = NULL);

    typedef std::pair<CWalletTx*, CAccountingEntry*> TxPair;
    typedef std::multimap<int64_t, TxPair > TxItems;

    /** Get the wallet's activity log
        @return multimap of ordered transactions and accounting entries
        @warning Returned pointers are *only* valid within the scope of passed acentries
     */
    TxItems OrderedTxItems(std::list<CAccountingEntry>& acentries, std::string strAccount = "");

    void MarkDirty();
    bool AddToWallet(const CWalletTx& wtxIn, bool fFromLoadWallet=false);
    void SyncTransaction(const CTransaction& tx, const CBlock* pblock, bool fConnect = true);
    bool AddToWalletIfInvolvingMe(const CTransaction& tx, const CBlock* pblock, bool fUpdate);
    void EraseFromWallet(const uint256 &hash);
    int ScanForWalletTransactions(CBlockIndex* pindexStart, bool fUpdate = false);
    void ReacceptWalletTransactions();
    void ResendWalletTransactions(bool fForce = false);
    CAmount GetBalance() const;
    CAmount GetUnconfirmedBalance() const;
    CAmount GetImmatureBalance() const;
    CAmount GetStake() const;
    CAmount GetNewMint() const;
 
    CAmount GetAnonymizableBalance(bool includeAlreadyAnonymized=false) const; 
    CAmount GetAnonymizedBalance() const;
    CAmount GetWatchOnlyBalance() const;
    CAmount GetWatchOnlyStake() const;
    CAmount GetUnconfirmedWatchOnlyBalance() const;
    CAmount GetImmatureWatchOnlyBalance() const;
    double GetAverageAnonymizedRounds() const;
    CAmount GetNormalizedAnonymizedBalance() const;
    CAmount GetDenominatedBalance(bool onlyDenom=true, bool onlyUnconfirmed=false, bool includeAlreadyAnonymized = true) const; 
 
    bool CreateTransaction(const std::vector<std::pair<CScript, CAmount> >& vecSend,
                           CWalletTx& wtxNew, CReserveKey& reservekey, CAmount& nFeeRet, int32_t& nChangePos, std::string& strFailReason, const CCoinControl *coinControl = NULL, AvailableCoinsType coin_type=ALL_COINS, bool useIX=false, CAmount nFeePay=0);
    bool CreateTransaction(CScript scriptPubKey, const CAmount& nValue, std::string& sNarr,
                           CWalletTx& wtxNew, CReserveKey& reservekey, CAmount& nFeeRet, const CCoinControl *coinControl = NULL, AvailableCoinsType coin_type=ALL_COINS, bool useIX=false, CAmount nFeePay=0);

    bool CommitTransaction(CWalletTx& wtxNew, CReserveKey& reservekey, std::string strCommand="tx");

    uint64_t GetStakeWeight() const;
    uint64_t GetStakeWeight2() const;
    bool CreateCoinStake(const CKeyStore& keystore, unsigned int nBits, int64_t nSearchInterval, CAmount nFees, CTransaction& txNew, CKey& key);

    std::string SendMoney(CScript scriptPubKey, CAmount nValue, std::string& sNarr, CWalletTx& wtxNew, bool fAskFee=false);
    std::string SendMoneyToDestination(const CTxDestination &address, CAmount nValue, std::string& sNarr, CWalletTx& wtxNew, bool fAskFee=false);

    bool NewStealthAddress(std::string& sError, std::string& sLabel, CStealthAddress& sxAddr);
    bool AddStealthAddress(CStealthAddress& sxAddr);
    bool UnlockStealthAddresses(const CKeyingMaterial& vMasterKeyIn);
    bool UpdateStealthAddress(std::string &addr, std::string &label, bool addIfNotExist);
    
    bool CreateStealthTransaction(CScript scriptPubKey, CAmount nValue, std::vector<uint8_t>& P, std::vector<uint8_t>& narr, std::string& sNarr, CWalletTx& wtxNew, CReserveKey& reservekey, CAmount& nFeeRet, const CCoinControl* coinControl=NULL);
    std::string SendStealthMoney(CScript scriptPubKey, CAmount nValue, std::vector<uint8_t>& P, std::vector<uint8_t>& narr, std::string& sNarr, CWalletTx& wtxNew, bool fAskFee=false);
    bool SendStealthMoneyToDestination(CStealthAddress& sxAddress, CAmount nValue, std::string& sNarr, CWalletTx& wtxNew, std::string& sError, bool fAskFee=false);
    bool FindStealthTransactions(const CTransaction& tx, mapValue_t& mapNarr);

    std::string PrepareSandstormDenominate(int minRounds, int maxRounds);
    int GenerateSandstormOutputs(int nTotalValue, std::vector<CTxOut>& vout);
    bool CreateCollateralTransaction(CMutableTransaction &txCollateral, std::string strReason);
    bool GetBudgetSystemCollateralTX(CTransaction& tx, uint256 hash, bool useIX);
    bool GetBudgetSystemCollateralTX(CWalletTx& tx, uint256 hash, bool useIX);
    bool ConvertList(std::vector<CTxIn> vCoins, std::vector<CAmount>& vecAmounts);

    bool NewKeyPool();
    bool TopUpKeyPool(unsigned int nSize = 0);
    int64_t AddReserveKey(const CKeyPool& keypool);
    void ReserveKeyFromKeyPool(int64_t& nIndex, CKeyPool& keypool);
    void KeepKey(int64_t nIndex);
    void ReturnKey(int64_t nIndex);
    bool GetKeyFromPool(CPubKey &key);
    int64_t GetOldestKeyPoolTime();
    void GetAllReserveKeys(std::set<CKeyID>& setAddress) const;

    std::set< std::set<CTxDestination> > GetAddressGroupings();
    std::map<CTxDestination, CAmount> GetAddressBalances();


    bool IsDenominated(const CTxIn &txin) const;
    bool IsDenominated(const CTransaction& tx) const;

    bool IsDenominatedAmount(CAmount nInputAmount) const;


    isminetype IsMine(const CTxIn& txin) const;
    CAmount GetDebit(const CTxIn& txin, const isminefilter& filter) const;
    isminetype IsMine(const CTxOut& txout) const;
    CAmount GetCredit(const CTxOut& txout, const isminefilter& filter) const;
    bool IsChange(const CTxOut& txout) const;
    CAmount GetChange(const CTxOut& txout) const;
    bool IsMine(const CTransaction& tx) const;
    /** should probably be renamed to IsRelevantToMe */
    bool IsFromMe(const CTransaction& tx) const;
    CAmount GetDebit(const CTransaction& tx, const isminefilter& filter) const;
    CAmount GetCredit(const CTransaction& tx, const isminefilter& filter) const;
    CAmount GetChange(const CTransaction& tx) const; 

    void SetBestChain(const CBlockLocator& loc);

    DBErrors LoadWallet(bool& fFirstRunRet);

    bool SetAddressBookName(const CTxDestination& address, const std::string& strName);

    bool DelAddressBookName(const CTxDestination& address);

    bool UpdatedTransaction(const uint256 &hashTx);

    void Inventory(const uint256 &hash)
    {
        {
            LOCK(cs_wallet);
            std::map<uint256, int>::iterator mi = mapRequestCount.find(hash);
            if (mi != mapRequestCount.end())
                (*mi).second++;
        }
    }

    unsigned int GetKeyPoolSize()
    {
        AssertLockHeld(cs_wallet); // setKeyPool
        return setKeyPool.size();
    }

    bool SetDefaultKey(const CPubKey &vchPubKey);

    // signify that a particular wallet feature is now used. this may change nWalletVersion and nWalletMaxVersion if those are lower
    bool SetMinVersion(enum WalletFeature, CWalletDB* pwalletdbIn = NULL, bool fExplicit = false);

    // change which version we're allowed to upgrade to (note that this does not immediately imply upgrading to that format)
    bool SetMaxVersion(int nVersion);

    // get the current wallet format (the oldest client version guaranteed to understand this wallet)
    int GetVersion() { LOCK(cs_wallet); return nWalletVersion; }

    // Get wallet transactions that conflict with given transaction (spend same outputs)
    std::set<uint256> GetConflicts(const uint256& txid) const;

    //! Verify the wallet database and perform salvage if required
    static bool Verify();

    void FixSpentCoins(int& nMismatchSpent, CAmount& nBalanceInQuestion, bool fCheckOnly = false);
    void DisableTransaction(const CTransaction &tx);

    /** Address book entry changed.
     * @note called with lock cs_wallet held.
     */
    boost::signals2::signal<void (CWallet *wallet, const CTxDestination
        &address, const std::string &label, bool isMine,
        ChangeType status)> NotifyAddressBookChanged;

    /** Wallet transaction added, removed or updated.
     * @note called with lock cs_wallet held.
     */
    boost::signals2::signal<void (CWallet *wallet, const uint256 &hashTx, ChangeType status)> NotifyTransactionChanged;

    /** Show progress e.g. for rescan */
    boost::signals2::signal<void (const std::string &title, int nProgress)> ShowProgress;

    /** Watch-only address added */
    boost::signals2::signal<void (bool fHaveWatchOnly)> NotifyWatchonlyChanged;
};

/** A key allocated from the key pool. */
class CReserveKey
{
protected:
    CWallet* pwallet;
    int64_t nIndex;
    CPubKey vchPubKey;
public:
    CReserveKey(CWallet* pwalletIn)
    {
        nIndex = -1;
        pwallet = pwalletIn;
    }

    ~CReserveKey()
    {
        ReturnKey();
    }

    void ReturnKey();
    bool GetReservedKey(CPubKey &pubkey);
    void KeepKey();
};


typedef std::map<std::string, std::string> mapValue_t;


static void ReadOrderPos(int64_t& nOrderPos, mapValue_t& mapValue)
{
    if (!mapValue.count("n"))
    {
        nOrderPos = -1; // TODO: calculate elsewhere
        return;
    }
    nOrderPos = atoi64(mapValue["n"].c_str());
}


static void WriteOrderPos(const int64_t& nOrderPos, mapValue_t& mapValue)
{
    if (nOrderPos == -1)
        return;
    mapValue["n"] = i64tostr(nOrderPos);
}

/** A transaction with a merkle branch linking it to the block chain. */
class CMerkleTx : public CTransaction
{
private:
    int GetDepthInMainChainINTERNAL(CBlockIndex* &pindexRet) const;
public:
    uint256 hashBlock;
    std::vector<uint256> vMerkleBranch;
    int nIndex;

    // memory only
    mutable bool fMerkleVerified;


    CMerkleTx()
    {
        Init();
    }

    CMerkleTx(const CTransaction& txIn) : CTransaction(txIn)
    {
        Init();
    }

    void Init()
    {
        hashBlock = 0;
        nIndex = -1;
        fMerkleVerified = false;
    }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        //TODO (Amir): Review translation below.
        //nSerSize += SerReadWrite(s, *(CTransaction*)this, nType, nVersion, ser_action);
        READWRITE(*(CTransaction*)this);
        nVersion = this->nVersion;
        READWRITE(hashBlock);
        READWRITE(vMerkleBranch);
        READWRITE(nIndex);
    }

    int SetMerkleBranch(const CBlock* pblock=NULL);

    // Return depth of transaction in blockchain:
    // -1  : not in blockchain, and not in memory pool (conflicted transaction)
    //  0  : in memory pool, waiting to be included in a block
    // >=1 : this many blocks deep in the main chain
    int GetDepthInMainChain(CBlockIndex* &pindexRet, bool enableIX=true) const;
    int GetDepthInMainChain(bool enableIX=true) const { CBlockIndex *pindexRet; return GetDepthInMainChain(pindexRet, enableIX); }
    bool IsInMainChain() const { CBlockIndex *pindexRet; return GetDepthInMainChainINTERNAL(pindexRet) > 0; }
    int GetBlocksToMaturity() const;
    bool AcceptToMemoryPool(bool fLimitFree=true, bool fRejectInsaneFee=true, bool ignoreFees=false);
    int GetTransactionLockSignatures() const;
    bool IsTransactionLockTimedOut() const;
    
};

/** A transaction with a bunch of additional info that only the owner cares about.
 * It includes any unrecorded transactions needed to link it back to the block chain.
 */
class CWalletTx : public CMerkleTx
{
private:
    const CWallet* pwallet;

public:
    std::vector<CMerkleTx> vtxPrev;
    mapValue_t mapValue;
    std::vector<std::pair<std::string, std::string> > vOrderForm;
    unsigned int fTimeReceivedIsTxTime;
    unsigned int nTimeReceived;  // time received by this node
    unsigned int nTimeSmart;
    char fFromMe;
    std::string strFromAccount;
    std::vector<char> vfSpent; // which outputs are already spent
    int64_t nOrderPos;  // position in ordered transaction list

    // memory only
    mutable bool fDebitCached;
    mutable bool fCreditCached;
    mutable bool fImmatureCreditCached;
    mutable bool fDenomUnconfCreditCached;
    mutable bool fDenomConfCreditCached;
    mutable bool fAnonymizedCreditCached;
    mutable bool fAnonymizableCreditCached;
    mutable bool fAvailableWatchCreditCached;
    mutable bool fImmatureWatchCreditCached;
    mutable bool fAvailableCreditCached;
    mutable bool fChangeCached;
    mutable bool fWatchDebitCached;
    mutable bool fWatchCreditCached;

    mutable CAmount nWatchDebitCached;
    mutable CAmount nWatchCreditCached;
    mutable CAmount nImmatureCreditCached;
    mutable CAmount nDebitCached;
    mutable CAmount nCreditCached;
    mutable CAmount nAvailableCreditCached;
    mutable CAmount nChangeCached;
    mutable CAmount nImmatureWatchCreditCached;
    mutable CAmount nAvailableWatchCreditCached;
    mutable CAmount nAnonymizableCreditCached;
    mutable CAmount nAnonymizedCreditCached;
    mutable CAmount nDenomUnconfCreditCached;
    mutable CAmount nDenomConfCreditCached;

    CWalletTx()
    {
        Init(NULL);
    }

    CWalletTx(const CWallet* pwalletIn)
    {
        Init(pwalletIn);
    }

    CWalletTx(const CWallet* pwalletIn, const CMerkleTx& txIn) : CMerkleTx(txIn)
    {
        Init(pwalletIn);
    }

    CWalletTx(const CWallet* pwalletIn, const CTransaction& txIn) : CMerkleTx(txIn)
    {
        Init(pwalletIn);
    }

    void Init(const CWallet* pwalletIn)
    {
        pwallet = pwalletIn;
        vtxPrev.clear();
        mapValue.clear();
        vOrderForm.clear();
        fTimeReceivedIsTxTime = false;
        nTimeReceived = 0;
        nTimeSmart = 0;
        fFromMe = false;
        strFromAccount.clear();
        vfSpent.clear();
        fDebitCached = false;
        fCreditCached = false;
        fImmatureCreditCached = false;
        fAvailableCreditCached = false;
        fAnonymizableCreditCached = false;
        fAnonymizedCreditCached = false;
        fDenomUnconfCreditCached = false;
        fDenomConfCreditCached = false;
        fWatchDebitCached = false;
        fWatchCreditCached = false;
        fImmatureWatchCreditCached = false;
        fAvailableWatchCreditCached = false;
        fChangeCached = false;
        nDebitCached = 0;
        nCreditCached = 0;
        nAvailableCreditCached = 0;
        nAnonymizableCreditCached = 0;
        nAnonymizedCreditCached = 0;
        nDenomUnconfCreditCached = 0;
        nDenomConfCreditCached = 0;
        nWatchDebitCached = 0;
        nWatchCreditCached = 0;
        nAvailableWatchCreditCached = 0;
        nImmatureWatchCreditCached = 0;
        nChangeCached = 0;
        nOrderPos = -1;
    }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        CWalletTx* pthis = const_cast<CWalletTx*>(this);
        if (ser_action.ForRead())
            pthis->Init(NULL);
        char fSpent = false;

        if (!ser_action.ForRead())
        {
            pthis->mapValue["fromaccount"] = pthis->strFromAccount;

            std::string str;
            BOOST_FOREACH(char f, vfSpent)
            {
                str += (f ? '1' : '0');
                if (f)
                    fSpent = true;
            }
            pthis->mapValue["spent"] = str;

            WriteOrderPos(pthis->nOrderPos, pthis->mapValue);

            if (nTimeSmart)
                pthis->mapValue["timesmart"] = strprintf("%u", nTimeSmart);
        }

        //TODO (Amir): Review translation of this serialization line:
        //nSerSize += SerReadWrite(s, *(CMerkleTx*)this, nType, nVersion,ser_action);
        READWRITE(*(CMerkleTx*)this);
        READWRITE(vtxPrev);
        READWRITE(mapValue);
        READWRITE(vOrderForm);
        READWRITE(fTimeReceivedIsTxTime);
        READWRITE(nTimeReceived);
        READWRITE(fFromMe);
        READWRITE(fSpent);

        if (ser_action.ForRead())
        {
            pthis->strFromAccount = pthis->mapValue["fromaccount"];

            if (mapValue.count("spent"))
                BOOST_FOREACH(char c, pthis->mapValue["spent"])
                    pthis->vfSpent.push_back(c != '0');
            else
                pthis->vfSpent.assign(vout.size(), fSpent);

            ReadOrderPos(pthis->nOrderPos, pthis->mapValue);

            pthis->nTimeSmart = mapValue.count("timesmart") ? (unsigned int)atoi64(pthis->mapValue["timesmart"]) : 0;
        }

        pthis->mapValue.erase("fromaccount");
        pthis->mapValue.erase("version");
        pthis->mapValue.erase("spent");
        pthis->mapValue.erase("n");
        pthis->mapValue.erase("timesmart");
    }

    // marks certain txout's as spent
    // returns true if any update took place
    bool UpdateSpent(const std::vector<char>& vfNewSpent)
    {
        bool fReturn = false;
        for (unsigned int i = 0; i < vfNewSpent.size(); i++)
        {
            if (i == vfSpent.size())
                break;

            if (vfNewSpent[i] && !vfSpent[i])
            {
                vfSpent[i] = true;
                fReturn = true;
                fAvailableCreditCached = false;
            }
        }
        return fReturn;
    }

    // make sure balances are recalculated
    void MarkDirty()
    {
        fCreditCached = false;
        fAvailableCreditCached = false;
        fImmatureCreditCached = false;
        fAnonymizableCreditCached = false;
        fAnonymizableCreditCached = false;
        fDenomUnconfCreditCached = false;
        fDenomConfCreditCached = false;
        fAvailableWatchCreditCached = false;
        fImmatureWatchCreditCached = false;
        fWatchDebitCached = false;
        fWatchCreditCached = false;
        fDebitCached = false;
        fChangeCached = false;
    }

    void BindWallet(CWallet *pwalletIn)
    {
        pwallet = pwalletIn;
        MarkDirty();
    }

    void MarkSpent(unsigned int nOut)
    {
        if (nOut >= vout.size())
            throw std::runtime_error("CWalletTx::MarkSpent() : nOut out of range");
        vfSpent.resize(vout.size());
        if (!vfSpent[nOut])
        {
            vfSpent[nOut] = true;
            fAvailableCreditCached = false;
        }
    }

    void MarkUnspent(unsigned int nOut)
    {
        if (nOut >= vout.size())
            throw std::runtime_error("CWalletTx::MarkUnspent() : nOut out of range");
        vfSpent.resize(vout.size());
        if (vfSpent[nOut])
        {
            vfSpent[nOut] = false;
            fAvailableCreditCached = false;
        }
    }

    bool IsSpent(unsigned int nOut) const
    {
        if (nOut >= vout.size())
            throw std::runtime_error("CWalletTx::IsSpent() : nOut out of range");
        if (nOut >= vfSpent.size())
            return false;
        return (!!vfSpent[nOut]);
    }

    int64_t IsDenominated() const
    {
        if (vin.empty())
            return 0;
        return pwallet->IsDenominated(*this);
    }

    //! filter decides which addresses will count towards the debit
    CAmount GetDebit(const isminefilter& filter) const;
    CAmount GetCredit(const isminefilter& filter) const;
    CAmount GetImmatureCredit(bool fUseCache=true) const;
    CAmount GetAvailableCredit(bool fUseCache=true) const;
    CAmount GetAnonymizableCredit(const bool& fUseCache=true) const;
    CAmount GetDenominatedCredit(const bool& unconfirmed=false, const bool& fUseCache=false) const;
    CAmount GetImmatureWatchOnlyCredit(const bool& fUseCache=true) const;
    CAmount GetAvailableWatchOnlyCredit(const bool& fUseCache=true) const;
    CAmount GetChange() const;

    void GetAmounts(std::list<std::pair<CTxDestination, CAmount> >& listReceived,
                    std::list<std::pair<CTxDestination, CAmount> >& listSent, CAmount& nFee, std::string& strSentAccount, const isminefilter& filter) const;

    void GetAccountAmounts(const std::string& strAccount, CAmount& nReceived,
                           CAmount& nSent, CAmount& nFee, const isminefilter& filter) const;

    bool IsFromMe(const isminefilter& filter) const
    {
        return (GetDebit(filter) > 0);
    }

    bool InMempool() const;
    bool IsTrusted() const;

    bool WriteToDisk();

    int64_t GetTxTime() const;
    int GetRequestCount() const;

    void AddSupportingTransactions(CTxDB& txdb);

    bool AcceptWalletTransaction(CTxDB& txdb);
    bool AcceptWalletTransaction();

    void RelayWalletTransaction(std::string strCommand="tx");

    std::set<uint256> GetConflicts() const;
};




class COutput
{
public:
    const CWalletTx *tx;
    int i;
    int nDepth;
    bool fSpendable;

    COutput(const CWalletTx *txIn, int iIn, int nDepthIn, bool fSpendableIn)
    {
        tx = txIn; i = iIn; nDepth = nDepthIn; fSpendable = fSpendableIn;
    }

    std::string ToString() const
    {
        return strprintf("COutput(%s, %d, %d) [%s]", tx->GetHash().ToString(), i, nDepth, FormatMoney(tx->vout[i].nValue));
    }

    //Used with Sandstorm. Will return largest nondenom, then denominations, then very small inputs
    int Priority() const
    {
        BOOST_FOREACH(int64_t d, sandStormDenominations)
            if(tx->vout[i].nValue == d) return 10000;
        if(tx->vout[i].nValue < 1*COIN) return 20000;

        //nondenom return largest first
        return -(tx->vout[i].nValue/COIN);
    }

    void print() const
    {
        LogPrintf("%s\n", ToString().c_str());
    }
};




/** Private key that includes an expiration date in case it never gets used. */
class CWalletKey
{
public:
    CPrivKey vchPrivKey;
    int64_t nTimeCreated;
    int64_t nTimeExpires;
    std::string strComment;
    //// todo: add something to note what created it (user, getnewaddress, change)
    ////   maybe should have a map<string, string> property map

    CWalletKey(int64_t nExpires=0)
    {
        nTimeCreated = (nExpires ? GetTime() : 0);
        nTimeExpires = nExpires;
    }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        if (!(nType & SER_GETHASH))
            READWRITE(nVersion);
        READWRITE(vchPrivKey);
        READWRITE(nTimeCreated);
        READWRITE(nTimeExpires);
        READWRITE(LIMITED_STRING(strComment, 65536));
    }
};






/** Account information.
 * Stored in wallet with key "acc"+string account name.
 */
class CAccount
{
public:
    CPubKey vchPubKey;

    CAccount()
    {
        SetNull();
    }

    void SetNull()
    {
        vchPubKey = CPubKey();
    }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        if (!(nType & SER_GETHASH))
            READWRITE(nVersion);
        READWRITE(vchPubKey);
    }
};



/** Internal transfers.
 * Database key is acentry<account><counter>.
 */
class CAccountingEntry
{
public:
    std::string strAccount;
    CAmount nCreditDebit;
    int64_t nTime;
    std::string strOtherAccount;
    std::string strComment;
    mapValue_t mapValue;
    int64_t nOrderPos;  // position in ordered transaction list
    uint64_t nEntryNo;

    CAccountingEntry()
    {
        SetNull();
    }

    void SetNull()
    {
        nCreditDebit = 0;
        nTime = 0;
        strAccount.clear();
        strOtherAccount.clear();
        strComment.clear();
        nOrderPos = -1;
    }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        CAccountingEntry& me = *const_cast<CAccountingEntry*>(this);
        if (!(nType & SER_GETHASH))
            READWRITE(nVersion);
        // Note: strAccount is serialized as part of the key, not here.
        READWRITE(nCreditDebit);
        READWRITE(nTime);
        READWRITE(LIMITED_STRING(strComment, 65536));

        if (!ser_action.ForRead())
        {
            WriteOrderPos(nOrderPos, me.mapValue);

            if (!(mapValue.empty() && _ssExtra.empty()))
            {
                CDataStream ss(nType, nVersion);
                ss.insert(ss.begin(), '\0');
                ss << mapValue;
                ss.insert(ss.end(), _ssExtra.begin(), _ssExtra.end());
                me.strComment.append(ss.str());
            }
        }

        READWRITE(LIMITED_STRING(strComment, 65536));

        size_t nSepPos = strComment.find("\0", 0, 1);
        if (ser_action.ForRead())
        {
            me.mapValue.clear();
            if (std::string::npos != nSepPos)
            {
                CDataStream ss(std::vector<char>(strComment.begin() + nSepPos + 1, strComment.end()), nType, nVersion);
                ss >> me.mapValue;
                me._ssExtra = std::vector<char>(ss.begin(), ss.end());
            }
            ReadOrderPos(me.nOrderPos, me.mapValue);
        }
        if (std::string::npos != nSepPos)
            me.strComment.erase(nSepPos);

        me.mapValue.erase("n");
    }
private:
    std::vector<char> _ssExtra;
};

#endif
