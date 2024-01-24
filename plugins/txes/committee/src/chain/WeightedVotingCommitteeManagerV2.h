/**
*** Copyright 2024 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/chain/CommitteeManager.h"
#include "src/cache/CommitteeAccountCollector.h"

namespace catapult { namespace config { class CommitteeConfiguration; } }

namespace catapult { namespace chain {

	/// Committee manager that implements weighted voting selection of committee.
	class WeightedVotingCommitteeManagerV2 : public CommitteeManager {
	public:
		/// Creates a weighted voting committee manager around \a pAccountCollector.
		explicit WeightedVotingCommitteeManagerV2(std::shared_ptr<cache::CommitteeAccountCollector> pAccountCollector);

	public:
		const Committee& selectCommittee(const model::NetworkConfiguration& config) override;
		Key getBootKey(const Key& harvestKey, const model::NetworkConfiguration& config) const override;

		void reset() override;

		HarvesterWeight weight(const Key& accountKey, const model::NetworkConfiguration& config) const override;
		HarvesterWeight zeroWeight() const override;
		void add(HarvesterWeight& weight, const chain::HarvesterWeight& delta) const override;
		void mul(HarvesterWeight& weight, double multiplier) const override;
		bool ge(const HarvesterWeight& weight1, const chain::HarvesterWeight& weight2) const override;
		bool eq(const HarvesterWeight& weight1, const chain::HarvesterWeight& weight2) const override;
		std::string str(const HarvesterWeight& weight) const override;

		void logCommittee() const override;

	public:
		cache::AccountMap& accounts() {
			return m_accounts;
		}

	protected:
		void decreaseActivities(const config::CommitteeConfiguration& config);

	private:
		std::shared_ptr<cache::CommitteeAccountCollector> m_pAccountCollector;
		std::map<Key, Hash256> m_hashes;
		cache::AccountMap m_accounts;
		Timestamp m_timestamp;
		uint64_t m_phaseTime;
	};
}}
