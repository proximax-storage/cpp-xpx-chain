/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "DownloadBaseSets.h"
#include "catapult/config_holder/BlockchainConfigurationHolder.h"
#include "plugins/txes/lock_shared/src/cache/LockInfoCacheView.h"

namespace catapult { namespace cache {

	/// Basic view on top of the download cache.
	class BasicDownloadCacheView
		: public BasicLockInfoCacheView<DownloadCacheDescriptor, DownloadCacheTypes>
		, public LockInfoCacheViewMixins<DownloadCacheDescriptor, DownloadCacheTypes>::ConfigBasedEnable<config::ServiceConfiguration> {
	public:
		/// Creates a view around \a downloadSets and \a pConfigHolder.
		explicit BasicDownloadCacheView(
			const DownloadCacheTypes::BaseSets& downloadSets,
			std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
				: BasicLockInfoCacheView<DownloadCacheDescriptor, DownloadCacheTypes>::BasicLockInfoCacheView(downloadSets)
				, LockInfoCacheViewMixins<DownloadCacheDescriptor, DownloadCacheTypes>::ConfigBasedEnable<config::ServiceConfiguration>(
					pConfigHolder, [](const auto& config) { return config.DownloadCacheEnabled; })
		{}
	};

	/// View on top of the download cache.
	class DownloadCacheView : public LockInfoCacheView<DownloadCacheDescriptor, DownloadCacheTypes, BasicDownloadCacheView> {
	public:
		/// Creates a view around \a downloadSets and \a pConfigHolder.
		explicit DownloadCacheView(
			const DownloadCacheTypes::BaseSets& downloadSets,
			std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
				: LockInfoCacheView<DownloadCacheDescriptor, DownloadCacheTypes, BasicDownloadCacheView>::LockInfoCacheView(downloadSets, pConfigHolder)
		{}
	};
}}
