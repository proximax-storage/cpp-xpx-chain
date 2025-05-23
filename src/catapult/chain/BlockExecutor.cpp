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

#include "BlockExecutor.h"
#include "catapult/cache_core/AccountStateCache.h"

namespace catapult { namespace chain {

	namespace {
		observers::ObserverContext CreateObserverContext(
				const BlockExecutionContext& executionContext,
				Height height,
				const Timestamp& timestamp,
				observers::NotifyMode mode) {
			return observers::ObserverContext(
				executionContext.State,
				executionContext.ConfigHolder->Config(height),
				height,
				timestamp,
				mode,
				executionContext.Resolvers
			);
		}

		void ObserveAll(
				const observers::EntityObserver& observer,
				observers::ObserverContext& context,
				const model::WeakEntityInfos& entityInfos) {
			for (const auto& entityInfo : entityInfos)
				observer.notify(entityInfo, context);
		}
	}

	void ExecuteBlock(const model::BlockElement& blockElement, const BlockExecutionContext& executionContext) {
		model::WeakEntityInfos entityInfos;
		model::ExtractEntityInfos(blockElement, entityInfos);

		executionContext.State.Cache.setHeight(blockElement.Block.Height);
		auto context = CreateObserverContext(executionContext, blockElement.Block.Height, blockElement.Block.Timestamp, observers::NotifyMode::Commit);
		ObserveAll(executionContext.Observer, context, entityInfos);
	}

	void RollbackBlock(const model::BlockElement& blockElement, const BlockExecutionContext& executionContext) {
		model::WeakEntityInfos entityInfos;
		model::ExtractEntityInfos(blockElement, entityInfos);
		std::reverse(entityInfos.begin(), entityInfos.end());

		executionContext.State.Cache.setHeight(blockElement.Block.Height);
		auto context = CreateObserverContext(executionContext, blockElement.Block.Height, blockElement.Block.Timestamp, observers::NotifyMode::Rollback);
		ObserveAll(executionContext.Observer, context, entityInfos);

		// commit removals
		executionContext.State.Cache.sub<cache::AccountStateCache>().commitRemovals();
	}
}}
