/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/model/Elements.h"
#include "catapult/types.h"
#include "catapult/utils/ArraySet.h"
#include "catapult/utils/NonCopyable.h"
#include <vector>
#include <optional>

namespace catapult { namespace cache { class CatapultCache; } }

namespace catapult { namespace state {

	/// Interface for contract state.
	class ContractState : public utils::NonCopyable {
	public:
		virtual ~ContractState() = default;

	public:
		void setCache(cache::CatapultCache* pCache) {
			m_pCache = pCache;
		}

	public:

		virtual bool isExecutorRegistered(const Key& key) const = 0;

	public:
		virtual Height getChainHeight() = 0;

	protected:
		cache::CatapultCache* m_pCache;
	};
}}
