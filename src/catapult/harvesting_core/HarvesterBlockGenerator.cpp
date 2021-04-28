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

	namespace {
		model::UniqueEntityPtr<model::Block> GenerateBlock(
				HarvestingUtFacade& facade,
				const model::Block& originalBlockHeader,
				const TransactionsInfo& transactionsInfo) {
			// copy and update block header
			model::UniqueEntityPtr<model::Block> pBlockHeader = utils::MakeUniqueWithSize<model::Block>(originalBlockHeader.Size);
			std::memcpy(static_cast<void*>(pBlockHeader.get()), &originalBlockHeader, originalBlockHeader.Size);
			pBlockHeader->BlockTransactionsHash = transactionsInfo.TransactionsHash;
			pBlockHeader->FeeMultiplier = transactionsInfo.FeeMultiplier;

			// generate the block
			return facade.commit(*pBlockHeader);
		}
	}

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
				return model::UniqueEntityPtr<model::Block>();
			}

			// 2. select transactions
			auto transactionsInfo = transactionsInfoSupplier(*pUtFacade, maxTransactionsPerBlock);

			// 3. re-apply transactions if they were truncated
			if (pUtFacade->size() != transactionsInfo.Transactions.size()) {
				pUtFacade.reset(nullptr);
				pUtFacade = utFacadeFactory.create(blockHeader.Timestamp);
				for (auto pTransactionInfo : transactionsInfo.Transactions) {
					if (!pUtFacade->apply(*pTransactionInfo))
						CATAPULT_THROW_RUNTIME_ERROR_1("transaction re-applying failed", pTransactionInfo->EntityHash)
				}
			}

			// 4. build a block
			auto pBlock = GenerateBlock(*pUtFacade, blockHeader, transactionsInfo);
			if (!pBlock) {
				CATAPULT_LOG(warning) << "failed to generate harvested block";
				return model::UniqueEntityPtr<model::Block>();
			}

			return pBlock;
		};
	}
}}
