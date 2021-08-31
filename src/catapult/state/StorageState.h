/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/types.h"
#include "catapult/utils/NonCopyable.h"
#include <vector>
#include <map>

namespace catapult { namespace cache { class CatapultCache; } }

namespace catapult { namespace state {

	struct ReplicatorData {
		std::vector<std::pair<Key, uint64_t>> Drives;
		std::map<Key, std::vector<std::pair<Hash256, Hash256>>> DriveModifications;
		std::map<Key, std::pair<std::vector<Key>, uint64_t>> Consumers;
	};

	/// Interface for storage state.
	class StorageState : public utils::NonCopyable {
	public:
		virtual ~StorageState() = default;

	public:
		virtual bool isReplicatorRegistered(const Key& key) = 0;
		virtual ReplicatorData getReplicatorData(const Key& replicatorKey, cache::CatapultCache& m_cache) = 0;
	};
}}
