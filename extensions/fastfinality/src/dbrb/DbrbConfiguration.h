/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/dbrb/DbrbDefinitions.h"
#include "catapult/utils/TimeSpan.h"
#include <boost/filesystem/path.hpp>
#include <set>

namespace catapult { namespace utils { class ConfigurationBag; } }

namespace catapult { namespace dbrb {

	/// Storage configuration settings.
	struct DbrbConfiguration {
	public:
		/// Timeout for the transactions sent by the DBRB process.
		utils::TimeSpan TransactionTimeout;

		/// True if this node is a DBRB process.
		bool IsDbrbProcess;

	private:
		DbrbConfiguration() = default;

	public:
		/// Creates an uninitialized storage configuration.
		static DbrbConfiguration Uninitialized();

	public:
		/// Loads a storage configuration from \a bag.
		static DbrbConfiguration LoadFromBag(const utils::ConfigurationBag& bag);

		/// Loads a storage configuration from \a resourcesPath.
		static DbrbConfiguration LoadFromPath(const boost::filesystem::path& resourcesPath);
	};
}}
