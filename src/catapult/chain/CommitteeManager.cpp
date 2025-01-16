/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "CommitteeManager.h"

namespace catapult { namespace chain {

	void IncreasePhaseTime(uint64_t& phaseTimeMillis, const model::NetworkConfiguration& config) {
		if (1.0 == config.CommitteeTimeAdjustment)
			return;

		auto maxPhaseTimeMillis = config.MaxCommitteePhaseTime.millis();
		if (phaseTimeMillis == maxPhaseTimeMillis)
			return;

		phaseTimeMillis *= config.CommitteeTimeAdjustment;
		if (phaseTimeMillis > maxPhaseTimeMillis)
			phaseTimeMillis = maxPhaseTimeMillis;
	}

	void DecreasePhaseTime(uint64_t& phaseTimeMillis, const model::NetworkConfiguration& config) {
		if (1.0 == config.CommitteeTimeAdjustment)
			return;

		auto minPhaseTimeMillis = config.MinCommitteePhaseTime.millis();
		if (phaseTimeMillis == minPhaseTimeMillis)
			return;

		phaseTimeMillis /= config.CommitteeTimeAdjustment;
		if (phaseTimeMillis < minPhaseTimeMillis)
			phaseTimeMillis = minPhaseTimeMillis;
	}

	void CommitteeManager::setLastBlockElementSupplier(const model::BlockElementSupplier& supplier) {
		if (!!m_lastBlockElementSupplier)
			CATAPULT_THROW_RUNTIME_ERROR("last block element supplier already set");

		m_lastBlockElementSupplier = supplier;
	}

	const model::BlockElementSupplier& CommitteeManager::lastBlockElementSupplier() {
		if (!m_lastBlockElementSupplier)
			CATAPULT_THROW_RUNTIME_ERROR("last block element supplier not set");

		return m_lastBlockElementSupplier;
	}

	bool Committee::validateBlockProposer(const Key& key) const {
		if (BlockProposers.size() < 2)
			return (key == BlockProposer);

		return std::any_of(BlockProposers.cbegin(), BlockProposers.cend(), [&key](const Key& blockProposer) {
			return (key == blockProposer);
		});
	}
}}
