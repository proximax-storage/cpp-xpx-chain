/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "OperationBaseSets.h"
#include "OperationCacheSerializers.h"
#include "plugins/txes/lock_shared/src/cache/LockInfoCacheDelta.h"

namespace catapult { namespace cache {

	/// Basic delta on top of the operation cache.
	class BasicOperationCacheDelta : public BasicLockInfoCacheDelta<OperationCacheDescriptor, OperationCacheTypes> {
	public:
		using BasicLockInfoCacheDelta<OperationCacheDescriptor, OperationCacheTypes>::BasicLockInfoCacheDelta;
	};

	/// Delta on top of the operation cache.
	class OperationCacheDelta
			: public LockInfoCacheDelta<OperationCacheDescriptor, OperationCacheTypes, BasicOperationCacheDelta> {
	public:
		using LockInfoCacheDelta<
				OperationCacheDescriptor,
				OperationCacheTypes,
				BasicOperationCacheDelta>::LockInfoCacheDelta;
	};
}}
