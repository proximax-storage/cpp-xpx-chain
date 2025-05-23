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
#include "NodeSubscriber.h"
#include "StateChangeSubscriber.h"
#include "TransactionStatusSubscriber.h"
#include "catapult/cache_tx/MemoryPtCache.h"
#include "catapult/cache_tx/MemoryUtCache.h"
#include "catapult/cache_tx/PtChangeSubscriber.h"
#include "catapult/cache_tx/UtChangeSubscriber.h"
#include "catapult/io/BlockChangeSubscriber.h"
#include "catapult/io/FileBlockStorage.h"
#include "catapult/notification_handlers/DemuxHandlerBuilder.h"
#include "catapult/utils/Casting.h"

namespace catapult { namespace config { class BlockchainConfiguration; } }

namespace catapult::model { class TransactionFeeCalculator; };

namespace catapult { namespace subscribers {

	/// A manager for subscribing to notifications.
	class SubscriptionManager {
	public:
		/// Creates a new subscription manager around \a config.
		explicit SubscriptionManager(const config::BlockchainConfiguration& config);

	public:
		/// Gets the underlying file storage.
		io::BlockStorage& fileStorage();

	public:
		/// Registers a block change subscriber (\a pSubscriber).
		void addBlockChangeSubscriber(std::unique_ptr<io::BlockChangeSubscriber>&& pSubscriber);

		/// Registers a post block commit subscriber (\a pSubscriber).
		void addPostBlockCommitSubscriber(std::unique_ptr<io::BlockChangeSubscriber>&& pSubscriber);

		/// Registers an unconfirmed transactions change subscriber (\a pSubscriber).
		void addUtChangeSubscriber(std::unique_ptr<cache::UtChangeSubscriber>&& pSubscriber);

		/// Registers a partial transactions change subscriber (\a pSubscriber).
		void addPtChangeSubscriber(std::unique_ptr<cache::PtChangeSubscriber>&& pSubscriber);

		/// Adds a transaction status subscriber (\a pSubscriber).
		void addTransactionStatusSubscriber(std::unique_ptr<TransactionStatusSubscriber>&& pSubscriber);

		/// Adds a state change subscriber (\a pSubscriber).
		void addStateChangeSubscriber(std::unique_ptr<StateChangeSubscriber>&& pSubscriber);

		/// Adds a node subscriber (\a pSubscriber).
		void addNodeSubscriber(std::unique_ptr<NodeSubscriber>&& pSubscriber);

		/// Adds a notification subscriber.
		template<typename TNotification>
		void addNotificationSubscriber(notification_handlers::DemuxHandlerBuilder::NotificationHandlerPointerT<TNotification>&& pHandler) {
			requireUnused(SubscriberType::Notifications);
			m_notificationSubscriberBuilder.add(std::move(pHandler));
		}

	public:
		/// Creates the block change subscriber.
		std::unique_ptr<io::BlockChangeSubscriber> createBlockChangeSubscriber();

		/// Creates the post block commit subscriber.
		std::unique_ptr<io::BlockChangeSubscriber> createPostBlockCommitSubscriber();

		/// Creates the ut change subscriber.
		std::unique_ptr<cache::UtChangeSubscriber> createUtChangeSubscriber();

		/// Creates the pt change subscriber.
		std::unique_ptr<cache::PtChangeSubscriber> createPtChangeSubscriber();

		/// Creates the transaction status subscriber.
		std::unique_ptr<TransactionStatusSubscriber> createTransactionStatusSubscriber();

		/// Creates the state change subscriber.
		std::unique_ptr<StateChangeSubscriber> createStateChangeSubscriber();

		/// Creates the node subscriber.
		std::unique_ptr<NodeSubscriber> createNodeSubscriber();

		/// Creates the notification subscriber.
		notification_handlers::AggregateNotificationHandlerPointer createNotificationSubscriber();

	public:
		/// Creates the block storage and sets \a pSubscriber to the created block change subscriber.
		/// \note createBlockChangeSubscriber cannot be called if this function is called.
		std::unique_ptr<io::BlockStorage> createBlockStorage(io::BlockChangeSubscriber*& pSubscriber);

		/// Creates the unconfirmed transactions cache with the specified cache \a options.
		/// \note createUtChangeSubscriber cannot be called if this function is called.
		std::unique_ptr<cache::MemoryUtCacheProxy> createUtCache(
				const cache::MemoryCacheOptions& options,
				std::shared_ptr<model::TransactionFeeCalculator> pTransactionFeeCalculator);

		/// Creates the partial transactions cache with the specified cache \a options.
		/// \note createPtChangeSubscriber cannot be called if this function is called.
		std::unique_ptr<cache::MemoryPtCacheProxy> createPtCache(
				const cache::MemoryCacheOptions& options,
				std::shared_ptr<model::TransactionFeeCalculator> pTransactionFeeCalculator);

	private:
		enum class SubscriberType : uint32_t { BlockChange, UtChange, PtChange, TransactionStatus, StateChange, Node, PostBlockCommit, Notifications, Count };

	private:
		void requireUnused(SubscriberType subscriberType) const;

		void markUsed(SubscriberType subscriberType);

	private:
		std::unique_ptr<io::FileBlockStorage> m_pStorage;
		std::array<bool, utils::to_underlying_type(SubscriberType::Count)> m_subscriberUsedFlags;

		std::vector<std::unique_ptr<io::BlockChangeSubscriber>> m_blockChangeSubscribers;
		std::vector<std::unique_ptr<io::BlockChangeSubscriber>> m_postBlockCommitSubscribers;
		std::vector<std::unique_ptr<cache::UtChangeSubscriber>> m_utChangeSubscribers;
		std::vector<std::unique_ptr<cache::PtChangeSubscriber>> m_ptChangeSubscribers;
		std::vector<std::unique_ptr<TransactionStatusSubscriber>> m_transactionStatusSubscribers;
		std::vector<std::unique_ptr<StateChangeSubscriber>> m_stateChangeSubscribers;
		std::vector<std::unique_ptr<NodeSubscriber>> m_nodeSubscribers;
		notification_handlers::DemuxHandlerBuilder m_notificationSubscriberBuilder;
	};
}}
