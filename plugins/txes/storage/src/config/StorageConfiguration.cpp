/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "StorageConfiguration.h"
#include "catapult/utils/ConfigurationUtils.h"

namespace catapult { namespace config {

	StorageConfiguration StorageConfiguration::Uninitialized() {
		return StorageConfiguration();
	}

	StorageConfiguration StorageConfiguration::LoadFromBag(const utils::ConfigurationBag& bag) {
		StorageConfiguration config;

#define LOAD_PROPERTY(NAME) utils::LoadIniProperty(bag, "", #NAME, config.NAME)
		LOAD_PROPERTY(Enabled);
#undef LOAD_PROPERTY

#define TRY_LOAD_CHAIN_PROPERTY(NAME) utils::TryLoadIniProperty(bag, "", #NAME, config.NAME)

		config.MinDriveSize = utils::FileSize::FromMegabytes(1);
		TRY_LOAD_CHAIN_PROPERTY(MinDriveSize);
		config.MaxDriveSize = utils::FileSize::FromTerabytes(10);
		TRY_LOAD_CHAIN_PROPERTY(MaxDriveSize);
		config.MaxModificationSize = utils::FileSize::FromTerabytes(10);
		TRY_LOAD_CHAIN_PROPERTY(MaxModificationSize);
		config.MinReplicatorCount = 1;
		TRY_LOAD_CHAIN_PROPERTY(MinReplicatorCount);
		config.MaxFreeDownloadSize = utils::FileSize::FromMegabytes(1);
		TRY_LOAD_CHAIN_PROPERTY(MaxFreeDownloadSize);
		config.MaxDownloadSize = utils::FileSize::FromTerabytes(10);
		TRY_LOAD_CHAIN_PROPERTY(MaxDownloadSize);
		config.StorageBillingPeriod = utils::TimeSpan::FromHours(24 * 7);
		TRY_LOAD_CHAIN_PROPERTY(StorageBillingPeriod);
		config.DownloadBillingPeriod = utils::TimeSpan::FromHours(24);
		TRY_LOAD_CHAIN_PROPERTY(DownloadBillingPeriod);
		config.VerificationInterval = utils::TimeSpan::FromHours(4);
		TRY_LOAD_CHAIN_PROPERTY(VerificationInterval);
		config.ShardSize = 20;
		TRY_LOAD_CHAIN_PROPERTY(ShardSize);
		config.VerificationExpirationCoefficient = 0.06;
		TRY_LOAD_CHAIN_PROPERTY(VerificationExpirationCoefficient);
		config.VerificationExpirationConstant = 10;
		TRY_LOAD_CHAIN_PROPERTY(VerificationExpirationConstant);

#undef TRY_LOAD_CHAIN_PROPERTY

		return config;
	}
}}
