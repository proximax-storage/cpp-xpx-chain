/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "DownloadCacheSerializers.h"
#include "DownloadCacheTypes.h"
#include "plugins/txes/lock_shared/src/cache/LockInfoBaseSets.h"

namespace catapult { namespace cache {

	class DownloadPatriciaTree : public LockInfoPatriciaTree<DownloadCacheDescriptor> {
	public:
		using LockInfoPatriciaTree<DownloadCacheDescriptor>::LockInfoPatriciaTree;
		using Serializer = DownloadCacheDescriptor::Serializer;
	};

	struct DownloadBaseSetDeltaPointers : public LockInfoBaseSetDeltaPointers<DownloadCacheTypes, DownloadCacheDescriptor>
	{};

	struct DownloadBaseSets : public LockInfoBaseSets<DownloadCacheTypes, DownloadCacheDescriptor, DownloadBaseSetDeltaPointers> {
		using LockInfoBaseSets<
			DownloadCacheTypes,
			DownloadCacheDescriptor,
			DownloadBaseSetDeltaPointers>::LockInfoBaseSets;
	};
}}
