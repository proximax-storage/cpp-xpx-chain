/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/utils/TimeSpan.h"
#include <boost/filesystem/path.hpp>
#include <string>
#include <unordered_set>

namespace catapult { namespace utils { class ConfigurationBag; } }

namespace catapult { namespace storage {

	/// Storage configuration settings.
	struct StorageConfiguration {
	public:
		/// Replicator key.
		std::string Key;

		/// Replicator host.
		std::string Host;

		/// Replicator port.
		std::string Port;

		/// Replicator websocket port.
		std::string WsPort;

		/// Timeout for the transactions sent by the replicator.
		utils::TimeSpan TransactionTimeout;

		/// Storage directory.
		std::string StorageDirectory;

		/// Use TCP socket.
		bool UseTcpSocket;

		/// Log options
		std::string LogOptions;

		// Use RPC to connect to Replicator
		bool UseRpcReplicator;

		/// Replicator host.
		std::string RpcHost;

		/// Replicator port.
		std::string RpcPort;

		// Whether node does not mind the replicator crash
		bool RpcHandleLostConnection;

		// Whether the replicator can be crashed by the outside command
		bool RpcDbgChildCrash;

	private:
		StorageConfiguration() = default;

	public:
		/// Creates an uninitialized storage configuration.
		static StorageConfiguration Uninitialized();

	public:
		/// Loads a storage configuration from \a bag.
		static StorageConfiguration LoadFromBag(const utils::ConfigurationBag& bag);

		/// Loads a storage configuration from \a resourcesPath.
		static StorageConfiguration LoadFromPath(const boost::filesystem::path& resourcesPath);
	};
}}
