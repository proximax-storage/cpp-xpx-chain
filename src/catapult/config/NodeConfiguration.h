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
#include "catapult/ionet/ConnectionSecurityMode.h"
#include "catapult/ionet/NodeRoles.h"
#include "catapult/model/TransactionSelectionStrategy.h"
#include "catapult/utils/FileSize.h"
#include "catapult/utils/TimeSpan.h"

namespace catapult { namespace utils { class ConfigurationBag; } }

namespace catapult { namespace config {

	/// Node configuration settings.
	struct NodeConfiguration {
	public:
		/// Server port.
		unsigned short Port;

		/// Server api port.
		unsigned short ApiPort;

		/// \c true if the server should reuse ports already in use.
		bool ShouldAllowAddressReuse;

		/// \c true if a single thread pool should be used, \c false if multiple thread pools should be used.
		bool ShouldUseSingleThreadPool;

		/// \c true if cache data should be saved in a database.
		bool ShouldUseCacheDatabaseStorage;

		/// \c true if temporary sync files should be automatically cleaned up.
		/// \note This should be \c false if broker process is running.
		bool ShouldEnableAutoSyncCleanup;

		/// \c true if transaction spam throttling should be enabled.
		bool ShouldEnableTransactionSpamThrottling;

		/// Maximum fee that will boost a transaction through the spam throttle when spam throttling is enabled.
		Amount TransactionSpamThrottlingMaxBoostFee;

		/// Maximum number of blocks per sync attempt.
		uint32_t MaxBlocksPerSyncAttempt;

		/// Maximum chain bytes per sync attempt.
		utils::FileSize MaxChainBytesPerSyncAttempt;

		/// Duration of a transaction in the short lived cache.
		utils::TimeSpan ShortLivedCacheTransactionDuration;

		/// Duration of a block in the short lived cache.
		utils::TimeSpan ShortLivedCacheBlockDuration;

		/// Time between short lived cache pruning.
		utils::TimeSpan ShortLivedCachePruneInterval;

		/// Maximum size of a short lived cache.
		uint32_t ShortLivedCacheMaxSize;

		/// Minimum fee multiplier of transactions to propagate and include in blocks.
		BlockFeeMultiplier MinFeeMultiplier;

		/// Transaction selection strategy used for syncing and harvesting unconfirmed transactions.
		model::TransactionSelectionStrategy TransactionSelectionStrategy;

		/// Maximum size of an unconfirmed transactions response.
		utils::FileSize UnconfirmedTransactionsCacheMaxResponseSize;

		/// Maximum size of the unconfirmed transactions cache.
		uint32_t UnconfirmedTransactionsCacheMaxSize;

		/// Timeout for connecting to a peer.
		utils::TimeSpan ConnectTimeout;

		/// Timeout for syncing with a peer.
		utils::TimeSpan SyncTimeout;

		/// Initial socket working buffer size (socket reads will attempt to read buffers of roughly this size).
		utils::FileSize SocketWorkingBufferSize;

		/// Socket working buffer sensitivity (lower values will cause memory to be more aggressively reclaimed).
		/// \note \c 0 will disable memory reclamation.
		uint32_t SocketWorkingBufferSensitivity;

		/// Maximum packet data size.
		utils::FileSize MaxPacketDataSize;

		/// Size of the block disruptor circular buffer.
		uint32_t BlockDisruptorSize;

		/// Multiple of elements at which a block element should be traced through queue and completion.
		uint32_t BlockElementTraceInterval;

		/// Size of the transaction disruptor circular buffer.
		uint32_t TransactionDisruptorSize;

		/// Multiple of elements at which a transaction element should be traced through queue and completion.
		uint32_t TransactionElementTraceInterval;

		/// \c true if the process should terminate when any dispatcher is full.
		bool ShouldAbortWhenDispatcherIsFull;

		/// \c true if all dispatcher inputs should be audited.
		bool ShouldAuditDispatcherInputs;

		/// Security mode of outgoing connections initiated by this node.
		ionet::ConnectionSecurityMode OutgoingSecurityMode;

		/// Accepted security modes of incoming connections initiated by other nodes.
		ionet::ConnectionSecurityMode IncomingSecurityModes;

		/// Maximum cache database write batch size.
		utils::FileSize MaxCacheDatabaseWriteBatchSize;

		/// Maximum number of nodes to track in memory.
		uint32_t MaxTrackedNodes;

	public:
		/// Local node configuration.
		struct LocalSubConfiguration {
			/// Node host (leave empty to auto-detect IP).
			std::string Host;

			/// Node friendly name (leave empty to use address).
			std::string FriendlyName;

			/// Node version.
			uint32_t Version;

			/// Node roles.
			ionet::NodeRoles Roles;
		};

	public:
		/// Local node configuration.
		LocalSubConfiguration Local;

	public:
		/// Connections configuration.
		struct ConnectionsSubConfiguration {
			/// Maximum number of active connections.
			uint16_t MaxConnections;

			/// Maximum connection age.
			uint16_t MaxConnectionAge;

			/// Maximum connection ban age.
			uint16_t MaxConnectionBanAge;

			/// Number of consecutive connection failures before a connection is banned.
			uint16_t NumConsecutiveFailuresBeforeBanning;
		};

		/// Incoming connections configuration.
		struct IncomingConnectionsSubConfiguration : public ConnectionsSubConfiguration {
			/// Maximum size of the pending connections queue.
			uint16_t BacklogSize;
		};

	public:
		/// Outgoing connections configuration.
		ConnectionsSubConfiguration OutgoingConnections;

		/// Incoming connections configuration.
		IncomingConnectionsSubConfiguration IncomingConnections;

	private:
		NodeConfiguration() = default;

	public:
		/// Creates an uninitialized node configuration.
		static NodeConfiguration Uninitialized();

	public:
		/// Loads a node configuration from \a bag.
		static NodeConfiguration LoadFromBag(const utils::ConfigurationBag& bag);
	};
}}
