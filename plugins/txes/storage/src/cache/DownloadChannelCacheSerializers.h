/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "DownloadChannelCacheTypes.h"
#include "src/state/DownloadChannelEntrySerializer.h"
#include "catapult/cache/CacheSerializerAdapter.h"

namespace catapult { namespace cache {

	/// Primary serializer for download cache.
	struct DownloadChannelEntryPrimarySerializer : public CacheSerializerAdapter<state::DownloadChannelEntrySerializer, DownloadChannelCacheDescriptor>
	{};
}}
