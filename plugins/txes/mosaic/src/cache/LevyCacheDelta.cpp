/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "LevyCacheDelta.h"
#include "catapult/cache/IdentifierGroupCacheUtils.h"

namespace catapult { namespace cache {
	void BasicLevyCacheDelta::insert(const state::LevyEntry& entry) {
		LevyCacheDeltaMixins::BasicInsertRemove::insert(entry);
	}
	
	void BasicLevyCacheDelta::remove(MosaicId mosaicId) {
		LevyCacheDeltaMixins::BasicInsertRemove::remove(mosaicId);
	}
		
	BasicLevyCacheDelta::CachedMosaicIdByHeight BasicLevyCacheDelta::getCachedMosaicIdsByHeight(const Height& height) {
		return GetAllIdentifiersWithGroup(*m_pHistoryAtHeight, height);
	}
}}
