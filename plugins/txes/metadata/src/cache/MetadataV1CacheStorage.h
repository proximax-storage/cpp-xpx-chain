/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "MetadataV1CacheTypes.h"
#include "plugins/txes/metadata/src/state/MetadataV1Serializer.h"
#include "catapult/cache/CacheStorageInclude.h"

namespace catapult { namespace cache {

	/// Policy for saving and loading metadata cache data.
	struct MetadataV1CacheStorage
			: public CacheStorageForBasicInsertRemoveCache<MetadataV1CacheDescriptor>
			, public state::MetadataV1Serializer {
		/// Loads \a metadata into \a cacheDelta.
		static void LoadInto(const ValueType& metadata, DestinationType& cacheDelta);
	};
}}
