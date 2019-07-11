/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/cache/CatapultCache.h"

namespace catapult { namespace extensions {
	/// Holder used to hold the cache.
	class CacheHolder {
	public:
		/// Creates a default holder.
		CacheHolder() : m_catapultCache({}) {};

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
