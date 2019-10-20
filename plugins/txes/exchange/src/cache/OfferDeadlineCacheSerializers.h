/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "OfferDeadlineCacheTypes.h"
#include "src/state/OfferDeadlineEntrySerializer.h"
#include "catapult/cache/CacheSerializerAdapter.h"

namespace catapult { namespace cache {

	/// Primary serializer for offer deadline cache.
	struct OfferDeadlineEntryPrimarySerializer : public CacheSerializerAdapter<state::OfferDeadlineEntrySerializer, OfferDeadlineCacheDescriptor>
	{};
}}
