/**
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "ReadOnlyViewSequenceCache.h"
#include "ViewSequenceCacheDelta.h"
#include "ViewSequenceCacheView.h"

namespace catapult { namespace cache {

	ReadOnlyViewSequenceCache::ReadOnlyViewSequenceCache(const BasicViewSequenceCacheView& cache)
			: ReadOnlySubCache(cache)
			, m_pCache(&cache)
			, m_pCacheDelta(nullptr)
	{}

	ReadOnlyViewSequenceCache::ReadOnlyViewSequenceCache(const BasicViewSequenceCacheDelta& cache)
			: ReadOnlySubCache(cache)
			, m_pCache(nullptr)
			, m_pCacheDelta(&cache)
	{}

	dbrb::View ReadOnlyViewSequenceCache::getLatestView() const {
		return m_pCache ? m_pCache->getLatestView() : m_pCacheDelta->getLatestView();
	}
}}
