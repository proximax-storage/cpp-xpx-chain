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
		utils::FileSize MinDriveSize;

		/// Maximal size of the drive.
		utils::FileSize MaxDriveSize;

		/// Minimal capacity of the replicator.
		utils::FileSize MinCapacity;

		/// Maximal size of the modification (for single payment).
		utils::FileSize MaxModificationSize;

		/// Minimal number of replicators.
		uint16_t MinReplicatorCount;

		/// Maximal number of replicators
		uint16_t MaxReplicatorCount;

		/// Max size to download files for free
		utils::FileSize MaxFreeDownloadSize;

		/// Maximal size of the download (for single payment).
		utils::FileSize MaxDownloadSize;

		/// Storage billing period.
		utils::TimeSpan StorageBillingPeriod;

		/// Download billing period.
		utils::TimeSpan DownloadBillingPeriod;

		/// Whether the plugin is enabled.
		bool Enabled;

		/// Expected Verification frequency.
		utils::TimeSpan VerificationInterval;

		/// The number of replicators in shard.
		uint8_t ShardSize;

		/// The verification expiration coefficient.
		double VerificationExpirationCoefficient;

		/// The verification expiration constant.
		double VerificationExpirationConstant;

	private:
		StorageConfiguration() = default;

	public:
		/// Creates an uninitialized storage configuration.
		static StorageConfiguration Uninitialized();

		/// Loads an storage configuration from \a bag.
		static StorageConfiguration LoadFromBag(const utils::ConfigurationBag& bag);
	};
}}
