/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "OperationBaseSets.h"
#include "OperationCacheSerializers.h"
#include "OperationCacheTools.h"
#include "catapult/config_holder/BlockchainConfigurationHolder.h"
#include "plugins/txes/lock_shared/src/cache/LockInfoCacheDelta.h"

namespace catapult { namespace cache {

	/// Basic delta on top of the operation cache.
	class BasicOperationCacheDelta : public BasicLockInfoCacheDelta<OperationCacheDescriptor, OperationCacheTypes> {
	public:
		/// Creates a delta around \a operationSets and \a pConfigHolder.
		explicit BasicOperationCacheDelta(
			const OperationCacheTypes::BaseSetDeltaPointers& operationSets,
			std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
				: BasicLockInfoCacheDelta<OperationCacheDescriptor, OperationCacheTypes>::BasicLockInfoCacheDelta(operationSets)
				, m_pConfigHolder(pConfigHolder)
		{}

	public:
		bool enabled() const {
			return OperationPluginEnabled(m_pConfigHolder, height());
		}

	private:
		std::shared_ptr<config::BlockchainConfigurationHolder> m_pConfigHolder;
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
