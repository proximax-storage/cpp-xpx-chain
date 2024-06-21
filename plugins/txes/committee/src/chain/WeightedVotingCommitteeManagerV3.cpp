/**
*** Copyright 2024 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "WeightedVotingCommitteeManagerV3.h"
#include "src/config/CommitteeConfiguration.h"
#include "catapult/utils/NetworkTime.h"

namespace catapult { namespace chain {

	void WeightedVotingCommitteeManagerV3::logCommittee() const {
		std::lock_guard<std::mutex> guard(m_mutex);

		logHarvesters();

		std::ostringstream out;
		auto iter = m_accounts.find(m_committee.BlockProposer);
		if (iter == m_accounts.end())
			CATAPULT_THROW_RUNTIME_ERROR_1("account not found", m_committee.BlockProposer)
		logProcess("block proposer ", m_committee.BlockProposer, iter->second.ExpirationTime, out);

		CATAPULT_LOG(debug) << out.str();
	}

	WeightedVotingCommitteeManagerV3::WeightedVotingCommitteeManagerV3(std::shared_ptr<cache::CommitteeAccountCollector> pAccountCollector)
		: WeightedVotingCommitteeManagerV2(std::move(pAccountCollector))
	{}

	const Committee& WeightedVotingCommitteeManagerV3::selectCommittee(const model::NetworkConfiguration& networkConfig, const BlockchainVersion& blockchainVersion) {
		std::lock_guard<std::mutex> guard(m_mutex);

		const auto& config = networkConfig.GetPluginConfiguration<config::CommitteeConfiguration>();
		auto rates = getCandidates(networkConfig, config, blockchainVersion);

		// The first account may be followed by the accounts with the same rate, select them all as candidates.
		auto endRateIter = rates.begin();
		if (endRateIter != rates.end())
			endRateIter = rates.equal_range(endRateIter->first).second;
		std::map<Rate, Key, RateGreater> candidates;
		for (auto rateIter = rates.begin(); rateIter != endRateIter; ++rateIter)
			candidates.emplace(Rate(rateIter->first, rateIter->second), rateIter->second);

		if (candidates.empty())
			CATAPULT_THROW_RUNTIME_ERROR_1("no block proposer candidates", m_committee.Round);

		// Select block proposer.
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
		CATAPULT_LOG(trace) << "committee: block proposer " << m_committee.BlockProposer;

		return m_committee;
	}
}}
