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

namespace catapult { namespace contract {

	/// Executor configuration settings.
	struct ExecutorConfiguration {
	public:
		/// Replicator key.
		std::string Key;

		/// Timeout for the transactions sent by the replicator.
		utils::TimeSpan TransactionTimeout;

		std::string StorageRPCAddress;

		std::string MessengerRPCAddress;

		std::string VirtualMachineRPCAddress;

	private:
		ExecutorConfiguration() = default;

	public:
		/// Creates an uninitialized contract configuration.
		static ExecutorConfiguration Uninitialized();

	public:
		/// Loads a contract configuration from \a bag.
		static ExecutorConfiguration LoadFromBag(const utils::ConfigurationBag& bag);

		/// Loads a contract configuration from \a resourcesPath.
		static ExecutorConfiguration LoadFromPath(const boost::filesystem::path& resourcesPath);
	};
}}
