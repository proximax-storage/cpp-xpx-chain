/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "OperationBaseSets.h"
#include "OperationCacheSerializers.h"
#include "plugins/txes/lock_shared/src/cache/LockInfoCacheView.h"

namespace catapult { namespace cache {

	/// Basic view on top of the operation cache.
	class BasicOperationCacheView : public BasicLockInfoCacheView<OperationCacheDescriptor, OperationCacheTypes> {
	public:
		using BasicLockInfoCacheView<OperationCacheDescriptor, OperationCacheTypes>::BasicLockInfoCacheView;
	};

	/// View on top of the operation cache.
	class OperationCacheView
			: public LockInfoCacheView<OperationCacheDescriptor, OperationCacheTypes, BasicOperationCacheView> {
	public:
		using LockInfoCacheView<OperationCacheDescriptor, OperationCacheTypes, BasicOperationCacheView>::LockInfoCacheView;
	};
}}
