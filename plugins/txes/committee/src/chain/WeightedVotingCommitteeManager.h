/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/chain/CommitteeManager.h"
#include "src/cache/CommitteeAccountCollector.h"

namespace catapult { namespace chain {

	class Hasher {
	public:
		virtual ~Hasher(){}
	public:
		virtual Hash256& calculateHash(Hash256& hash, const GenerationHash& generationHash, const Key& key) const = 0;
		virtual Hash256& calculateHash(Hash256& hash) const = 0;
		virtual Hash256 calculateHash(double rate, const Key& key) const = 0;
	};

	/// Committee manager that implements weighted voting selection of committee.
	class WeightedVotingCommitteeManager : public CommitteeManager {
	public:
		/// Creates a weighted voting committee manager around \a pAccountCollector.
		explicit WeightedVotingCommitteeManager(const std::shared_ptr<cache::CommitteeAccountCollector>& pAccountCollector);

	public:
		const Committee& selectCommittee(const model::NetworkConfiguration& config) override;

		void reset() override;

		double weight(const Key& accountKey) const override;

	public:
		const std::shared_ptr<cache::CommitteeAccountCollector>& accountCollector() {
			return m_pAccountCollector;
		}

	protected:
		std::unique_ptr<Hasher> m_pHasher;

	private:
		std::shared_ptr<cache::CommitteeAccountCollector> m_pAccountCollector;
		std::map<Key, Hash256> m_hashes;
	};
}}
