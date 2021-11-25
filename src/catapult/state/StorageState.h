/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/types.h"
#include "catapult/utils/NonCopyable.h"
#include "plugins/txes/storage/src/state/BcDriveEntry.h"
#include <vector>
#include <map>

namespace catapult { namespace cache { class CatapultCache; } }

namespace catapult { namespace state {

	struct DownloadChannelState {
		Key Consumer;
		uint64_t DownloadSize;
		uint16_t DownloadApprovalCount;
		std::vector<Key> ListOfPublicKeys;
	};

	struct DriveState {
		Key Key;
		uint64_t Size;
		uint64_t UsedSize;
		utils::KeySet Replicators;
		ActiveDataModification LastDataModification;
        // TODO add channels in V3
	};

	struct ReplicatorData {
		std::vector<DriveState> DrivesStates;
		std::vector<DownloadChannelState> DownloadChannels;
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
