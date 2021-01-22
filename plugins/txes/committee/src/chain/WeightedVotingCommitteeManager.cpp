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
	}

	WeightedVotingCommitteeManager::WeightedVotingCommitteeManager(const std::shared_ptr<cache::CommitteeAccountCollector>& pAccountCollector)
		: m_pHasher(std::make_unique<DefaultHasher>())
		, m_pAccountCollector(pAccountCollector)
	{}

	void WeightedVotingCommitteeManager::reset() {
		m_hashes.clear();
		m_committee = Committee();
	}

	const Committee& WeightedVotingCommitteeManager::selectCommittee(const model::NetworkConfiguration& networkConfig) {
		auto generationHash = lastBlockElementSupplier()()->GenerationHash;
		const auto& accounts = m_pAccountCollector->accounts();
		auto round = m_committee.Round;
		m_committee = Committee(round + 1);

		// Compute account rates and sort them in descending order.
		std::multimap<double, Key, std::greater<>> rates;
		for (const auto& pair : accounts) {
			const auto& accountData = pair.second;
			if (!accountData.CanHarvest)
				continue;

			const auto& key = pair.first;
			auto weight = 1.0 / (1.0 + std::exp(-accountData.Activity));
			const auto& hash = round ?
				m_pHasher->calculateHash(m_hashes.at(key)) :
				m_pHasher->calculateHash(m_hashes[key], generationHash, key);
			auto hit = *reinterpret_cast<const uint64_t*>(hash.data());
			if (hit == 0u)
				hit = 1u;
			auto rate = static_cast<double>(accountData.EffectiveBalance.unwrap()) / hit * weight;

			rates.emplace(rate, key);
		}

		// The 21st account may be followed by the accounts with the same rate, select them all as candidates for
		// the committee.
		const auto& config = networkConfig.GetPluginConfiguration<config::CommitteeConfiguration>();
		auto endRateIter = rates.begin();
		for (auto i = 0u; i < networkConfig.CommitteeSize && endRateIter != rates.end(); ++i, ++endRateIter);
		if (endRateIter != rates.end())
			endRateIter = rates.equal_range(endRateIter->first).second;
		std::map<Rate, Key, RateGreater> committeeCandidates;
		for (auto rateIter = rates.begin(); rateIter != endRateIter; ++rateIter)
			committeeCandidates.emplace(Rate(rateIter->first, rateIter->second, *m_pHasher), rateIter->second);

		// TODO: what if factual committee number is zero or less than CommitteeNumber?
		if (committeeCandidates.empty()) {
			return m_committee;
		}

		// Select the first 21 candidates to the committee and select block proposer.
		std::map<Rate, Key, RateLess> blockProposerCandidates;
		auto candidateIter = committeeCandidates.begin();
		for (auto i = 0u; i < networkConfig.CommitteeSize && candidateIter != committeeCandidates.end(); ++i, ++candidateIter) {
			const auto& key = candidateIter->second;
			m_committee.Cosigners.insert(key);

			auto greed = std::max(accounts.at(key).Greed, config.MinGreed);
			auto hit = *reinterpret_cast<const uint64_t*>(m_hashes.at(key).data());
			blockProposerCandidates.emplace(Rate(greed * hit, key, *m_pHasher), key);
		}

		m_committee.BlockProposer = blockProposerCandidates.begin()->second;
		m_committee.Cosigners.erase(m_committee.BlockProposer);

		return m_committee;
	}
}}
