/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "OperationCacheSerializers.h"
#include "OperationCacheTypes.h"
#include "plugins/txes/lock_shared/src/cache/LockInfoBaseSets.h"

namespace catapult { namespace cache {

	class OperationPatriciaTree : public LockInfoPatriciaTree<OperationCacheDescriptor> {
	public:
		using LockInfoPatriciaTree<OperationCacheDescriptor>::LockInfoPatriciaTree;
		using Serializer = OperationCacheDescriptor::Serializer;
	};

	struct OperationBaseSetDeltaPointers
			: public LockInfoBaseSetDeltaPointers<OperationCacheTypes, OperationCacheDescriptor>
	{};

	struct OperationBaseSets
			: public LockInfoBaseSets<OperationCacheTypes, OperationCacheDescriptor, OperationBaseSetDeltaPointers> {
		using LockInfoBaseSets<
			OperationCacheTypes,
			OperationCacheDescriptor,
			OperationBaseSetDeltaPointers>::LockInfoBaseSets;
	};
}}
