/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "src/cache/DriveCache.h"
#include "src/cache/DriveCacheStorage.h"
#include "src/model/ServiceEntityType.h"
#include "tests/test/cache/CacheTestUtils.h"
#include "tests/test/core/mocks/MockBlockchainConfigurationHolder.h"
#include "tests/test/nodeps/Random.h"
#include "tests/test/other/MutableBlockchainConfiguration.h"

namespace catapult { namespace test {

	/// Creates test drive entry.
	state::DriveEntry CreateDriveEntry(
		Key key = test::GenerateRandomByteArray<Key>(),
		uint16_t billingHistoryCount = 2,
		uint16_t drivePaymentCount = 2,
		uint16_t fileCount = 2,
		uint16_t fileActionCount = 2,
		uint16_t filePaymentCount = 2,
		uint16_t replicatorCount = 2,
		uint16_t fileWithoutDepositCount = 2);

	/// Verifies that \a entry1 is equivalent to \a entry2.
	void AssertEqualDriveData(const state::DriveEntry& entry1, const state::DriveEntry& entry2);

	/// Cache factory for creating a catapult cache composed of drive cache and core caches.
	struct DriveCacheFactory {
	private:
		static auto CreateSubCachesWithDriveCache(const config::BlockchainConfiguration& config) {
			auto cacheId = cache::DriveCache::Id;
			std::vector<std::unique_ptr<cache::SubCachePlugin>> subCaches(cacheId + 1);
			auto pConfigHolder = config::CreateMockConfigurationHolder(config);
			subCaches[cacheId] = MakeSubCachePlugin<cache::DriveCache, cache::DriveCacheStorage>(pConfigHolder);
			return subCaches;
		}

	public:
		/// Creates an empty catapult cache around default configuration.
		static cache::CatapultCache Create() {
			return Create(test::MutableBlockchainConfiguration().ToConst());
		}

		/// Creates an empty catapult cache around \a config.
		static cache::CatapultCache Create(const config::BlockchainConfiguration& config) {
			auto subCaches = CreateSubCachesWithDriveCache(config);
			CoreSystemCacheFactory::CreateSubCaches(config, subCaches);
			return cache::CatapultCache(std::move(subCaches));
		}
	};

    /// Creates a drive transaction.
    template<typename TTransaction>
	model::UniqueEntityPtr<TTransaction> CreateDriveTransaction(model::EntityType type, size_t additionalSize = 0) {
        uint32_t entitySize = sizeof(TTransaction) + additionalSize;
        auto pTransaction = utils::MakeUniqueWithSize<TTransaction>(entitySize);
		pTransaction->Signer = test::GenerateRandomByteArray<Key>();
		pTransaction->Version = model::MakeVersion(model::NetworkIdentifier::Mijin_Test, 1);
        pTransaction->Type = type;
        pTransaction->Size = entitySize;

        return pTransaction;
    }

    /// Creates a drive files reward transaction.
    template<typename TTransaction>
	model::UniqueEntityPtr<TTransaction> CreateDriveFilesRewardTransaction(size_t numDeletedFiles, size_t numReplicatorInfo) {
		auto deletedFileStructSize = sizeof(model::DeletedFile)  + numReplicatorInfo * sizeof(model::ReplicatorUploadInfo);
        auto pTransaction = CreateDriveTransaction<TTransaction>(model::Entity_Type_DriveFilesReward, numDeletedFiles * deletedFileStructSize);

		auto* pData = reinterpret_cast<uint8_t*>(pTransaction.get() + 1);
		for (auto i = 0u; i < numDeletedFiles; ++i) {
			auto pDeletedFile = reinterpret_cast<model::DeletedFile*>(pData);
			pDeletedFile->FileHash = test::GenerateRandomByteArray<Hash256>();
			pDeletedFile->Size = deletedFileStructSize;
			pData = reinterpret_cast<uint8_t*>(pDeletedFile->InfosPtr());
			for (auto j = 0u; j < numReplicatorInfo; ++j) {
				model::ReplicatorUploadInfo replicatorInfo{{test::GenerateRandomByteArray<Key>()}, j};
				memcpy(pData, static_cast<const void*>(&replicatorInfo), sizeof(model::ReplicatorUploadInfo));
				pData += sizeof(model::ReplicatorUploadInfo);
			}
		}

        return pTransaction;
    }

    /// Creates a drive file system transaction.
    template<typename TTransaction>
	model::UniqueEntityPtr<TTransaction> CreateDriveFileSystemTransaction(size_t numAddActions, size_t numRemoveActions) {
		auto pTransaction = CreateDriveTransaction<TTransaction>(model::Entity_Type_DriveFileSystem,
			numAddActions * sizeof(model::AddAction) + numRemoveActions * sizeof(model::RemoveAction));
		pTransaction->DriveKey = test::GenerateRandomByteArray<Key>();
		pTransaction->RootHash = test::GenerateRandomByteArray<Hash256>();
		pTransaction->XorRootHash = test::GenerateRandomByteArray<Hash256>();
		pTransaction->AddActionsCount = numAddActions;
		pTransaction->RemoveActionsCount = numRemoveActions;

        auto* pData = reinterpret_cast<uint8_t*>(pTransaction.get() + 1);
		for (auto i = 0u; i < numAddActions; ++i) {
			model::AddAction addAction{{test::GenerateRandomByteArray<Hash256>()}, test::GenerateRandomValue<Amount>().unwrap()};
            memcpy(pData, static_cast<const void*>(&addAction), sizeof(model::AddAction));
            pData += sizeof(model::AddAction);
        }

		for (auto i = 0u; i < numRemoveActions; ++i) {
			model::RemoveAction removeAction{{test::GenerateRandomByteArray<Hash256>()}};
            memcpy(pData, static_cast<const void*>(&removeAction), sizeof(model::RemoveAction));
            pData += sizeof(model::RemoveAction);
        }

        return pTransaction;
    }

    /// Creates an end drive transaction.
    template<typename TTransaction>
	model::UniqueEntityPtr<TTransaction> CreateEndDriveTransaction() {
		auto pTransaction = CreateDriveTransaction<TTransaction>(model::Entity_Type_EndDrive);
		pTransaction->DriveKey = test::GenerateRandomByteArray<Key>();
        return pTransaction;
    }

    /// Creates a start drive verification transaction.
    template<typename TTransaction>
	model::UniqueEntityPtr<TTransaction> CreateStartDriveVerificationTransaction() {
		auto pTransaction = CreateDriveTransaction<TTransaction>(model::Entity_Type_Start_Drive_Verification);
		pTransaction->DriveKey = test::GenerateRandomByteArray<Key>();
        return pTransaction;
    }

    /// Creates an end drive verification transaction with \a failures.
    template<typename TTransaction>
	model::UniqueEntityPtr<TTransaction> CreateEndDriveVerificationTransaction(size_t numFailures) {
		auto pTransaction = CreateDriveTransaction<TTransaction>(model::Entity_Type_End_Drive_Verification,
			numFailures * sizeof(model::VerificationFailure));
		pTransaction->FailureCount = numFailures;

        auto* pData = reinterpret_cast<uint8_t*>(pTransaction.get() + 1);
        for (auto i = 0u; i < numFailures; ++i) {
			model::VerificationFailure failure{test::GenerateRandomByteArray<Key>(), test::GenerateRandomByteArray<Hash256>()};
            memcpy(pData, static_cast<const void*>(&failure), sizeof(model::VerificationFailure));
            pData += sizeof(model::VerificationFailure);
        }

        return pTransaction;
    }

    /// Creates a file deposit transaction.
    template<typename TTransaction>
	model::UniqueEntityPtr<TTransaction> CreateFilesDepositTransaction(size_t numFiles) {
		auto pTransaction = CreateDriveTransaction<TTransaction>(model::Entity_Type_FilesDeposit, numFiles * sizeof(model::File));
		pTransaction->DriveKey = test::GenerateRandomByteArray<Key>();
		pTransaction->FilesCount = numFiles;

        auto* pData = reinterpret_cast<uint8_t*>(pTransaction.get() + 1);
        for (auto i = 0u; i < numFiles; ++i) {
			model::File file{test::GenerateRandomByteArray<Hash256>()};
            memcpy(pData, static_cast<const void*>(&file), sizeof(model::File));
            pData += sizeof(model::File);
        }

        return pTransaction;
    }

    /// Creates a join to drive transaction.
    template<typename TTransaction>
	model::UniqueEntityPtr<TTransaction> CreateJoinToDriveTransaction() {
		auto pTransaction = CreateDriveTransaction<TTransaction>(model::Entity_Type_JoinToDrive);
		pTransaction->DriveKey = test::GenerateRandomByteArray<Key>();
		return pTransaction;
    }

    /// Creates a prepare drive transaction.
    template<typename TTransaction>
	model::UniqueEntityPtr<TTransaction> CreatePrepareDriveTransaction() {
		auto pTransaction = CreateDriveTransaction<TTransaction>(model::Entity_Type_PrepareDrive);
		pTransaction->Owner = test::GenerateRandomByteArray<Key>();
		pTransaction->Duration = BlockDuration(1000);
		pTransaction->BillingPeriod = BlockDuration(100);
		pTransaction->BillingPrice = Amount(100);
		pTransaction->DriveSize = 500;
		pTransaction->Replicas = 10;
		pTransaction->MinReplicators = 3;
		pTransaction->PercentApprovers = 50;

        return pTransaction;
    }
}}


