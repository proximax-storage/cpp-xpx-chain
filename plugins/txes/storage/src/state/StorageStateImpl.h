/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/cache/CatapultCache.h"
#include "catapult/state/StorageState.h"
#include "src/cache/ReplicatorKeyCollector.h"

namespace catapult { namespace state {

	class StorageStateImpl : public StorageState {
	public:
		explicit StorageStateImpl(std::shared_ptr<cache::ReplicatorKeyCollector> pKeyCollector)
			: m_pKeyCollector(std::move(pKeyCollector))
		{}

	public:
		bool isReplicatorRegistered(const Key& key) override;
		std::vector<Key> getAllReplicators() override;

		bool driveExist(const Key& driveKey) override;
		Drive getDrive(const Key& driveKey) override;
		bool isReplicatorBelongToDrive(const Key& key, const Key& driveKey) override;
		std::vector<Drive> getReplicatorDrives(const Key& replicatorKey) override;
		std::vector<Key> getDriveReplicators(const Key& driveKey) override;

		uint64_t getDownloadWork(const Key& replicatorKey, const Key& driveKey) override;

		bool downloadChannelExist(const Hash256& id) override;
		std::vector<DownloadChannel> getDownloadChannels() override;
		DownloadChannel getDownloadChannel(const Hash256& id) override;

	private:
		std::shared_ptr<cache::ReplicatorKeyCollector> m_pKeyCollector;
	};
}}
