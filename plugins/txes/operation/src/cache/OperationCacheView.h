/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "OperationBaseSets.h"
#include "OperationCacheSerializers.h"
#include "OperationCacheTools.h"
#include "plugins/txes/lock_shared/src/cache/LockInfoCacheView.h"

namespace catapult { namespace cache {

	/// Basic view on top of the operation cache.
	class BasicOperationCacheView : public BasicLockInfoCacheView<OperationCacheDescriptor, OperationCacheTypes> {
	public:
		/// Creates a view around \a operationSets and \a pConfigHolder.
		explicit BasicOperationCacheView(
			const OperationCacheTypes::BaseSets& operationSets,
			std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
				: BasicLockInfoCacheView<OperationCacheDescriptor, OperationCacheTypes>::BasicLockInfoCacheView(operationSets)
				, m_pConfigHolder(pConfigHolder)
		{}

	public:
		bool enabled() const {
			return OperationPluginEnabled(m_pConfigHolder, height());
		}

	private:
		std::shared_ptr<config::BlockchainConfigurationHolder> m_pConfigHolder;
	};

	/// View on top of the operation cache.
	class OperationCacheView : public LockInfoCacheView<OperationCacheDescriptor, OperationCacheTypes, BasicOperationCacheView> {
	public:
		/// Creates a view around \a operationSets and \a pConfigHolder.
		explicit OperationCacheView(
			const OperationCacheTypes::BaseSets& operationSets,
			std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
				: LockInfoCacheView<OperationCacheDescriptor, OperationCacheTypes, BasicOperationCacheView>::LockInfoCacheView(operationSets, pConfigHolder)
		{}
	};
}}
