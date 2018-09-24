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

#include "BlockChainSyncConsumer.h"
#include "BlockConsumers.h"
#include "catapult/cache_core/BalanceView.h"
#include "catapult/utils/Casting.h"
#include "ConsumerResultFactory.h"
#include "InputUtils.h"

namespace catapult { namespace consumers {

	namespace {
		using disruptor::InputSource;

		constexpr bool IsAborted(const disruptor::ConsumerResult& result) {
			return disruptor::CompletionStatus::Aborted == result.CompletionStatus;
		}

		struct UnwindResult {
		public:
			consumers::TransactionInfos TransactionInfos;

		public:
			void addBlockTransactionInfos(const std::shared_ptr<const model::BlockElement>& pBlockElement) {
				model::ExtractTransactionInfos(TransactionInfos, pBlockElement);
			}
		};

		class BlockChainSyncConsumer {
		public:
			explicit BlockChainSyncConsumer(
					const extensions::LocalNodeStateRef& localNodeState,
					const BlockChainSyncHandlers& handlers)
					: m_localNodeState(localNodeState)
					, m_handlers(handlers)
			{}

		public:
			ConsumerResult operator()(disruptor::ConsumerInput& input) const {
				return input.empty()
						? Abort(Failure_Consumer_Empty_Input)
						: sync(input.blocks(), input.source());
			}

		private:
			ConsumerResult sync(BlockElements& elements, InputSource source) const {
				// 1. preprocess the peer and local chains and extract the sync state
				SyncState syncState;
				auto intermediateResult = preprocess(elements, source, syncState);
				if (IsAborted(intermediateResult))
					return intermediateResult;

				// 2. validate and execute the peer chain
				intermediateResult = process(elements, syncState);
				if (IsAborted(intermediateResult))
					return intermediateResult;

				// 3. commit all changes
				commitAll(elements, syncState);
				return Continue();
			}

		private:
			ConsumerResult preprocess(const BlockElements& elements, InputSource source, SyncState& syncState) const {
				// 1. check that the peer chain can be linked to the current chain
				auto storageView = m_localNodeState.Storage.view();
				auto peerStartHeight = elements[0].Block.Height;
				auto localChainHeight = storageView.chainHeight();
				if (!IsLinked(peerStartHeight, localChainHeight, source))
					return Abort(Failure_Consumer_Remote_Chain_Unlinked);

				// 2. check that the remote chain is not too far behind the current chain
				auto heightDifference = static_cast<int64_t>((localChainHeight - peerStartHeight).unwrap());
				if (heightDifference > m_localNodeState.Config.BlockChain.MaxRollbackBlocks)
					return Abort(Failure_Consumer_Remote_Chain_Too_Far_Behind);

				// 3. check remote difficulty against difficulty in cache
				if (!m_handlers.DifficultyChecker(elements[elements.size() - 1].Block, storageView.loadBlockElement(localChainHeight)->Block))
					return Abort(Failure_Consumer_Remote_Chain_Difficulty_Not_Better);

				// 4. unwind to the common block height and calculate the local chain score
				syncState = SyncState(m_localNodeState);
				auto commonBlockHeight = peerStartHeight - Height(1);
				auto unwindResult = unwindLocalChain(localChainHeight, commonBlockHeight, syncState);

				auto pCommonBlockElement = storageView.loadBlockElement(commonBlockHeight);

				syncState.update(std::move(pCommonBlockElement), std::move(unwindResult.TransactionInfos));
				return Continue();
			}

			static constexpr bool IsLinked(Height peerStartHeight, Height localChainHeight, InputSource source) {
				// peer should never return nemesis block
				return peerStartHeight >= Height(2)
						// peer chain should connect to local chain
						&& peerStartHeight <= localChainHeight + Height(1)
						// remote pull is allowed to cause (deep) rollback, but other sources
						// are only allowed to rollback the last block
						&& (InputSource::Remote_Pull == source || localChainHeight <= peerStartHeight);
			}

			UnwindResult unwindLocalChain(
					Height localChainHeight,
					Height commonBlockHeight,
					SyncState& state) const {
				UnwindResult result;
				if (localChainHeight == commonBlockHeight)
					return result;

				const auto& currentObserver = state.currentObserverState();
				const auto& preivousObserver = state.preivousObserverState();
				const auto& storage = state.storage();
				auto height = localChainHeight;
				std::shared_ptr<const model::BlockElement> pChildBlockElement;
				while (true) {
					auto pParentBlockElement = storage.loadBlockElement(height);
					if (pChildBlockElement) {
						// add all child block transaction infos to the result
						result.addBlockTransactionInfos(pChildBlockElement);
					}

					if (height == commonBlockHeight)
						break;

					m_handlers.UndoBlock(*pParentBlockElement, currentObserver);
					// Restore cache effectiveBalanceHeight blocks below
					if (height > state.EffectiveBalanceHeight) {
						auto pOldBlockElement = storage.loadBlockElement(height - state.EffectiveBalanceHeight);
						m_handlers.UndoBlock(*pOldBlockElement, preivousObserver);
					}

					pChildBlockElement = std::move(pParentBlockElement);
					height = height - Height(1);
				}

				return result;
			}

			virtual BlockChainProcessor createProcessor() const {
				auto processor = CreateBlockChainProcessor(
						[]() { return chain::BlockHitPredicate{}; },
						m_handlers
				);

				return processor;
			}

			ConsumerResult process(BlockElements& elements, SyncState& syncState) const {
				auto processor = createProcessor();
				auto processResult = processor(syncState, elements);
				if (!validators::IsValidationResultSuccess(processResult)) {
					CATAPULT_LOG(warning) << "processing of peer chain failed with " << processResult;
					return Abort(processResult);
				}

				return Continue();
			}

			void commitAll(const BlockElements& elements, SyncState& syncState) const {
				auto newHeight = elements.back().Block.Height;

				// 1. save the peer chain into storage
				commitToStorage(syncState.commonBlockHeight(), elements);

				// 2. indicate a state change
				m_handlers.StateChange(StateChangeInfo(syncState.currentCacheDelta(), newHeight));

				// 3. commit changes to the in-memory cache
				syncState.commit(newHeight);

				// 4. update the unconfirmed transactions
				auto peerTransactionHashes = ExtractTransactionHashes(elements);
				auto revertedTransactionInfos = CollectRevertedTransactionInfos(
						peerTransactionHashes,
						syncState.detachRemovedTransactionInfos());
				m_handlers.TransactionsChange({ peerTransactionHashes, revertedTransactionInfos });
			}

			void commitToStorage(Height commonBlockHeight, const BlockElements& elements) const {
				auto storageModifier = m_localNodeState.Storage.modifier();
				storageModifier.dropBlocksAfter(commonBlockHeight);
				storageModifier.saveBlocks(elements);
			}

		private:
			const extensions::LocalNodeStateRef& m_localNodeState;
			BlockChainSyncHandlers m_handlers;
		};

		class MockBlockChainSyncConsumer : public BlockChainSyncConsumer {

		public:
			explicit MockBlockChainSyncConsumer(
					const extensions::LocalNodeStateRef& localNodeState,
					const BlockChainSyncHandlers& handlers,
					const BlockChainProcessor& processor)
					: BlockChainSyncConsumer(localNodeState, handlers)
					, Processor(processor)
			{}

		private:

			virtual BlockChainProcessor createProcessor() const override {
				return Processor;
			}

		private:
			BlockChainProcessor Processor;
		};
	}

	disruptor::DisruptorConsumer CreateBlockChainSyncConsumer(
			const extensions::LocalNodeStateRef& localNodeState,
			const BlockChainSyncHandlers& handlers) {
		return BlockChainSyncConsumer(localNodeState, handlers);
	}

	disruptor::DisruptorConsumer CreateMockBlockChainSyncConsumer(
			const extensions::LocalNodeStateRef& localNodeState,
			const BlockChainSyncHandlers& handlers,
			const BlockChainProcessor& processor) {
		return MockBlockChainSyncConsumer(localNodeState, handlers, processor);
	}
}}
