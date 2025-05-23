/**
*** Copyright (c) 2016-present,
*** Jaguar0625, gimre, BloodyRookie, Tech Bureau, Corp. All rights reserved.
***
*** This file is part of Catapult.
***
*** Catapult is free software: you can redistribute it and/or modify
*** it under the terms of the GNU Lesser General Public License as published by
*** the Free Software Foundation, either version 3 of the License, or
*** (at your option) any later version.
***
*** Catapult is distributed in the hope that it will be useful,
*** but WITHOUT ANY WARRANTY; without even the implied warranty of
*** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*** GNU Lesser General Public License for more details.
***
*** You should have received a copy of the GNU Lesser General Public License
*** along with Catapult. If not, see <http://www.gnu.org/licenses/>.
**/

#include "ScheduledHarvesterTask.h"

namespace catapult { namespace harvesting {

	void ScheduledHarvesterTask::harvest() {
		if (m_isAnyHarvestedBlockPending || !m_harvestingAllowed())
			return;

		auto pLastBlockElement = m_lastBlockElementSupplier();
		auto timestamp = m_timeSupplier();
		if (timestamp <= pLastBlockElement->Block.Timestamp)
			return;

		auto pBlock = m_pHarvester->harvest(*pLastBlockElement, timestamp);
		if (!pBlock)
			return;

		CATAPULT_LOG(info) << "successfully harvested block at " << pBlock->Height << " with signer " << pBlock->Signer;
		m_isAnyHarvestedBlockPending = true;
		m_rangeConsumer(model::BlockRange::FromEntity(std::move(pBlock)), [&isBlockPending = m_isAnyHarvestedBlockPending](auto, auto) {
			isBlockPending = false;
		});
	}
}}
