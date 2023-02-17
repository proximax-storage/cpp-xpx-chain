/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/cache/CatapultCache.h"
#include "catapult/utils/Logging.h"

namespace catapult { namespace extensions {
	/// Holder used to hold the cache.
	class CacheHolder {
	public:
		/// Creates a default holder.
		CacheHolder() : m_catapultCache({}) {};
		CATAPULT_DESTRUCTOR_CLEANUP_LOG(info, CacheHolder, "Destroying cache holder.")
	public:
		/// Gets const cache.
		const cache::CatapultCache& cache() const {
			return m_catapultCache;
		}

		/// Gets const cache.
		cache::CatapultCache& cache() {
			return m_catapultCache;
		}

	private:
		cache::CatapultCache m_catapultCache;
	};
}}
