/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "BlockConsumers.h"
#include "ConsumerResultFactory.h"
#include "ConsumerUtils.h"
#include "catapult/cache/CatapultCache.h"

namespace catapult { namespace consumers {

	namespace {
		class BlockValidatorConsumer {
		public:
			explicit BlockValidatorConsumer(
					cache::CatapultCache& cache,
					state::CatapultState& state,
					const BlockChainSyncHandlers& handlers,
					const model::BlockElementSupplier& lastBlockElementSupplier)
				: m_cache(cache)
				, m_state(state)
				, m_handlers(handlers)
				, m_lastBlockElementSupplier(lastBlockElementSupplier)
			{}

		public:
			ConsumerResult operator()(BlockElements& elements) const {
				if (elements.empty())
					return Abort(Failure_Consumer_Empty_Input);

				if (elements.size() > 1)
					return Abort(Failure_Consumer_Remote_Chain_Too_Many_Blocks);

				model::NetworkConfigurations remoteConfigs;
				if (!ExtractConfigs(elements, remoteConfigs))
					return Abort(Failure_Consumer_Remote_Network_Config_Malformed);

				auto blocks = ExtractBlocks(elements);
				if (!m_handlers.DifficultyChecker(blocks, m_cache, remoteConfigs))
					return Abort(Failure_Consumer_Remote_Chain_Mismatched_Difficulties);

				auto cacheDetachableDelta = m_cache.createDetachableDelta(Height(1));
				auto cacheDetachedDelta = cacheDetachableDelta.detach();
				auto pCacheDelta = cacheDetachedDelta.tryLock();

				state::CatapultState stateCopy(m_state);
				std::vector<std::unique_ptr<model::Notification>> notifications;
				observers::ObserverState observerState(*pCacheDelta, stateCopy, notifications);

				auto pLastBlockElement = m_lastBlockElementSupplier();
				auto processResult = m_handlers.Processor(WeakBlockInfo(*pLastBlockElement), elements, observerState);
				if (!validators::IsValidationResultSuccess(processResult)) {
					CATAPULT_LOG(warning) << "proposed block processing failed with " << processResult;
					return Abort(processResult);
				}

				return CompleteSuccess();
			}

		private:
			cache::CatapultCache& m_cache;
			state::CatapultState& m_state;
			BlockChainSyncHandlers m_handlers;
			model::BlockElementSupplier m_lastBlockElementSupplier;
		};
	}

	disruptor::BlockConsumer CreateBlockValidatorConsumer(
			cache::CatapultCache& cache,
			state::CatapultState& state,
			const BlockChainSyncHandlers& handlers,
			const model::BlockElementSupplier& lastBlockElementSupplier) {
		return BlockValidatorConsumer(cache, state, handlers, lastBlockElementSupplier);
	}
}}
