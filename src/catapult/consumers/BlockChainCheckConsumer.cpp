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

#include "BlockConsumers.h"
#include "ConsumerResultFactory.h"
#include "catapult/chain/ChainUtils.h"

namespace catapult { namespace consumers {

	namespace {
		namespace {
			bool IsLink(const model::BlockElement& previousElement, const model::Block& currentBlock) {
				return chain::IsChainLink(previousElement.Block, previousElement.EntityHash, currentBlock);
			}
		}

		class BlockChainCheckConsumer {
		public:
			explicit BlockChainCheckConsumer(
					uint32_t maxChainSize,
					const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder,
					const chain::TimeSupplier& timeSupplier)
					: m_maxChainSize(maxChainSize)
					, m_pConfigHolder(pConfigHolder)
					, m_timeSupplier(timeSupplier)
			{}

		public:
			ConsumerResult operator()(const BlockElements& elements) const {
				if (elements.empty())
					return Abort(Failure_Consumer_Empty_Input);

				if (elements.size() > m_maxChainSize)
					return Abort(Failure_Consumer_Remote_Chain_Too_Many_Blocks);

				if (!isChainTimestampAllowed(elements.back().Block))
					return Abort(Failure_Consumer_Remote_Chain_Too_Far_In_Future);

				utils::HashPointerSet hashes;
				const model::BlockElement* pPreviousElement = nullptr;
				for (const auto& element : elements) {
					// check for a valid chain link
					if (pPreviousElement && !IsLink(*pPreviousElement, element.Block))
						return Abort(Failure_Consumer_Remote_Chain_Improper_Link);

					// check for duplicate transactions
					for (const auto& transactionElement : element.Transactions) {
						if (!hashes.insert(&transactionElement.EntityHash).second)
							return Abort(Failure_Consumer_Remote_Chain_Duplicate_Transactions);
					}

					pPreviousElement = &element;
				}

				return Continue();
			}

		private:
			bool isChainTimestampAllowed(const model::Block& block) const {
				return block.Timestamp <= m_timeSupplier() + m_pConfigHolder->Config(block.Height).Network.MaxBlockFutureTime;
			}

		private:
			uint32_t m_maxChainSize;
			std::shared_ptr<config::BlockchainConfigurationHolder> m_pConfigHolder;
			chain::TimeSupplier m_timeSupplier;
		};
	}

	disruptor::ConstBlockConsumer CreateBlockChainCheckConsumer(
			uint32_t maxChainSize,
			const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder,
			const chain::TimeSupplier& timeSupplier) {
		return BlockChainCheckConsumer(maxChainSize, pConfigHolder, timeSupplier);
	}
}}
