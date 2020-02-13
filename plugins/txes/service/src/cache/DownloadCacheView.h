/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "DownloadBaseSets.h"
#include "ServiceCacheTools.h"
#include "catapult/config_holder/BlockchainConfigurationHolder.h"
#include "plugins/txes/lock_shared/src/cache/LockInfoCacheView.h"

namespace catapult { namespace cache {

	/// Basic view on top of the download cache.
	class BasicDownloadCacheView : public BasicLockInfoCacheView<DownloadCacheDescriptor, DownloadCacheTypes> {
	public:
		/// Creates a view around \a downloadSets and \a pConfigHolder.
		explicit BasicDownloadCacheView(
			const DownloadCacheTypes::BaseSets& downloadSets,
			std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
				: BasicLockInfoCacheView<DownloadCacheDescriptor, DownloadCacheTypes>::BasicLockInfoCacheView(downloadSets)
				, m_pConfigHolder(pConfigHolder)
		{}

	public:
		bool enabled() const {
			return DownloadCacheEnabled(m_pConfigHolder, height());
		}

	private:
		std::shared_ptr<config::BlockchainConfigurationHolder> m_pConfigHolder;
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
