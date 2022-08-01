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
		/// Replicator host.
		std::string Host;

		/// Replicator port.
		std::string Port;

		/// Timeout for the transactions sent by the replicator.
		utils::TimeSpan TransactionTimeout;

		/// Storage directory.
		std::string StorageDirectory;

		/// Storage sandbox directory.
		std::string SandboxDirectory;

		/// Use TCP socket.
		bool UseTcpSocket;

//		// Use rpc for connection to Replicator
//		bool UseRPC = true;

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
