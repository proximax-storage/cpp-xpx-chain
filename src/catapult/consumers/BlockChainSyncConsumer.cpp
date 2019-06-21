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
#include "InputUtils.h"
#include "catapult/cache/CatapultCache.h"
#include "catapult/chain/BlockScorer.h"
#include "catapult/chain/ChainUtils.h"
#include "catapult/io/BlockStorageCache.h"
#include "catapult/model/BlockUtils.h"
#include "catapult/utils/Casting.h"
#include "catapult/utils/StackLogger.h"

namespace catapult { namespace consumers {

	namespace {
		using disruptor::InputSource;

		constexpr bool IsAborted(const disruptor::ConsumerResult& result) {
			return disruptor::CompletionStatus::Aborted == result.CompletionStatus;
		}

		struct UnwindResult {
		public:
			model::ChainScore Score;
			consumers::TransactionInfos TransactionInfos;

		public:
			void addBlockTransactionInfos(const std::shared_ptr<const model::BlockElement>& pBlockElement) {
				model::ExtractTransactionInfos(TransactionInfos, pBlockElement);
			}
		};

		struct SyncState {
		public:
			SyncState() = default;

			SyncState(cache::CatapultCache& cache, state::CatapultState& state)
					: m_pOriginalCache(&cache)
					, m_pOriginalState(&state)
					, m_pCacheDelta(std::make_unique<cache::CatapultCacheDelta>(cache.createDelta()))
					, m_stateCopy(state)
			{}

		public:
			WeakBlockInfo commonBlockInfo() const {
				return WeakBlockInfo(*m_pCommonBlockElement);
			}

			Height commonBlockHeight() const {
				return m_pCommonBlockElement->Block.Height;
			}

			const model::ChainScore& scoreDelta() const {
				return m_scoreDelta;
			}

			const cache::CatapultCacheDelta& cacheDelta() const {
				return *m_pCacheDelta;
			}

			const state::CatapultState& state() const {
				return m_stateCopy;
			}

		public:
			TransactionInfos detachRemovedTransactionInfos() {
				return std::move(m_removedTransactionInfos);
			}

			observers::ObserverState observerState() {
				return observers::ObserverState(*m_pCacheDelta, m_stateCopy);
			}

			void update(
					std::shared_ptr<const model::BlockElement>&& pCommonBlockElement,
					model::ChainScore&& scoreDelta,
					TransactionInfos&& removedTransactionInfos) {
				m_pCommonBlockElement = std::move(pCommonBlockElement);
				m_scoreDelta = std::move(scoreDelta);
				m_removedTransactionInfos = std::move(removedTransactionInfos);
			}

			void commit(Height height) {
				m_pOriginalCache->commit(height);
				m_pCacheDelta.reset(); // release the delta after commit so that the UT updater can acquire a lock

				*m_pOriginalState = m_stateCopy;
			}

		private:
			cache::CatapultCache* m_pOriginalCache;
			state::CatapultState* m_pOriginalState;
			std::unique_ptr<cache::CatapultCacheDelta> m_pCacheDelta; // unique_ptr to allow explicit release of lock in commit
			state::CatapultState m_stateCopy;
			std::shared_ptr<const model::BlockElement> m_pCommonBlockElement;
			model::ChainScore m_scoreDelta;
			TransactionInfos m_removedTransactionInfos;
		};

		class BlockChainSyncConsumer {
		public:
			BlockChainSyncConsumer(
					cache::CatapultCache& cache,
					state::CatapultState& state,
					io::BlockStorageCache& storage,
					uint32_t maxRollbackBlocks,
					const BlockChainSyncHandlers& handlers)
					: m_cache(cache)
					, m_state(state)
					, m_storage(storage)
					, m_maxRollbackBlocks(maxRollbackBlocks)
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
				auto storageView = m_storage.view();
				auto peerStartHeight = elements[0].Block.Height;
				auto localChainHeight = storageView.chainHeight();
				if (!IsLinked(peerStartHeight, localChainHeight, source))
					return Abort(Failure_Consumer_Remote_Chain_Unlinked);

				// 2. check that the remote chain is not too far behind the current chain
				auto heightDifference = static_cast<int64_t>((localChainHeight - peerStartHeight).unwrap());
				if (heightDifference > m_maxRollbackBlocks)
					return Abort(Failure_Consumer_Remote_Chain_Too_Far_Behind);

				// 3. check difficulties against difficulties in cache
				auto blocks = ExtractBlocks(elements);
				if (!m_handlers.DifficultyChecker(blocks, m_cache))
					return Abort(Failure_Consumer_Remote_Chain_Mismatched_Difficulties);

				// 4. unwind to the common block height and calculate the local chain score
				syncState = SyncState(m_cache, m_state);
				auto commonBlockHeight = peerStartHeight - Height(1);
				auto observerState = syncState.observerState();
				auto unwindResult = unwindLocalChain(localChainHeight, commonBlockHeight, storageView, observerState);
				const auto& localScore = unwindResult.Score;

				// 5. calculate the remote chain score
				auto pCommonBlockElement = storageView.loadBlockElement(commonBlockHeight);
				auto peerScore = chain::CalculatePartialChainScore(pCommonBlockElement->Block, blocks);

				// 6. do not accept a chain with the same score because two different blocks with the same height
				//    that are pushed to the network could result in indefinite switching and lots of i/o
				if (peerScore <= localScore) {
					CATAPULT_LOG(warning) << "peer score (" << peerScore << ") is not better than local score (" << localScore << ")";
					return Abort(Failure_Consumer_Remote_Chain_Score_Not_Better);
				}

				peerScore -= localScore; // calculate the score delta
				syncState.update(std::move(pCommonBlockElement), std::move(peerScore), std::move(unwindResult.TransactionInfos));
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
					const io::BlockStorageView& storage,
					observers::ObserverState& observerState) const {
				UnwindResult result;
				if (localChainHeight == commonBlockHeight)
					return result;

				auto height = localChainHeight;
				std::shared_ptr<const model::BlockElement> pChildBlockElement;
				while (true) {
					auto pParentBlockElement = storage.loadBlockElement(height);
					if (pChildBlockElement) {
						result.Score += model::ChainScore(chain::CalculateScore(pParentBlockElement->Block, pChildBlockElement->Block));

						// add all child block transaction infos to the result
						result.addBlockTransactionInfos(pChildBlockElement);
					}

					if (height == commonBlockHeight) {
						m_handlers.UndoBlock(*pParentBlockElement, observerState, UndoBlockType::Common);
						break;
					}

					m_handlers.UndoBlock(*pParentBlockElement, observerState, UndoBlockType::Rollback);
					pChildBlockElement = std::move(pParentBlockElement);
					height = height - Height(1);
				}

				return result;
			}

			ConsumerResult process(BlockElements& elements, SyncState& syncState) const {
				auto observerState = syncState.observerState();
				auto processResult = m_handlers.Processor(syncState.commonBlockInfo(), elements, observerState);
				if (!validators::IsValidationResultSuccess(processResult)) {
					CATAPULT_LOG(warning) << "processing of peer chain failed with " << processResult;
					return Abort(processResult);
				}

				return Continue();
			}

			void commitAll(const BlockElements& elements, SyncState& syncState) const {
				utils::SlowOperationLogger logger("BlockChainSyncConsumer::commitAll", utils::LogLevel::Warning);

				// 1. save the peer chain into storage
				auto storageModifier = m_storage.modifier();
				storageModifier.dropBlocksAfter(syncState.commonBlockHeight());
				storageModifier.saveBlocks(elements);
				m_handlers.CommitStep(CommitOperationStep::Blocks_Written);

				// 2. indicate a state change
				auto newHeight = elements.back().Block.Height;
				m_handlers.StateChange({ cache::CacheChanges(syncState.cacheDelta()), syncState.scoreDelta(), newHeight });
				m_handlers.PreStateWritten(syncState.cacheDelta(), syncState.state(), newHeight);
				m_handlers.CommitStep(CommitOperationStep::State_Written);

				// *** checkpoint ***
				// - both blocks and state have been written out to disk and can be fully restored
				// - broker process is not yet able to consume changes (all changes are consumable after step 3)

				// 3. commit changes to the in-memory cache and primary block chain storage
				syncState.commit(newHeight);
				storageModifier.commit();
				m_handlers.CommitStep(CommitOperationStep::All_Updated);

				// 4. update the unconfirmed transactions
				auto peerTransactionHashes = ExtractTransactionHashes(elements);
				auto revertedTransactionInfos = CollectRevertedTransactionInfos(
						peerTransactionHashes,
						syncState.detachRemovedTransactionInfos());
				m_handlers.TransactionsChange({ peerTransactionHashes, revertedTransactionInfos });
			}

		private:
			cache::CatapultCache& m_cache;
			state::CatapultState& m_state;
			io::BlockStorageCache& m_storage;
			uint32_t m_maxRollbackBlocks;
			BlockChainSyncHandlers m_handlers;
		};
	}

	disruptor::DisruptorConsumer CreateBlockChainSyncConsumer(
			cache::CatapultCache& cache,
			state::CatapultState& state,
			io::BlockStorageCache& storage,
			uint32_t maxRollbackBlocks,
			const BlockChainSyncHandlers& handlers) {
		return BlockChainSyncConsumer(cache, state, storage, maxRollbackBlocks, handlers);
	}
}}
