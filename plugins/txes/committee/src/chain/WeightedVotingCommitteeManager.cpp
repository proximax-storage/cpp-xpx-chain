/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "WeightedVotingCommitteeManager.h"
#include "src/config/CommitteeConfiguration.h"

namespace catapult { namespace chain {

	namespace {
		class DefaultHasher : public Hasher {
		public:
			Hash256& calculateHash(Hash256& hash, const GenerationHash& generationHash, const Key& key) const override {
				crypto::Sha3_256_Builder sha3;
				sha3.update({ generationHash, key });
				sha3.final(hash);
				return hash;
			}

			Hash256& calculateHash(Hash256& hash) const override {
				crypto::Sha3_256_Builder sha3;
				sha3.update(hash);
				sha3.final(hash);
				return hash;
			}

			 Hash256 calculateHash(double rate, const Key& key) const override {
				Hash256 hash;
				crypto::Sha3_256_Builder sha3;
				sha3.update({ RawBuffer{reinterpret_cast<const uint8_t*>(&rate), sizeof(rate)}, key });
				sha3.final(hash);
				return hash;
			}
		};

		class Rate {
		public:
			explicit Rate(double value, const Key& key, const Hasher& hasher)
				: m_value(value)
				, m_key(key)
				, m_hasher(hasher)
			{}

		public:
			double value() const {
				return m_value;
			}

			Hash256 hash() const {
				return m_hasher.calculateHash(m_value, m_key);
			}

		private:
			double m_value;
			const Key& m_key;
			const Hasher& m_hasher;
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
					return (x.hash() < y.hash());

				return (x.value() > y.value());
			}
		};

		double CalculateWeight(const state::AccountData& accountData) {
			return 1.0 / (1.0 + std::exp(-accountData.Activity));
		}

		void LogAccountData(const cache::AccountMap& accounts, utils::LogLevel logLevel) {
			for (const auto& pair : accounts) {
				const auto& data = pair.second;
				CATAPULT_LOG_LEVEL(logLevel) << "committee account " << pair.first << " data: "
					<< CalculateWeight(data) << "|" << data.Activity << data.Greed << "|" << data.LastSigningBlockHeight << "|"
					<< data.CanHarvest << "|" << data.EffectiveBalance;
			}
		}

		void DecreaseActivity(const Key& key, cache::AccountMap& accounts, const config::CommitteeConfiguration& config, utils::LogLevel logLevel) {
			auto& data = accounts.at(key);
			auto& activity = data.Activity;
			activity -= config.ActivityCommitteeNotCosignedDelta;

			CATAPULT_LOG_LEVEL(logLevel) << "committee account " << key << ": activity " << activity << ", weight " << CalculateWeight(data);
		}
	}

	WeightedVotingCommitteeManager::WeightedVotingCommitteeManager(const std::shared_ptr<cache::CommitteeAccountCollector>& pAccountCollector)
		: m_pHasher(std::make_unique<DefaultHasher>())
		, m_pAccountCollector(pAccountCollector)
		, m_accounts(m_pAccountCollector->accounts())
		, m_logLevel(utils::LogLevel::Debug)
	{}

	void WeightedVotingCommitteeManager::reset() {
		m_hashes.clear();
		m_committee = Committee();
		m_accounts = m_pAccountCollector->accounts();
	}

	double WeightedVotingCommitteeManager::weight(const Key& accountKey) const {
		auto iter = m_accounts.find(accountKey);

		return (m_accounts.end() != iter) ? CalculateWeight(iter->second) : 0.0;
	}

	void WeightedVotingCommitteeManager::decreaseActivities(const config::CommitteeConfiguration& config) {
		CATAPULT_LOG_LEVEL(m_logLevel) << "decreasing activities of previous committee accounts";
		DecreaseActivity(m_committee.BlockProposer, m_accounts, config, m_logLevel);
		for (const auto& cosigner : m_committee.Cosigners) {
			DecreaseActivity(cosigner, m_accounts, config, m_logLevel);
		}
	}

	const Committee& WeightedVotingCommitteeManager::selectCommittee(const model::NetworkConfiguration& networkConfig) {
		auto previousRound = m_committee.Round;
		CATAPULT_LOG_LEVEL(m_logLevel) << "selecting committee for round " << previousRound + 1;
		const auto& config = networkConfig.GetPluginConfiguration<config::CommitteeConfiguration>();
		if (previousRound < 0) {
			LogAccountData(m_accounts, m_logLevel);
		} else {
			decreaseActivities(config);
		}

		m_committee = Committee(previousRound + 1);

		// Compute account rates and sort them in descending order.
		std::multimap<double, Key, std::greater<>> rates;
		for (const auto& pair : m_accounts) {
			const auto& accountData = pair.second;
			if (!accountData.CanHarvest)
				continue;

			const auto& key = pair.first;
			auto weight = CalculateWeight(accountData);
			const auto& hash = previousRound < 0 ?
				m_pHasher->calculateHash(m_hashes[key], lastBlockElementSupplier()()->GenerationHash, key) :
				m_pHasher->calculateHash(m_hashes.at(key));
			auto hit = *reinterpret_cast<const uint64_t*>(hash.data());
			if (hit == 0u)
				hit = 1u;
			auto rate = static_cast<double>(accountData.EffectiveBalance.unwrap()) / hit * weight;

			rates.emplace(rate, key);
		}

		// The 21st account may be followed by the accounts with the same rate, select them all as candidates for
		// the committee.
		auto endRateIter = rates.begin();
		for (auto i = 0u; i < networkConfig.CommitteeSize && endRateIter != rates.end(); ++i, ++endRateIter);
		if (endRateIter != rates.end())
			endRateIter = rates.equal_range(endRateIter->first).second;
		std::map<Rate, Key, RateGreater> committeeCandidates;
		for (auto rateIter = rates.begin(); rateIter != endRateIter; ++rateIter)
			committeeCandidates.emplace(Rate(rateIter->first, rateIter->second, *m_pHasher), rateIter->second);

		if (committeeCandidates.empty())
			CATAPULT_THROW_RUNTIME_ERROR_1("committee empty", m_committee.Round);

		// Select the first 21 candidates to the committee and select block proposer.
		std::map<Rate, Key, RateLess> blockProposerCandidates;
		auto candidateIter = committeeCandidates.begin();
		for (auto i = 0u; i < networkConfig.CommitteeSize && candidateIter != committeeCandidates.end(); ++i, ++candidateIter) {
			const auto& key = candidateIter->second;
			m_committee.Cosigners.insert(key);

			auto greed = std::max(m_accounts.at(key).Greed, config.MinGreed);
			auto hit = *reinterpret_cast<const uint64_t*>(m_hashes.at(key).data());
			blockProposerCandidates.emplace(Rate(greed * hit, key, *m_pHasher), key);
		}

		if (m_committee.Cosigners.size() < networkConfig.CommitteeSize)
			CATAPULT_THROW_RUNTIME_ERROR_2("committee not full", m_committee.Cosigners.size(), networkConfig.CommitteeSize);

		m_committee.BlockProposer = blockProposerCandidates.begin()->second;
		m_committee.Cosigners.erase(m_committee.BlockProposer);

		CATAPULT_LOG_LEVEL(m_logLevel) << "committee: block proposer " << m_committee.BlockProposer;
		for (const auto& cosigner : m_committee.Cosigners)
			CATAPULT_LOG_LEVEL(m_logLevel) << "committee: cosigner " << cosigner;

		return m_committee;
	}
}}
