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

#include "BlockChainProcessor.h"
#include "InputUtils.h"
#include "catapult/cache/CatapultCache.h"
#include "catapult/cache/ReadOnlyCatapultCache.h"
#include "catapult/chain/ChainResults.h"
#include "catapult/chain/ChainUtils.h"
#include "catapult/model/BlockUtils.h"
#include "catapult/utils/TimeSpan.h"

using namespace catapult::validators;

namespace catapult { namespace consumers {

	namespace {
		model::WeakEntityInfos ExtractEntityInfos(const model::BlockElement& element) {
			model::WeakEntityInfos entityInfos;
			model::ExtractEntityInfos(element, entityInfos);
			return entityInfos;
		}

		bool IsLinked(const WeakBlockInfo& parentBlockInfo, const BlockElements& elements) {
			return chain::IsChainLink(parentBlockInfo.entity(), parentBlockInfo.hash(), elements[0].Block);
		}

		class DefaultBlockChainProcessor {
		public:
			DefaultBlockChainProcessor(
					const BlockHitPredicateFactory& blockHitPredicateFactory,
					const BlockChainSyncHandlers& handlers)
					: m_blockHitPredicateFactory(blockHitPredicateFactory)
					, m_handlers(handlers)
			{}

		public:
			ValidationResult operator()(
					SyncState& state,
					BlockElements& elements) const {
				if (elements.empty())
					return ValidationResult::Neutral;

				const auto& parentBlockInfo = state.commonBlockInfo();

				if (!IsLinked(parentBlockInfo, elements))
					return chain::Failure_Chain_Unlinked;

				// TODO: ? Pass current and previous cache to hitPredicate, to calculate effective balance
//				auto readOnlyPreviousCache = state.previousCacheDelta().toReadOnly();
				auto readOnlyCurrentCache = state.currentCacheDelta().toReadOnly();
				auto blockHitPredicate = m_blockHitPredicateFactory();

				auto previousTimeStamp = parentBlockInfo.entity().Timestamp;
				const auto* pParentGenerationHash = &parentBlockInfo.generationHash();
				const auto& currentObserverState = state.currentObserverState();
				const auto& preivousObserverState = state.preivousObserverState();
				const auto& storage = state.storage();
				const auto& effectiveBalanceHeight = state.EffectiveBalanceHeight;
				for (auto& element : elements) {
					const auto& block = element.Block;
					element.GenerationHash = model::CalculateGenerationHash(*pParentGenerationHash, block.Signer);
					if (!blockHitPredicate(element.GenerationHash, block.BaseTarget,
							utils::TimeSpan::FromDifference(block.Timestamp, previousTimeStamp), block.EffectiveBalance)) {
						CATAPULT_LOG(warning) << "block " << block.Height << " failed hit";
						return chain::Failure_Chain_Block_Not_Hit;
					}

					auto result = m_handlers.BatchEntityProcessor(block.Height, block.Timestamp, ExtractEntityInfos(element), currentObserverState);
					if (!IsValidationResultSuccess(result)) {
						CATAPULT_LOG(warning) << "batch processing of block " << block.Height << " failed with " << result;
						return result;
					}

					// Restore old block which is EffectiveBalanceHeight blocks below
					if (block.Height > effectiveBalanceHeight) {
						std::shared_ptr<const model::BlockElement> pOldBlockElement;
						const auto& chainHeight = storage.chainHeight();

						auto diff = block.Height - effectiveBalanceHeight;
						if (diff <= chainHeight) {
							pOldBlockElement = storage.loadBlockElement(diff);
						} else {
							diff = diff - chainHeight;
							pOldBlockElement = std::make_shared<const model::BlockElement>(elements[diff.unwrap() - 1]);
						}

						m_handlers.CommitBlock(*pOldBlockElement, preivousObserverState);
					}

					previousTimeStamp = block.Timestamp;
					pParentGenerationHash = &element.GenerationHash;
				}

				return ValidationResult::Success;
			}

		private:
			BlockHitPredicateFactory m_blockHitPredicateFactory;
			const BlockChainSyncHandlers& m_handlers;
		};
	}

	BlockChainProcessor CreateBlockChainProcessor(
			const BlockHitPredicateFactory& blockHitPredicateFactory,
			const BlockChainSyncHandlers& handlers) {
		return DefaultBlockChainProcessor(blockHitPredicateFactory, handlers);
	}
}}
