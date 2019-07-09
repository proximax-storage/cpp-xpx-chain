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

#pragma once
#include "ObserverContext.h"
#include "ObserverTypes.h"
#include "catapult/config_holder/LocalNodeConfigurationHolder.h"

namespace catapult { namespace observers {

	/// Returns \c true if \a context and \a pruneInterval indicate that pruning should be done.
	constexpr bool ShouldPrune(const ObserverContext& context, size_t pruneInterval) {
		return NotifyMode::Commit == context.Mode && 0 == context.Height.unwrap() % pruneInterval;
	}

	/// Returns \c true if \a action and \a notifyMode indicate that a link should be made.
	template<typename TAction>
	constexpr bool ShouldLink(TAction action, NotifyMode notifyMode) {
		return NotifyMode::Commit == notifyMode ? TAction::Link == action : TAction::Unlink == action;
	}

	/// Creates a block-based cache pruning observer with \a name that runs every \a interval blocks
	/// with the specified grace period (\a gracePeriod).
	template<typename TCache>
	NotificationObserverPointerT<model::BlockNotification<1>> CreateCacheBlockPruningObserver(
		const std::string& name,
		size_t interval,
		BlockDuration gracePeriod) {
		using ObserverType = FunctionalNotificationObserverT<model::BlockNotification<1>>;
		return std::make_unique<ObserverType>(name + "PruningObserver", [interval, gracePeriod](const auto&, auto& context) {
			if (!ShouldPrune(context, interval))
				return;

			if (context.Height.unwrap() <= gracePeriod.unwrap())
				return;

			auto pruneHeight = Height(context.Height.unwrap() - gracePeriod.unwrap());
			auto& cache = context.Cache.template sub<TCache>();
			cache.prune(pruneHeight);
		});
	}

	/// Creates a block-based cache pruning observer with \a name that runs every \a interval blocks
	/// with the grace period equals to maximum rollback blocks as specified in (\a config).
	template<typename TCache>
	NotificationObserverPointerT<model::BlockNotification<1>> CreateCacheBlockPruningObserver(
			const std::string& name,
			size_t interval,
			const std::shared_ptr<config::LocalNodeConfigurationHolder>& pConfigHolder) {
		using ObserverType = FunctionalNotificationObserverT<model::BlockNotification<1>>;
		return std::make_unique<ObserverType>(name + "PruningObserver", [interval, pConfigHolder](const auto&, auto& context) {
			if (!ShouldPrune(context, interval))
				return;

			const model::BlockChainConfiguration& config = pConfigHolder->Config(context.Height).BlockChain;
			auto gracePeriod = BlockDuration(config.MaxRollbackBlocks);
			if (context.Height.unwrap() <= gracePeriod.unwrap())
				return;

			auto pruneHeight = Height(context.Height.unwrap() - gracePeriod.unwrap());
			auto& cache = context.Cache.template sub<TCache>();
			cache.prune(pruneHeight);
		});
	}

	/// Creates a block-based cache pruning observer with \a name that runs every BlockPruneInterval
	/// as specified in (\a config).
	template<typename TCache>
	NotificationObserverPointerT<model::BlockNotification<1>> CreateCacheBlockPruningObserver(
			const std::string& name,
			const std::shared_ptr<config::LocalNodeConfigurationHolder>& pConfigHolder) {
		using ObserverType = FunctionalNotificationObserverT<model::BlockNotification<1>>;
		return std::make_unique<ObserverType>(name + "PruningObserver", [pConfigHolder](const auto&, auto& context) {
			const model::BlockChainConfiguration& config = pConfigHolder->Config(context.Height).BlockChain;
			if (!ShouldPrune(context, config.BlockPruneInterval))
				return;

			auto gracePeriod = BlockDuration{};
			if (context.Height.unwrap() <= gracePeriod.unwrap())
				return;

			auto pruneHeight = Height(context.Height.unwrap() - gracePeriod.unwrap());
			auto& cache = context.Cache.template sub<TCache>();
			cache.prune(pruneHeight);
		});
	}

	/// Creates a time-based cache pruning observer with \a name that runs every \a interval blocks.
	template<typename TCache>
	NotificationObserverPointerT<model::BlockNotification<1>> CreateCacheTimePruningObserver(
			const std::string& name,
			const std::shared_ptr<config::LocalNodeConfigurationHolder>& pConfigHolder) {
		using ObserverType = FunctionalNotificationObserverT<model::BlockNotification<1>>;
		return std::make_unique<ObserverType>(name + "PruningObserver", [pConfigHolder](const auto& notification, const auto& context) {
			const model::BlockChainConfiguration& config = pConfigHolder->Config(context.Height).BlockChain;
			if (!ShouldPrune(context, config.BlockPruneInterval))
				return;

			auto& cache = context.Cache.template sub<TCache>();
			cache.prune(notification.Timestamp);
		});
	}

	/// Creates a block-based cache touch observer with \a name that touches the cache at every block height
	/// and creates a receipt of type \a receiptType for all deactivating elements.
	template<typename TCache>
	NotificationObserverPointerT<model::BlockNotification<1>> CreateCacheBlockTouchObserver(
			const std::string& name,
			model::ReceiptType receiptType) {
		using ObserverType = FunctionalNotificationObserverT<model::BlockNotification<1>>;
		return std::make_unique<ObserverType>(name + "TouchObserver", [receiptType](const auto&, auto& context) {
			auto& cache = context.Cache.template sub<TCache>();
			auto expiryIds = cache.touch(context.Height);

			if (NotifyMode::Rollback == context.Mode)
				return;

			// sort expiry ids because receipts must be generated deterministically
			std::set<typename decltype(expiryIds)::value_type> orderedExpiryIds(expiryIds.cbegin(), expiryIds.cend());
			for (auto id : orderedExpiryIds)
				context.StatementBuilder().addReceipt(model::ArtifactExpiryReceipt<decltype(id)>(receiptType, id));
		});
	}
}}
