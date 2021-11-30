/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
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
		const utils::KeySet& getDriveReplicators(const Key& driveKey, cache::CatapultCache& cache) override;
		const BcDriveEntry* getDrive(const Key& driveKey, cache::CatapultCache& cache) override;
		const DriveInfo* getReplicatorDriveInfo(const Key& replicatorKey, const Key& driveKey, cache::CatapultCache& cache) override;
		std::vector<const BcDriveEntry*> getReplicatorDrives(const Key& replicatorKey, cache::CatapultCache& cache) override;
		std::vector<const DownloadChannelEntry*> getReplicatorDownloadChannels(const Key& replicatorKey, cache::CatapultCache& cache) override;

	private:
		std::shared_ptr<cache::ReplicatorKeyCollector> m_pKeyCollector;
	};
}}
