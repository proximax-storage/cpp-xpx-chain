/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include <catapult/utils/FileSize.h>
#include <catapult/utils/TimeSpan.h>
#include "catapult/model/PluginConfiguration.h"
#include "catapult/types.h"

namespace catapult { namespace utils { class ConfigurationBag; } }

namespace catapult { namespace config {

	/// Storage plugin configuration settings.
	struct StorageConfiguration : public model::PluginConfiguration {
	public:
		DEFINE_CONFIG_CONSTANTS(storage)

		/// Minimal size of the drive.
		utils::FileSize MinDriveSize{};

		/// Minimal number of replicators.
		uint16_t MinReplicatorCount{};

		/// Max size to download files for free
		utils::FileSize MaxFreeDownloadSize{};

		/// Storage billing period.
		utils::TimeSpan StorageBillingPeriod;

		/// Download billing period.
		utils::TimeSpan DownloadBillingPeriod;

		/// Whether the plugin is enabled.
		bool Enabled;

	private:
		StorageConfiguration() = default;

	public:
		/// Creates an uninitialized storage configuration.
		static StorageConfiguration Uninitialized();

		/// Loads an storage configuration from \a bag.
		static StorageConfiguration LoadFromBag(const utils::ConfigurationBag& bag);
	};
}}
