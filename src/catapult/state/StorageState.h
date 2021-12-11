/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/types.h"
#include "catapult/utils/ArraySet.h"
#include "catapult/utils/NonCopyable.h"
#include <vector>

namespace catapult { namespace cache { class CatapultCache; } }

namespace catapult { namespace state {

	struct DataModification {
		Hash256 Id;
		Key Owner;
		Hash256 DownloadDataCdi;
		uint64_t ExpectedUploadSize;
		uint64_t ActualUploadSize;
	};

	struct Drive {
		Key Id;
		Key Owner;
		uint64_t Size;
		uint64_t UsedSize;
		utils::KeySet Replicators;
		std::vector<DataModification> DataModifications;
	};

	struct DownloadChannel {
		Hash256 Id;
		uint64_t DownloadSize;
		uint16_t DownloadApprovalCount;
		std::vector<Key> Consumers;
		Key DriveKey; // TODO inv V3
	};

	/// Interface for storage state.
	class StorageState : public utils::NonCopyable {
	public:
		virtual ~StorageState() = default;

	public:
		void setCache(cache::CatapultCache* pCache) {
			m_pCache = pCache;
		}

	public:
		virtual bool isReplicatorRegistered(const Key& key) = 0;
		virtual std::vector<Key> getAllReplicators() = 0; // TODO only for V2. Delete in V3

		virtual bool driveExist(const Key& driveKey) = 0;
		virtual Drive getDrive(const Key& driveKey) = 0;
		virtual bool isReplicatorBelongToDrive(const Key& key, const Key& driveKey) = 0;
		virtual std::vector<Drive> getReplicatorDrives(const Key& replicatorKey) = 0;
		virtual std::vector<Key> getDriveReplicators(const Key& driveKey) = 0;

        virtual Hash256 getLastApprovedDataModificationId(const Key& driveKey) = 0;

		virtual uint64_t getDownloadWork(const Key& replicatorKey, const Key& driveKey) = 0;

		virtual bool downloadChannelExist(const Hash256& id) = 0;
		virtual std::vector<DownloadChannel> getDownloadChannels() = 0;
		virtual DownloadChannel getDownloadChannel(const Hash256& id) = 0;

	protected:
		cache::CatapultCache* m_pCache;
	};
}}
