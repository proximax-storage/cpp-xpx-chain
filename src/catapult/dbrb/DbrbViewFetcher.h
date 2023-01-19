/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma  once
#include "DbrbDefinitions.h"
#include "catapult/utils/NonCopyable.h"

namespace catapult {
	namespace cache { class CatapultCache; }
	namespace ionet { class Node; }
}

namespace catapult { namespace dbrb {

	class DbrbViewFetcher : public utils::NonCopyable {
	public:
		virtual ~DbrbViewFetcher() = default;

	public:
		void setCache(cache::CatapultCache* pCache) {
			m_pCache = pCache;
		}

	public:
		/// Returns the latest registered view.
		virtual ViewData getLatestView() = 0;

	protected:
		cache::CatapultCache* m_pCache;
	};
}}