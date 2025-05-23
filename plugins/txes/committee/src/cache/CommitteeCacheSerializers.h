/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "CommitteeCacheTypes.h"
#include "src/state/CommitteeEntrySerializers.h"
#include "catapult/cache/CacheSerializerAdapter.h"

namespace catapult { namespace cache {

	/// Primary serializer for committee cache.
	struct CommitteeEntryPrimarySerializer : public CacheSerializerAdapter<state::CommitteeEntrySerializer, CommitteeCacheDescriptor> {};

	/// Primary serializer for committee cache.
	struct CommitteeEntryPatriciaTreeSerializer : public CacheSerializerAdapter<state::CommitteeEntryPatriciaTreeSerializer, CommitteeCacheDescriptor> {};
}}
