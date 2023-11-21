/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "WeightedVotingCommitteeManager.h"
#include "src/config/CommitteeConfiguration.h"
#include "catapult/utils/NetworkTime.h"

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

		void LogAccountData(const cache::AccountMap& accounts) {
			for (const auto& pair : accounts) {
				const auto& data = pair.second;
				CATAPULT_LOG_LEVEL(utils::LogLevel::Trace) << "committee account " << pair.first << " data: "
					<< CalculateWeight(data) << "|" << data.Activity << "|" << data.Greed << "|" << data.LastSigningBlockHeight << "|"
					<< data.CanHarvest << "|" << data.EffectiveBalance;
			}
		}

		void DecreaseActivity(const Key& key, cache::AccountMap& accounts, const config::CommitteeConfiguration& config) {
			auto& data = accounts.at(key);
			auto& activity = data.Activity;
			activity -= config.ActivityCommitteeNotCosignedDelta;

			CATAPULT_LOG_LEVEL(utils::LogLevel::Trace) << "committee account " << key << ": activity " << activity << ", weight " << CalculateWeight(data);
		}

		void LogProcess(const char* prefix, const Key& process, const Timestamp& timestamp, std::ostringstream& out) {
			auto time = std::chrono::system_clock::to_time_t(utils::ToTimePoint(timestamp));
			char buffer[40];
			std::strftime(buffer, 40 ,"%F %T", std::localtime(&time));
			out << std::endl << prefix << process << " expires at: " << buffer;
		}

		void LogHarvesters(const cache::AccountMap& accounts) {
			std::ostringstream out;
			out << std::endl << "Harvesters (" << accounts.size() << "):";
			for (const auto& [ key, data ] : accounts)
				LogProcess("harvester ", key, data.ExpirationTime, out);

			CATAPULT_LOG(trace) << out.str();
		}

		void LogCommittee(const cache::AccountMap& accounts, const Committee& committee) {
			std::ostringstream out;
			out << std::endl << "Committee (" << committee.Cosigners.size() + 1u << "):";
			LogProcess("block proposer ", committee.BlockProposer, accounts.at(committee.BlockProposer).ExpirationTime, out);
			for (const auto& key : committee.Cosigners)
				LogProcess("committee member ", key, accounts.at(key).ExpirationTime, out);

			CATAPULT_LOG(debug) << out.str();
		}
	}

	void WeightedVotingCommitteeManager::logCommittee() const {
		LogHarvesters(m_accounts);
		LogCommittee(m_accounts, m_committee);
	}

	WeightedVotingCommitteeManager::WeightedVotingCommitteeManager(std::shared_ptr<cache::CommitteeAccountCollector> pAccountCollector)
		: m_pHasher(std::make_unique<DefaultHasher>())
		, m_pAccountCollector(std::move(pAccountCollector))
		, m_accounts(m_pAccountCollector->accounts())
		, m_phaseTime(0u)
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
		CATAPULT_LOG_LEVEL(utils::LogLevel::Trace) << "decreasing activities of previous committee accounts";
		DecreaseActivity(m_committee.BlockProposer, m_accounts, config);
		for (const auto& cosigner : m_committee.Cosigners) {
			DecreaseActivity(cosigner, m_accounts, config);
		}
	}

	const Committee& WeightedVotingCommitteeManager::selectCommittee(const model::NetworkConfiguration& networkConfig) {
		auto previousRound = m_committee.Round;
		const auto& config = networkConfig.GetPluginConfiguration<config::CommitteeConfiguration>();
		auto pLastBlockElement = lastBlockElementSupplier()();
		if (previousRound < 0) {
			m_phaseTime = pLastBlockElement->Block.committeePhaseTime();
			if (!m_phaseTime)
				m_phaseTime = networkConfig.CommitteePhaseTime.millis();
			m_timestamp = pLastBlockElement->Block.Timestamp;
			LogAccountData(m_accounts);
		} else {
			if (previousRound > 0) {
				IncreasePhaseTime(m_phaseTime, networkConfig);
			} else {
				DecreasePhaseTime(m_phaseTime, networkConfig);
			}
			decreaseActivities(config);
		}
		m_timestamp = m_timestamp + Timestamp(CommitteePhaseCount * m_phaseTime);

		m_committee = Committee(previousRound + 1);

		// Compute account rates and sort them in descending order.
		std::multimap<double, Key, std::greater<>> rates;
		for (const auto& pair : m_accounts) {
			const auto& key = pair.first;
			const auto& accountData = pair.second;

			if (!accountData.CanHarvest)
				continue;

			if (networkConfig.EnableHarvesterExpiration && accountData.ExpirationTime <= m_timestamp && (networkConfig.EmergencyHarvesters.find(key) == networkConfig.EmergencyHarvesters.cend()))
				continue;

			auto weight = CalculateWeight(accountData);
			const auto& hash = previousRound < 0 ?
				m_pHasher->calculateHash(m_hashes[key], pLastBlockElement->GenerationHash, key) :
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

		CATAPULT_LOG_LEVEL(utils::LogLevel::Trace) << "committee: block proposer " << m_committee.BlockProposer;
		for (const auto& cosigner : m_committee.Cosigners)
			CATAPULT_LOG_LEVEL(utils::LogLevel::Trace) << "committee: cosigner " << cosigner;

		return m_committee;
	}
}}
