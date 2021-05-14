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
		config.MinReplicatorCount = 1;
		TRY_LOAD_CHAIN_PROPERTY(MinReplicatorCount);
		config.MaxFreeDownloadSize = utils::FileSize::FromMegabytes(1);
		TRY_LOAD_CHAIN_PROPERTY(MaxFreeDownloadSize);
		config.StorageBillingPeriod = utils::TimeSpan::FromHours(24 * 7);
		TRY_LOAD_CHAIN_PROPERTY(StorageBillingPeriod);
		config.DownloadBillingPeriod = utils::TimeSpan::FromHours(24);
		TRY_LOAD_CHAIN_PROPERTY(DownloadBillingPeriod);

#undef TRY_LOAD_CHAIN_PROPERTY

		return config;
	}
}}
