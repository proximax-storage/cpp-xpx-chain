/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "OperationBaseSets.h"
#include "OperationCacheSerializers.h"
#include "catapult/config_holder/BlockchainConfigurationHolder.h"
#include "plugins/txes/lock_shared/src/cache/LockInfoCacheDelta.h"
#include "src/config/OperationConfiguration.h"

namespace catapult { namespace cache {

	/// Basic delta on top of the operation cache.
	class BasicOperationCacheDelta
		: public BasicLockInfoCacheDelta<OperationCacheDescriptor, OperationCacheTypes>
		, public LockInfoCacheDeltaMixins<OperationCacheDescriptor, OperationCacheTypes>::ConfigBasedEnable<config::OperationConfiguration> {
	public:
		/// Creates a delta around \a operationSets and \a pConfigHolder.
		explicit BasicOperationCacheDelta(
			const OperationCacheTypes::BaseSetDeltaPointers& operationSets,
			std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
				: BasicLockInfoCacheDelta<OperationCacheDescriptor, OperationCacheTypes>::BasicLockInfoCacheDelta(operationSets)
				, LockInfoCacheDeltaMixins<OperationCacheDescriptor, OperationCacheTypes>::ConfigBasedEnable<config::OperationConfiguration>(
					pConfigHolder, [](const auto& config) { return config.Enabled; })
		{}
	};

	/// Delta on top of the operation cache.
	class OperationCacheDelta : public LockInfoCacheDelta<OperationCacheDescriptor, OperationCacheTypes, BasicOperationCacheDelta> {
	public:
		/// Creates a delta around \a operationSets and \a pConfigHolder.
		explicit OperationCacheDelta(
			const OperationCacheTypes::BaseSetDeltaPointers& operationSets,
			std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
				: LockInfoCacheDelta<OperationCacheDescriptor, OperationCacheTypes, BasicOperationCacheDelta>::LockInfoCacheDelta(operationSets, pConfigHolder)
		{}
	};
}}
