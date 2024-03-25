/**
*** Copyright 2024 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "WeightedVotingCommitteeManagerV2.h"
#include "src/config/CommitteeConfiguration.h"
#include "catapult/utils/NetworkTime.h"

namespace catapult { namespace chain {

	namespace {
		constexpr auto UNITS_IN_THE_LAST_PLACE = 2;

		class Hasher {
		public:
			static Hash256& calculateHash(Hash256& hash, const GenerationHash& generationHash, const Key& key) {
				crypto::Sha3_256_Builder sha3;
				sha3.update({ generationHash, key });
				sha3.final(hash);
				return hash;
			}

			static Hash256& calculateHash(Hash256& hash) {
				crypto::Sha3_256_Builder sha3;
				sha3.update(hash);
				sha3.final(hash);
				return hash;
			}

			 static Hash256 calculateHash(int64_t rate, const Key& key) {
				Hash256 hash;
				crypto::Sha3_256_Builder sha3;
				sha3.update({ RawBuffer{reinterpret_cast<const uint8_t*>(&rate), sizeof(rate)}, key });
				sha3.final(hash);
				return hash;
			}
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

		int64_t CalculateWeight(const state::AccountData& accountData, const config::CommitteeConfiguration& config) {
			auto weight = static_cast<int64_t>(config.WeightScaleFactor / (1.0 + std::exp(static_cast<double>(-accountData.Activity) / config.ActivityScaleFactor)));
			if (weight == 0)
				weight = 1;

			return weight;
		}

		void LogAccountData(const cache::AccountMap& accounts, const config::CommitteeConfiguration& config) {
			CATAPULT_LOG(trace) << "Harvester account data:";
			for (const auto& pair : accounts) {
				const auto& data = pair.second;
				CATAPULT_LOG(trace) << "committee account " << pair.first << " data: "
					<< CalculateWeight(data, config) << "|" << data.Activity << "|" << data.FeeInterest << "|" << data.FeeInterestDenominator << "|" << data.LastSigningBlockHeight << "|"
					<< data.CanHarvest << "|" << data.EffectiveBalance;
			}
		}

		void DecreaseActivity(const Key& key, cache::AccountMap& accounts, const config::CommitteeConfiguration& config) {
			auto iter = accounts.find(key);
			if (iter == accounts.end())
				CATAPULT_THROW_RUNTIME_ERROR_1("account not found", key)
			auto& data = iter->second;
			data.decreaseActivity(config.ActivityCommitteeNotCosignedDeltaInt);

			CATAPULT_LOG(trace) << "committee account " << key << ": activity " << data.Activity << ", weight " << CalculateWeight(data, config);
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
			auto iter = accounts.find(committee.BlockProposer);
			if (iter == accounts.end())
				CATAPULT_THROW_RUNTIME_ERROR_1("account not found", committee.BlockProposer)
			LogProcess("block proposer ", committee.BlockProposer, iter->second.ExpirationTime, out);
			for (const auto& key : committee.Cosigners) {
				iter = accounts.find(key);
				if (iter == accounts.end())
					CATAPULT_THROW_RUNTIME_ERROR_1("account not found", key)
				LogProcess("committee member ", key, iter->second.ExpirationTime, out);
			}

			CATAPULT_LOG(debug) << out.str();
		}
	}

	void WeightedVotingCommitteeManagerV2::logCommittee() const {
		std::lock_guard<std::mutex> guard(m_mutex);

		LogHarvesters(m_accounts);
		LogCommittee(m_accounts, m_committee);
	}

	WeightedVotingCommitteeManagerV2::WeightedVotingCommitteeManagerV2(std::shared_ptr<cache::CommitteeAccountCollector> pAccountCollector)
		: m_pAccountCollector(std::move(pAccountCollector))
		, m_accounts(m_pAccountCollector->accounts())
		, m_phaseTime(0u)
	{}

	void WeightedVotingCommitteeManagerV2::reset() {
		std::lock_guard<std::mutex> guard(m_mutex);

		m_hashes.clear();
		m_committee = Committee();
		m_accounts = m_pAccountCollector->accounts();
	}

	Committee WeightedVotingCommitteeManagerV2::committee() const {
		std::lock_guard<std::mutex> guard(m_mutex);

		return m_committee;
	}

	cache::AccountMap WeightedVotingCommitteeManagerV2::accounts() {
		std::lock_guard<std::mutex> guard(m_mutex);

		return m_accounts;
	}

	HarvesterWeight WeightedVotingCommitteeManagerV2::weight(const Key& accountKey, const model::NetworkConfiguration& networkConfig) const {
		std::lock_guard<std::mutex> guard(m_mutex);

		auto iter = m_accounts.find(accountKey);
		HarvesterWeight weight{};
		const auto& config = networkConfig.GetPluginConfiguration<config::CommitteeConfiguration>();
		weight.n = (m_accounts.end() != iter) ? CalculateWeight(iter->second, config) : 0;

		return weight;
	}

	HarvesterWeight WeightedVotingCommitteeManagerV2::zeroWeight() const {
		return HarvesterWeight{ .n = 0 };
	}

	void WeightedVotingCommitteeManagerV2::add(HarvesterWeight& weight, const chain::HarvesterWeight& delta) const {
		weight.n += delta.n;
	}

	void WeightedVotingCommitteeManagerV2::mul(HarvesterWeight& weight, double multiplier) const {
		weight.n = static_cast<int64_t>(static_cast<double>(weight.n) * multiplier);
	}

	bool WeightedVotingCommitteeManagerV2::ge(const HarvesterWeight& weight1, const chain::HarvesterWeight& weight2) const {
		return weight1.n >= weight2.n
		   || std::abs(weight1.n - weight2.n) <= static_cast<int64_t>(std::numeric_limits<double>::epsilon() * std::abs(static_cast<double>(weight1.n) + static_cast<double>(weight2.n)));
	}

	bool WeightedVotingCommitteeManagerV2::eq(const HarvesterWeight& weight1, const chain::HarvesterWeight& weight2) const {
		return weight1.n == weight1.n;
	}

	std::string WeightedVotingCommitteeManagerV2::str(const HarvesterWeight& weight) const {
		std::ostringstream out;
		out << weight.n;

		return out.str();
	}

	void WeightedVotingCommitteeManagerV2::decreaseActivities(const config::CommitteeConfiguration& config) {
		CATAPULT_LOG(trace) << "decreasing activities of previous committee accounts";
		DecreaseActivity(m_committee.BlockProposer, m_accounts, config);
		for (const auto& cosigner : m_committee.Cosigners) {
			DecreaseActivity(cosigner, m_accounts, config);
		}
	}

	Key WeightedVotingCommitteeManagerV2::getBootKey(const Key& harvestKey, const model::NetworkConfiguration& config) const {
		auto accountIter = m_accounts.find(harvestKey);
		if (accountIter != m_accounts.cend() && accountIter->second.BootKey != Key()) {
			CATAPULT_LOG(trace) << "Boot key found: " << harvestKey << " - " << accountIter->second.BootKey;
			return accountIter->second.BootKey;
		}

		auto bootstrapIter = config.BootstrapHarvesters.find(harvestKey);
		if (bootstrapIter != config.BootstrapHarvesters.cend()) {
			CATAPULT_LOG(trace) << "Boot key of bootstrap harvester found: " << harvestKey << " - " << bootstrapIter->second;
			return bootstrapIter->second;
		}

		CATAPULT_LOG(warning) << "Boot key not found for " << harvestKey;
		return {};
	}

	const Committee& WeightedVotingCommitteeManagerV2::selectCommittee(const model::NetworkConfiguration& networkConfig) {
		std::lock_guard<std::mutex> guard(m_mutex);

		auto previousRound = m_committee.Round;
		const auto& config = networkConfig.GetPluginConfiguration<config::CommitteeConfiguration>();
		LogAccountData(m_accounts, config);
		auto pLastBlockElement = lastBlockElementSupplier()();
		if (previousRound < 0) {
			m_phaseTime = pLastBlockElement->Block.committeePhaseTime();
			if (!m_phaseTime)
				m_phaseTime = networkConfig.CommitteePhaseTime.millis();
			m_timestamp = pLastBlockElement->Block.Timestamp;
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
		std::multimap<int64_t, Key, std::greater<>> rates;
		for (const auto& pair : m_accounts) {
			const auto& key = pair.first;
			const auto& accountData = pair.second;

			if (!accountData.CanHarvest)
				continue;

			if (networkConfig.BootstrapHarvesters.empty()) {
				if (networkConfig.EnableHarvesterExpiration && accountData.ExpirationTime <= m_timestamp && (networkConfig.EmergencyHarvesters.find(key) == networkConfig.EmergencyHarvesters.cend()))
					continue;
			} else {
				auto iter = networkConfig.BootstrapHarvesters.find(key);
				if (iter == networkConfig.BootstrapHarvesters.cend()) {
					if (networkConfig.EnableHarvesterExpiration && accountData.ExpirationTime <= m_timestamp)
						continue;

					if (accountData.BootKey == Key())
						continue;
				}
			}

			auto weight = static_cast<double>(CalculateWeight(accountData, config));
			const auto& hash = previousRound < 0 ?
				Hasher::calculateHash(m_hashes[key], pLastBlockElement->GenerationHash, key) :
				Hasher::calculateHash(m_hashes[key]);
			auto hit = *reinterpret_cast<const uint64_t*>(hash.data());
			if (hit == 0u)
				hit = 1u;
			auto stake = static_cast<double>(accountData.EffectiveBalance.unwrap()) / static_cast<double>(hit);
			auto fRate = stake * weight;
			auto nRate = std::numeric_limits<int64_t>::max();
			if (fRate < static_cast<double>(std::numeric_limits<int64_t>::max())) {
				nRate = static_cast<int64_t>(fRate);
				if (nRate == 0)
					nRate = 1;
			}

			CATAPULT_LOG(trace) << "rate of " << key << ": " << nRate;

			rates.emplace(nRate, key);
		}

		// The 21st account may be followed by the accounts with the same rate, select them all as candidates for
		// the committee.
		auto endRateIter = rates.begin();
		for (auto i = 0u; i < networkConfig.CommitteeSize && endRateIter != rates.end(); ++i, ++endRateIter);
		if (endRateIter != rates.end())
			endRateIter = rates.equal_range(endRateIter->first).second;
		std::map<Rate, Key, RateGreater> committeeCandidates;
		for (auto rateIter = rates.begin(); rateIter != endRateIter; ++rateIter)
			committeeCandidates.emplace(Rate(rateIter->first, rateIter->second), rateIter->second);

		if (committeeCandidates.empty())
			CATAPULT_THROW_RUNTIME_ERROR_1("committee empty", m_committee.Round);

		// Select the first 21 candidates to the committee and select block proposer.
		std::map<Rate, Key, RateLess> blockProposerCandidates;
		auto candidateIter = committeeCandidates.begin();
		for (auto i = 0u; i < networkConfig.CommitteeSize && candidateIter != committeeCandidates.end(); ++i, ++candidateIter) {
			const auto& key = candidateIter->second;
			m_committee.Cosigners.insert(key);

			const auto& data = m_accounts.at(key);
			auto greed = static_cast<double>(data.FeeInterest) / static_cast<double>(data.FeeInterestDenominator);
			auto minGreed = static_cast<double>(config.MinGreedFeeInterest) / static_cast<double>(config.MinGreedFeeInterestDenominator);
			greed = std::max(greed, minGreed);
			auto hit = *reinterpret_cast<const uint64_t*>(m_hashes[key].data());
			blockProposerCandidates.emplace(Rate(greed * static_cast<double>(hit), key), key);
		}

		if (m_committee.Cosigners.size() < networkConfig.CommitteeSize)
			CATAPULT_THROW_RUNTIME_ERROR_2("committee not full", m_committee.Cosigners.size(), networkConfig.CommitteeSize);

		m_committee.BlockProposer = blockProposerCandidates.begin()->second;
		m_committee.Cosigners.erase(m_committee.BlockProposer);

		CATAPULT_LOG(trace) << "committee: block proposer " << m_committee.BlockProposer;
		for (const auto& cosigner : m_committee.Cosigners)
			CATAPULT_LOG(trace) << "committee: cosigner " << cosigner;

		return m_committee;
	}
}}
