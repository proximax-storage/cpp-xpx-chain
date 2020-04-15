/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "DownloadBaseSets.h"
#include "catapult/config_holder/BlockchainConfigurationHolder.h"
#include "plugins/txes/lock_shared/src/cache/LockInfoCacheDelta.h"
#include "src/config/ServiceConfiguration.h"

namespace catapult { namespace cache {

	/// Basic delta on top of the download cache.
	class BasicDownloadCacheDelta
		: public BasicLockInfoCacheDelta<DownloadCacheDescriptor, DownloadCacheTypes>
		, public LockInfoCacheDeltaMixins<DownloadCacheDescriptor, DownloadCacheTypes>::ConfigBasedEnable<config::ServiceConfiguration> {
	public:
		/// Creates a delta around \a downloadSets and \a pConfigHolder.
		explicit BasicDownloadCacheDelta(
			const DownloadCacheTypes::BaseSetDeltaPointers& downloadSets,
			std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
				: BasicLockInfoCacheDelta<DownloadCacheDescriptor, DownloadCacheTypes>::BasicLockInfoCacheDelta(downloadSets)
				, LockInfoCacheDeltaMixins<DownloadCacheDescriptor, DownloadCacheTypes>::ConfigBasedEnable<config::ServiceConfiguration>(
					pConfigHolder, [](const auto& config) { return config.DownloadCacheEnabled; })
		{}
	};

	/// Delta on top of the download cache.
	class DownloadCacheDelta : public LockInfoCacheDelta<DownloadCacheDescriptor, DownloadCacheTypes, BasicDownloadCacheDelta> {
	public:
		/// Creates a delta around \a downloadSets and \a pConfigHolder.
		explicit DownloadCacheDelta(
			const DownloadCacheTypes::BaseSetDeltaPointers& downloadSets,
			std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
				: LockInfoCacheDelta<DownloadCacheDescriptor, DownloadCacheTypes, BasicDownloadCacheDelta>::LockInfoCacheDelta(downloadSets, pConfigHolder)
		{}
	};
}}
