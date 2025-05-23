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

#include "ChainSynchronizer.h"
#include "CompareChains.h"
#include "catapult/api/RemoteChainApi.h"
#include "catapult/extensions/ServiceState.h"
#include <queue>

namespace catapult { namespace chain {

	namespace {
		using NodeInteractionFuture = thread::future<ionet::NodeInteractionResultCode>;

		struct ElementInfo {
			disruptor::DisruptorElementId Id;
			Height EndHeight;
			size_t NumBytes;
		};

		class UnprocessedElements : public std::enable_shared_from_this<UnprocessedElements> {
		public:
			explicit UnprocessedElements(const CompletionAwareBlockRangeConsumerFunc& blockRangeConsumer, size_t maxSize)
					: m_blockRangeConsumer(blockRangeConsumer)
					, m_maxSize(maxSize)
					, m_numBytes(0)
					, m_hasPendingSync(false)
					, m_dirty(false)
			{}

		public:
			bool empty() {
				return 0 == numBytes();
			}

			size_t numBytes() {
				std::shared_lock lock(m_mutex);
				return m_numBytes;
			}

			bool shouldStartSync() {
				std::unique_lock lock(m_mutex);
				if (m_numBytes >= m_maxSize || m_hasPendingSync || m_dirty)
					return false;

				m_hasPendingSync = true;
				return true;
			}

			Height maxHeight() {
				std::shared_lock lock(m_mutex);
				return m_elements.empty() ? Height(0) : m_elements.back().EndHeight;
			}

			bool add(model::AnnotatedBlockRange&& range) {
				std::unique_lock lock(m_mutex);
				if (m_dirty)
					return false;

				auto endHeight = (--range.Range.cend())->Height;
				auto bufferSize = range.Range.totalSize();

				// need to use shared_from_this because dispatcher can finish processing a block after
				// scheduler is stopped (and owning DefaultChainSynchronizer is destroyed)
				auto newId = m_blockRangeConsumer(std::move(range), [pThis = shared_from_this()](auto id, auto result) {
					pThis->remove(id, result.CompletionStatus);
				});

				// if the disruptor is full, abort processing
				if (0 == newId)
					return false;

				auto info = ElementInfo{ newId, endHeight, bufferSize };
				m_numBytes += info.NumBytes;
				m_elements.emplace(info);
				return true;
			}

			void remove(disruptor::DisruptorElementId id, disruptor::CompletionStatus status) {
				std::unique_lock lock(m_mutex);
				const auto& info = m_elements.front();
				if (info.Id != id)
					CATAPULT_THROW_INVALID_ARGUMENT_1("unexpected element id", id);

				m_numBytes -= info.NumBytes;
				m_elements.pop();
				m_dirty = hasPendingOperation() && disruptor::CompletionStatus::Normal != status;
			}

			void clearPendingSync() {
				std::unique_lock lock(m_mutex);
				m_hasPendingSync = false;

				if (m_dirty)
					m_dirty = hasPendingOperation();
			}

		private:
			bool hasPendingOperation() const {
				return 0 != m_numBytes || m_hasPendingSync;
			}

		private:
			std::shared_mutex m_mutex;
			CompletionAwareBlockRangeConsumerFunc m_blockRangeConsumer;
			std::queue<ElementInfo> m_elements;
			size_t m_maxSize;
			size_t m_numBytes;
			bool m_hasPendingSync;
			bool m_dirty;
		};

		ionet::NodeInteractionResultCode ToNodeInteractionResultCode(ChainComparisonCode code) {
			// notice that this function is only called when code is not Remote_Is_Not_Synced
			return IsRemoteOutOfSync(code) || IsRemoteEvil(code)
					? ionet::NodeInteractionResultCode::Failure
					: ionet::NodeInteractionResultCode::Neutral;
		}

		class RangeAggregator {
		public:
			explicit RangeAggregator(const Key& sourcePublicKey)
					: m_numBlocks(0)
					, m_sourcePublicKey(sourcePublicKey)
			{}

		public:
			void add(model::BlockRange&& range) {
				m_numBlocks += range.size();
				m_ranges.push_back(std::move(range));
			}

			auto merge() {
				return model::AnnotatedBlockRange(model::BlockRange::MergeRanges(std::move(m_ranges)), m_sourcePublicKey);
			}

			auto empty() {
				return 0 == m_numBlocks;
			}

			auto numBlocks() const {
				return m_numBlocks;
			}

		private:
			size_t m_numBlocks;
			Key m_sourcePublicKey;
			std::vector<model::BlockRange> m_ranges;
		};

		auto CreateFutureSupplier(const api::RemoteChainApi& remoteChainApi, const api::BlocksFromOptions& options) {
			return [&remoteChainApi, options](auto height) {
				return remoteChainApi.blocksFrom(height, options);
			};
		}

		NodeInteractionFuture CompleteChainBlocksFrom(RangeAggregator& rangeAggregator, UnprocessedElements& unprocessedElements) {
			if (rangeAggregator.empty())
				return thread::make_ready_future(ionet::NodeInteractionResultCode::Neutral);

			auto mergedRange = rangeAggregator.merge();
			auto addResult = unprocessedElements.add(std::move(mergedRange))
					? ionet::NodeInteractionResultCode::Success
					: ionet::NodeInteractionResultCode::Neutral;
			return thread::make_ready_future(std::move(addResult));
		}

		NodeInteractionFuture ChainBlocksFrom(
				const std::function<thread::future<model::BlockRange>(Height)>& futureSupplier,
				Height height,
				uint64_t forkDepth,
				const std::shared_ptr<RangeAggregator>& pRangeAggregator,
				UnprocessedElements& unprocessedElements) {
			return thread::compose(futureSupplier(height), [futureSupplier, forkDepth, pRangeAggregator, &unprocessedElements](
					auto&& blocksFuture) {
				try {
					auto range = blocksFuture.get();

					// if the range is empty, stop processing
					if (range.empty()) {
						CATAPULT_LOG(info) << "peer returned 0 blocks";
						return CompleteChainBlocksFrom(*pRangeAggregator, unprocessedElements);
					}

					// if the range is not empty, continue processing
					auto endHeight = (--range.cend())->Height;
					CATAPULT_LOG(info)
							<< "peer returned " << range.size()
							<< " blocks (heights " << range.cbegin()->Height << " - " << endHeight << ")";

					pRangeAggregator->add(std::move(range));
					if (forkDepth <= pRangeAggregator->numBlocks())
						return CompleteChainBlocksFrom(*pRangeAggregator, unprocessedElements);

					auto nextHeight = endHeight + Height(1);
					return ChainBlocksFrom(futureSupplier, nextHeight, forkDepth, pRangeAggregator, unprocessedElements);
				} catch (const catapult_runtime_error& e) {
					CATAPULT_LOG(warning) << "exception thrown while requesting blocks: " << e.what();
					return thread::make_ready_future(ionet::NodeInteractionResultCode::Failure);
				}
			});
		}

		class DefaultChainSynchronizer {
		public:
			using RemoteApiType = api::RemoteChainApi;

		public:
			// note: the synchronizer will only request config.MaxRollbackBlocks blocks so that even if the peer returns
			//       a chain part that is a fork of the real chain, that fork is still resolvable because it can be rolled back
			explicit DefaultChainSynchronizer(
					const std::shared_ptr<const api::ChainApi>& pLocalChainApi,
					const ChainSynchronizerConfiguration& config,
					extensions::ServiceState& state,
					const CompletionAwareBlockRangeConsumerFunc& blockRangeConsumer)
					: m_pLocalChainApi(pLocalChainApi)
					, m_state(state)
					, m_blocksFromOptions(config.MaxBlocksPerSyncAttempt, config.MaxChainBytesPerSyncAttempt)
					, m_pUnprocessedElements(std::make_shared<UnprocessedElements>(
							blockRangeConsumer,
							3 * config.MaxChainBytesPerSyncAttempt))
			{}

		public:
			NodeInteractionFuture operator()(const RemoteApiType& remoteChainApi) {
				if (!m_pUnprocessedElements->shouldStartSync())
					return thread::make_ready_future(ionet::NodeInteractionResultCode::Neutral);

				auto syncFuture = thread::compose(compareChains(remoteChainApi), [this, &remoteChainApi](auto&& compareChainsFuture) {
					try {
						return this->syncWithPeer(remoteChainApi, compareChainsFuture.get());
					} catch (const catapult_runtime_error& e) {
						CATAPULT_LOG(warning) << "exception thrown while comparing chains: " << e.what();
						return thread::make_ready_future(ionet::NodeInteractionResultCode::Failure);
					}
				});
				return thread::compose(std::move(syncFuture), [&unprocessedElements = *m_pUnprocessedElements](
						auto&& nodeInteractionFuture) {
					// mark the current sync as completed
					unprocessedElements.clearPendingSync();
					return std::move(nodeInteractionFuture);
				});
			}

		private:
			// in case that there are no unprocessed elements in the disruptor, we do a normal synchronization round
			// else we bypass chain comparison and expand the existing chain part by pulling more blocks
			thread::future<CompareChainsResult> compareChains(const RemoteApiType& remoteChainApi) {
				const auto& config = m_state.config();
				if (m_pUnprocessedElements->empty())
					return CompareChains(*m_pLocalChainApi, remoteChainApi, CompareChainsOptions(config.Node.MaxBlocksPerSyncAttempt, config.Network.MaxRollbackBlocks));

				CompareChainsResult result;
				result.Code = ChainComparisonCode::Remote_Is_Not_Synced;
				result.CommonBlockHeight = m_pUnprocessedElements->maxHeight();
				result.ForkDepth = 0;
				return thread::make_ready_future(std::move(result));
			}

			NodeInteractionFuture syncWithPeer(const RemoteApiType& remoteChainApi, const CompareChainsResult& compareResult) const {
				switch (compareResult.Code) {
				case ChainComparisonCode::Remote_Is_Not_Synced:
					break;

				default:
					auto code = ToNodeInteractionResultCode(compareResult.Code);
					if (ionet::NodeInteractionResultCode::Failure == code)
						CATAPULT_LOG(warning) << "node interaction failed: " << compareResult.Code;

					return thread::make_ready_future(std::move(code));
				}

				CATAPULT_LOG(debug)
						<< "pulling blocks from remote with common height " << compareResult.CommonBlockHeight
						<< " (fork depth = " << compareResult.ForkDepth << ")";
				return ChainBlocksFrom(
						CreateFutureSupplier(remoteChainApi, m_blocksFromOptions),
						compareResult.CommonBlockHeight + Height(1),
						compareResult.ForkDepth,
						std::make_shared<RangeAggregator>(remoteChainApi.remotePublicKey()),
						*m_pUnprocessedElements);
			}

		private:
			std::shared_ptr<const api::ChainApi> m_pLocalChainApi;
			extensions::ServiceState& m_state;
			api::BlocksFromOptions m_blocksFromOptions;
			std::shared_ptr<UnprocessedElements> m_pUnprocessedElements;
		};
	}

	RemoteNodeSynchronizer<api::RemoteChainApi> CreateChainSynchronizer(
			const std::shared_ptr<const api::ChainApi>& pLocalChainApi,
			const ChainSynchronizerConfiguration& config,
			extensions::ServiceState& state,
			const CompletionAwareBlockRangeConsumerFunc& blockRangeConsumer) {
		auto pSynchronizer = std::make_shared<DefaultChainSynchronizer>(pLocalChainApi, config, state, blockRangeConsumer);
		return CreateRemoteNodeSynchronizer(pSynchronizer);
	}
}}
