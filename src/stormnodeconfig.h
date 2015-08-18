// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SRC_STORMNODECONFIG_H_
#define SRC_STORMNODECONFIG_H_

#include <string>
#include <vector>

#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>

class CStormnodeConfig;
extern CStormnodeConfig stormnodeConfig;

class CStormnodeConfig
{

public:
	class CStormnodeEntry {

	private:
		std::string alias;
		std::string ip;
		std::string privKey;
		std::string txHash;
		std::string outputIndex;

	public:

		CStormnodeEntry(std::string alias, std::string ip, std::string privKey, std::string txHash, std::string outputIndex) {
			this->alias = alias;
			this->ip = ip;
			this->privKey = privKey;
			this->txHash = txHash;
			this->outputIndex = outputIndex;
		}

		const std::string& getAlias() const {
			return alias;
		}

		void setAlias(const std::string& alias) {
			this->alias = alias;
		}

		const std::string& getOutputIndex() const {
			return outputIndex;
		}

		void setOutputIndex(const std::string& outputIndex) {
			this->outputIndex = outputIndex;
		}

		const std::string& getPrivKey() const {
			return privKey;
		}

		void setPrivKey(const std::string& privKey) {
			this->privKey = privKey;
		}

		const std::string& getTxHash() const {
			return txHash;
		}

		void setTxHash(const std::string& txHash) {
			this->txHash = txHash;
		}

		const std::string& getIp() const {
			return ip;
		}

		void setIp(const std::string& ip) {
			this->ip = ip;
		}
	};

	CStormnodeConfig() {
		entries = std::vector<CStormnodeEntry>();
	}

	void clear();
    bool read(std::string& strErr);
	void add(std::string alias, std::string ip, std::string privKey, std::string txHash, std::string outputIndex);

	std::vector<CStormnodeEntry>& getEntries() {
		return entries;
	}

private:
	std::vector<CStormnodeEntry> entries;


};


#endif /* SRC_STORMNODECONFIG_H_ */

