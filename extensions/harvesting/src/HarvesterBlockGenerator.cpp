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

#include "HarvesterBlockGenerator.h"
#include "HarvestingUtFacadeFactory.h"
#include "TransactionsInfoSupplier.h"

namespace catapult { namespace harvesting {

	BlockGenerator CreateHarvesterBlockGenerator(
			model::TransactionSelectionStrategy strategy,
			const HarvestingUtFacadeFactory& utFacadeFactory,
			const cache::MemoryUtCache& utCache) {
		auto transactionsInfoSupplier = CreateTransactionsInfoSupplier(strategy, utCache);
		return [utFacadeFactory, transactionsInfoSupplier](const auto& blockHeader, auto maxTransactionsPerBlock) {
			// 1. check height consistency
			auto pUtFacade = utFacadeFactory.create(blockHeader.Timestamp);
			if (blockHeader.Height != pUtFacade->height()) {
				CATAPULT_LOG(debug)
						<< "bypassing state hash calculation because cache height (" << pUtFacade->height() - Height(1)
						<< ") is inconsistent with block height (" << blockHeader.Height << ")";
				return std::unique_ptr<model::Block>();
			}

			// 2. select transactions
			auto transactionsInfo = transactionsInfoSupplier(*pUtFacade, maxTransactionsPerBlock);

			// 3. build a block
			auto pBlock = pUtFacade->commit(blockHeader);
			if (!pBlock) {
				CATAPULT_LOG(warning) << "failed to generate harvested block";
				return std::unique_ptr<model::Block>();
			}

			pBlock->BlockTransactionsHash = transactionsInfo.TransactionsHash;
			pBlock->FeeMultiplier = transactionsInfo.FeeMultiplier;
			return pBlock;
		};
	}
}}
