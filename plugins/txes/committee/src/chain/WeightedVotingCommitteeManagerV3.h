/**
*** Copyright 2024 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "WeightedVotingCommitteeManagerV2.h"

namespace catapult { namespace chain {

	/// Committee manager that implements weighted voting selection of committee.
	class WeightedVotingCommitteeManagerV3 : public WeightedVotingCommitteeManagerV2 {
	public:
		/// Creates a weighted voting committee manager around \a pAccountCollector.
		explicit WeightedVotingCommitteeManagerV3(std::shared_ptr<cache::CommitteeAccountCollector> pAccountCollector);

		void reset() override;
		void logCommittee() const override;

	public:
		void selectCommittee(const model::NetworkConfiguration& config, const BlockchainVersion& blockchainVersion) override;
		std::map<dbrb::ProcessId, BlockDuration> banPeriods() const override;

	private:
		utils::KeySet m_failedBlockProposers;
		utils::KeySet m_ineligibleHarvesters;
		std::map<dbrb::ProcessId, BlockDuration> m_banPeriods;
	};
}}
