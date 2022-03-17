/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/cache/CacheSerializerAdapter.h"
#include "catapult/cache/IdentifierGroupSerializer.h"
#include "src/state/LockFundRecordSerializer.h"
#include "LockFundCacheTypes.h"

namespace catapult { namespace cache {

	/// Primary serializer for lock fund cache.
	struct LockFundPrimarySerializer : public CacheSerializerAdapter<state::LockFundRecordSerializer<state::LockFundHeightIndexDescriptor>, LockFundCacheDescriptor> {};
	/// Serializer for keyed lock fund record groups.
	struct KeyedLockFundSerializer : public CacheSerializerAdapter<state::LockFundRecordSerializer<state::LockFundKeyIndexDescriptor>, LockFundCacheTypes::KeyedLockFundTypesDescriptor> {};
}}
