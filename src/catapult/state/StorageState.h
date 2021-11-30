/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/types.h"
#include "catapult/utils/NonCopyable.h"
#include "plugins/txes/storage/src/state/BcDriveEntry.h"
#include "plugins/txes/storage/src/state/ReplicatorEntry.h"
#include "plugins/txes/storage/src/state/DownloadChannelEntry.h"
#include <vector>
#include <map>

namespace catapult { namespace cache { class CatapultCache; } }

namespace catapult { namespace state {

	/// Interface for storage state.
	class StorageState : public utils::NonCopyable {
	public:
		virtual ~StorageState() = default;

	public:
		virtual bool isReplicatorRegistered(const Key& key) = 0;
		virtual const utils::KeySet& getDriveReplicators(const Key& driveKey, cache::CatapultCache& cache) = 0;
		virtual const BcDriveEntry* getDrive(const Key& driveKey, cache::CatapultCache& cache) = 0;
		virtual const DriveInfo* getReplicatorDriveInfo(const Key& replicatorKey, const Key& driveKey, cache::CatapultCache& cache) = 0;
		virtual std::vector<const BcDriveEntry*> getReplicatorDrives(const Key& replicatorKey, cache::CatapultCache& cache) = 0;
		virtual std::vector<const DownloadChannelEntry*> getReplicatorDownloadChannels(const Key& replicatorKey, cache::CatapultCache& cache) = 0;
	};
}}
