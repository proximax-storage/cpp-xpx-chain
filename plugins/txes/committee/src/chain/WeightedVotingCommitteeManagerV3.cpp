/**
*** Copyright 2024 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "WeightedVotingCommitteeManagerV3.h"
#include "src/config/CommitteeConfiguration.h"
#include "catapult/utils/NetworkTime.h"

namespace catapult { namespace chain {

	WeightedVotingCommitteeManagerV3::WeightedVotingCommitteeManagerV3(std::shared_ptr<cache::CommitteeAccountCollector> pAccountCollector)
			: WeightedVotingCommitteeManagerV2(std::move(pAccountCollector)) {
		setFilter([&failedBlockProposers = m_failedBlockProposers](const Key& key, const config::CommitteeConfiguration& config) {
			if (config.EnableHarvesterRotation && failedBlockProposers.find(key) != failedBlockProposers.cend()) {
				CATAPULT_LOG(debug) << "rejecting failed harvester " << key;
				return false;
			}

			return true;
		});

		setIneligibleHarvesterHandler([&ineligibleHarvesters = m_ineligibleHarvesters](const Key& key) {
			ineligibleHarvesters.insert(key);
		});
	}

	void WeightedVotingCommitteeManagerV3::reset() {
		std::lock_guard<std::mutex> guard(m_mutex);

		m_hashes.clear();
		m_committee = Committee();
		m_accounts = m_pAccountCollector->accounts();
		m_failedBlockProposers.clear();
		m_ineligibleHarvesters.clear();
		m_banPeriods.clear();
	}

	void WeightedVotingCommitteeManagerV3::logCommittee() const {
		std::lock_guard<std::mutex> guard(m_mutex);

		logHarvesters();

		std::ostringstream out;
		for (const auto& blockProposer : m_committee.BlockProposers) {
			auto iter = m_accounts.find(blockProposer);
			if (iter == m_accounts.end())
				CATAPULT_THROW_RUNTIME_ERROR_1("account not found", blockProposer)
			logProcess("block proposer ", blockProposer, iter->second.ExpirationTime, out);
		}
		CATAPULT_LOG(debug) << out.str();
	}

	void WeightedVotingCommitteeManagerV3::selectCommittee(const model::NetworkConfiguration& networkConfig, const BlockchainVersion& blockchainVersion) {
		std::lock_guard<std::mutex> guard(m_mutex);

		const auto& config = networkConfig.GetPluginConfiguration<config::CommitteeConfiguration>();
		if (m_committee.Round >= 0) {
			for (const auto& blockProposer : m_committee.BlockProposers) {
				bool notBootstrapHarvester = (networkConfig.BootstrapHarvesters.find(blockProposer) == networkConfig.BootstrapHarvesters.cend());
				bool notEmergencyHarvester = (networkConfig.EmergencyHarvesters.find(blockProposer) == networkConfig.EmergencyHarvesters.cend());
				if (notBootstrapHarvester && notEmergencyHarvester) {
					auto iter = m_accounts.find(blockProposer);
					if (iter == m_accounts.end())
						CATAPULT_THROW_RUNTIME_ERROR_1("block proposer not found", blockProposer)
					auto blockGenerationTargetTime = utils::TimeSpan::FromMilliseconds(networkConfig.MinCommitteePhaseTime.millis() * chain::CommitteePhaseCount);
					iter->second.BanPeriod = config.HarvesterBanPeriod.blocks(blockGenerationTargetTime);
					m_banPeriods[iter->second.BootKey] = iter->second.BanPeriod;
					CATAPULT_LOG(warning) << "banned harvester " << blockProposer << " (process id " << iter->second.BootKey << ") for " << iter->second.BanPeriod << " blocks";
				}
			}
		}

		auto rates = getCandidates(networkConfig, config, blockchainVersion);

		std::map<Rate, Key, RateGreater> candidates;
		for (const auto& [rate, key] : rates)
			candidates.emplace(Rate(rate, key), key);

		if (candidates.empty())
			CATAPULT_THROW_RUNTIME_ERROR_1("no block proposer candidates", m_committee.Round);

		// Select block proposers.
		if (config.EnableBlockProducerSelectionImprovement) {
			m_committee.BlockProposer = candidates.begin()->second;
			for (auto iter = candidates.cbegin(); iter != candidates.cend() && m_committee.BlockProposers.size() < networkConfig.HarvestersQueueSize; ++iter) {
				const auto& key = iter->second;
				m_committee.BlockProposers.push_back(key);
				CATAPULT_LOG(trace) << "committee: block proposer [" << m_committee.BlockProposers.size() << "] " << key << " (stake " << (m_accounts.at(key).EffectiveBalance.unwrap() / 1'000'000) << " XPX)";

				// Reject this harvester when selecting block proposer for the next round.
				m_failedBlockProposers.insert(key);
				// If all harvesters are failing or ineligible, let the failing ones try again.
				if (m_failedBlockProposers.size() + m_ineligibleHarvesters.size() == m_accounts.size()) {
					CATAPULT_LOG(debug) << "clearing failed harvesters";
					m_failedBlockProposers.clear();
				}
			}
		} else {
			std::map<Rate, Key, RateLess> blockProposerCandidates;
			for (auto & candidate : candidates) {
				const auto& key = candidate.second;
				const auto& data = m_accounts.at(key);
				auto greed = static_cast<double>(data.FeeInterest) / static_cast<double>(data.FeeInterestDenominator);
				auto minGreed = static_cast<double>(config.MinGreedFeeInterest) / static_cast<double>(config.MinGreedFeeInterestDenominator);
				greed = std::max(greed, minGreed);
				auto hit = *reinterpret_cast<const uint64_t*>(m_hashes[key].data());
				blockProposerCandidates.emplace(Rate(static_cast<int64_t>(greed * static_cast<double>(hit)), key), key);
			}

			m_committee.BlockProposer = blockProposerCandidates.begin()->second;

			for (auto iter = blockProposerCandidates.cbegin(); iter != blockProposerCandidates.cend() && m_committee.BlockProposers.size() < networkConfig.HarvestersQueueSize; ++iter) {
				const auto& key = iter->second;
				m_committee.BlockProposers.push_back(key);
				CATAPULT_LOG(trace) << "committee: block proposer [" << m_committee.BlockProposers.size() << "] " << key << " (stake " << (m_accounts.at(key).EffectiveBalance.unwrap() / 1'000'000) << " XPX)";

				// Reject this harvester when selecting block proposer for the next round.
				m_failedBlockProposers.insert(key);
				// If all harvesters are failing or ineligible, let the failing ones try again.
				if (m_failedBlockProposers.size() + m_ineligibleHarvesters.size() == m_accounts.size()) {
					CATAPULT_LOG(debug) << "clearing failed harvesters";
					m_failedBlockProposers.clear();
				}
			}
		}
	}

	std::map<dbrb::ProcessId, BlockDuration> WeightedVotingCommitteeManagerV3::banPeriods() const {
		std::lock_guard<std::mutex> guard(m_mutex);
		return m_banPeriods;
	}
}}
