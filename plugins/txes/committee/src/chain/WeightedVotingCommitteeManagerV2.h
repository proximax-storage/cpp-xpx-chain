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
		void selectCommittee(const model::NetworkConfiguration& config, const BlockchainVersion& blockchainVersion) override;
		Key getBootKey(const Key& harvestKey, const model::NetworkConfiguration& config) const override;

		void reset() override;

		Committee committee() const override;

		HarvesterWeight weight(const Key& accountKey, const model::NetworkConfiguration& config) const override;
		HarvesterWeight zeroWeight() const override;
		void add(HarvesterWeight& weight, const chain::HarvesterWeight& delta) const override;
		void mul(HarvesterWeight& weight, double multiplier) const override;
		bool ge(const HarvesterWeight& weight1, const chain::HarvesterWeight& weight2) const override;
		bool eq(const HarvesterWeight& weight1, const chain::HarvesterWeight& weight2) const override;
		std::string str(const HarvesterWeight& weight) const override;

		void logCommittee() const override;

		cache::AccountMap accounts();

	protected:
		std::multimap<int64_t, Key, std::greater<>> getCandidates(
			const model::NetworkConfiguration& networkConfig,
			const config::CommitteeConfiguration& config,
			const BlockchainVersion& blockchainVersion);
		void decreaseActivities(const config::CommitteeConfiguration& config);
		void logHarvesters() const;

		static void logProcess(const char* prefix, const Key& process, const Timestamp& timestamp, std::ostringstream& out);
		static int64_t calculateWeight(const state::AccountData& accountData, const config::CommitteeConfiguration& config);
		static void logAccountData(const cache::AccountMap& accounts, const config::CommitteeConfiguration& config);
		static void decreaseActivity(const Key& key, cache::AccountMap& accounts, const config::CommitteeConfiguration& config);

		void setFilter(predicate<const Key&, const config::CommitteeConfiguration&> filter) {
			m_filter = std::move(filter);
		}

		void setIneligibleHarvesterHandler(consumer<const Key&> ineligibleHarvesterHandler) {
			m_ineligibleHarvesterHandler = std::move(ineligibleHarvesterHandler);
		}

	protected:
		class Hasher {
		public:
			static Hash256& calculateHash(Hash256& hash, const GenerationHash& generationHash, const Key& key);
			static Hash256& calculateHash(Hash256& hash);
			static Hash256 calculateHash(int64_t rate, const Key& key);
		};

		class Rate {
		public:
			explicit Rate(int64_t value, const Key& key)
				: m_value(value)
				, m_key(key)
			{}

		public:
			int64_t value() const {
				return m_value;
			}

			Hash256 hash() const {
				return Hasher::calculateHash(m_value, m_key);
			}

		private:
			int64_t m_value;
			const Key& m_key;
		};

		struct RateLess {
			bool operator()(const Rate& x, const Rate& y) const {
				if (x.value() == y.value())
					return (x.hash() < y.hash());

				return (x.value() < y.value());
			}
		};

		struct RateGreater {
			bool operator()(const Rate& x, const Rate& y) const {
				if (x.value() == y.value())
					return (x.hash() > y.hash());

				return (x.value() > y.value());
			}
		};

	protected:
		std::shared_ptr<cache::CommitteeAccountCollector> m_pAccountCollector;
		std::map<Key, Hash256> m_hashes;
		cache::AccountMap m_accounts;
		Timestamp m_timestamp;
		uint64_t m_phaseTime;
		mutable std::mutex m_mutex;
		predicate<const Key&, const config::CommitteeConfiguration&> m_filter;
		consumer<const Key&> m_ineligibleHarvesterHandler;
	};
}}
