/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "LevyCacheDelta.h"
#include "catapult/cache/IdentifierGroupCacheUtils.h"

namespace catapult { namespace cache {
	BasicLevyCacheDelta::CachedMosaicIdByHeight BasicLevyCacheDelta::getCachedMosaicIdsByHeight(const Height& height) {
		return GetAllIdentifiersWithGroup(*m_pHistoryAtHeight, height);
	}
}}
