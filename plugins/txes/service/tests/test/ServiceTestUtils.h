/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "src/cache/DownloadCache.h"
#include "src/cache/DownloadCacheStorage.h"
#include "src/cache/DriveCache.h"
#include "src/cache/DriveCacheStorage.h"
#include "src/model/ServiceEntityType.h"
#include "plugins/txes/exchange/src/cache/ExchangeCache.h"
#include "plugins/txes/exchange/src/cache/ExchangeCacheStorage.h"
#include "plugins/txes/lock_secret/src/cache/SecretLockInfoCache.h"
#include "plugins/txes/multisig/src/cache/MultisigCache.h"
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
		uint16_t replicatorCount = 2,
		uint16_t activeFilesWithoutDepositCount = 2,
		uint16_t inactiveFilesWithoutDepositCount = 2,
		uint16_t heightCount = 2,
		uint16_t removedReplicatorCount = 2,
		uint16_t uploadPaymentCount = 2);

	state::AccountState CreateAccount(const Key& owner, model::NetworkIdentifier networkIdentifier = model::NetworkIdentifier::Mijin_Test);
	void AssertAccount(const state::AccountState& expected, const state::AccountState& actual);

	state::MultisigEntry CreateMultisigEntry(const state::DriveEntry& driveEntry);
	void AssertMultisig(const cache::MultisigCacheDelta& cache, const state::MultisigEntry& expected);

	/// Verifies that \a entry1 is equivalent to \a entry2.
	void AssertEqualDriveData(const state::DriveEntry& entry1, const state::DriveEntry& entry2);

	/// Cache factory for creating a catapult cache composed of drive cache, multisig cache, secret lock cache and core caches.
	struct DriveCacheFactory {
	private:
		static auto CreateSubCachesWithDriveCache(const config::BlockchainConfiguration& config) {
			auto id = std::max(cache::DriveCache::Id, std::max(cache::MultisigCache::Id, std::max(cache::ExchangeCache::Id, cache::SecretLockInfoCache::Id)));
			std::vector<std::unique_ptr<cache::SubCachePlugin>> subCaches(id + 1);
			auto pConfigHolder = config::CreateMockConfigurationHolder(config);
			subCaches[cache::DriveCache::Id] = MakeSubCachePlugin<cache::DriveCache, cache::DriveCacheStorage>(pConfigHolder);
			subCaches[cache::MultisigCache::Id] = MakeSubCachePlugin<cache::MultisigCache, cache::MultisigCacheStorage>();
			subCaches[cache::ExchangeCache::Id] = MakeSubCachePlugin<cache::ExchangeCache, cache::ExchangeCacheStorage>(pConfigHolder);
			subCaches[cache::SecretLockInfoCache::Id] = MakeSubCachePlugin<cache::SecretLockInfoCache, cache::SecretLockInfoCacheStorage>();
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

	/// Creates test drive entry.
	state::DownloadEntry CreateDownloadEntry(
		Hash256 operationToken = test::GenerateRandomByteArray<Hash256>(),
		Key driveKey = test::GenerateRandomByteArray<Key>(),
		Key fileRecipient = test::GenerateRandomByteArray<Key>(),
		Height height = test::GenerateRandomValue<Height>(),
		uint16_t fileCount = 2);

	/// Verifies that \a entry1 is equivalent to \a entry2.
	void AssertEqualDownloadData(const state::DownloadEntry& entry1, const state::DownloadEntry& entry2);

	/// Cache factory for creating a catapult cache composed of download cache and core caches.
	struct DownloadCacheFactory {
	private:
		static auto CreateSubCachesWithDownloadCache(const config::BlockchainConfiguration& config) {
			auto id = std::max(cache::DriveCache::Id, cache::DownloadCache::Id);
			std::vector<std::unique_ptr<cache::SubCachePlugin>> subCaches(id + 1);
			auto pConfigHolder = config::CreateMockConfigurationHolder(config);
			subCaches[cache::DriveCache::Id] = MakeSubCachePlugin<cache::DriveCache, cache::DriveCacheStorage>(pConfigHolder);
			subCaches[cache::DownloadCache::Id] = MakeSubCachePlugin<cache::DownloadCache, cache::DownloadCacheStorage>(pConfigHolder);
			return subCaches;
		}

	public:
		/// Creates an empty catapult cache around default configuration.
		static cache::CatapultCache Create() {
			return Create(test::MutableBlockchainConfiguration().ToConst());
		}

		/// Creates an empty catapult cache around \a config.
		static cache::CatapultCache Create(const config::BlockchainConfiguration& config) {
			auto subCaches = CreateSubCachesWithDownloadCache(config);
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
	model::UniqueEntityPtr<TTransaction> CreateDriveFilesRewardTransaction(size_t numUploadInfos) {
        auto pTransaction = CreateDriveTransaction<TTransaction>(model::Entity_Type_DriveFilesReward, numUploadInfos * sizeof(model::UploadInfo));
		pTransaction->UploadInfosCount = numUploadInfos;

		auto* pData = reinterpret_cast<uint8_t*>(pTransaction.get() + 1);
		for (auto i = 0u; i < numUploadInfos; ++i) {
			auto pDeletedFile = reinterpret_cast<model::UploadInfo*>(pData);
			pDeletedFile->Participant = test::GenerateRandomByteArray<Key>();
			pDeletedFile->Uploaded = test::Random();
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
			model::AddAction addAction{{test::GenerateRandomByteArray<Hash256>()}, test::Random()};
            memcpy(pData, static_cast<const void*>(&addAction), sizeof(model::AddAction));
            pData += sizeof(model::AddAction);
        }

		for (auto i = 0u; i < numRemoveActions; ++i) {
			model::RemoveAction removeAction{{{test::GenerateRandomByteArray<Hash256>()}, test::Random()}};
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
	model::UniqueEntityPtr<TTransaction> CreateEndDriveVerificationTransaction(size_t numFailures, size_t numHashes = 5) {
		auto failureSize = sizeof(model::VerificationFailure) + numHashes * sizeof(Hash256);
		auto pTransaction = CreateDriveTransaction<TTransaction>(model::Entity_Type_End_Drive_Verification, numFailures * failureSize);

        auto* pData = reinterpret_cast<uint8_t*>(pTransaction.get() + 1);
        for (auto i = 0u; i < numFailures; ++i) {
			auto pFailure = reinterpret_cast<model::VerificationFailure*>(pData);
			pFailure->Size = failureSize;
			pFailure->Replicator = test::GenerateRandomByteArray<Key>();
			auto* pBlockHashes = reinterpret_cast<Hash256*>(pFailure + 1);
			for (auto k = 0u; k < numHashes; ++k)
				pBlockHashes[k] = test::GenerateRandomByteArray<Hash256>();
            pData += failureSize;
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

    /// Creates a start file download transaction.
    template<typename TTransaction>
	model::UniqueEntityPtr<TTransaction> CreateStartFileDownloadTransaction(size_t numFiles) {
		auto pTransaction = CreateDriveTransaction<TTransaction>(model::Entity_Type_StartFileDownload, numFiles * sizeof(model::DownloadAction));
		pTransaction->DriveKey = test::GenerateRandomByteArray<Key>();
		pTransaction->FileCount = numFiles;

        auto* pData = reinterpret_cast<uint8_t*>(pTransaction.get() + 1);
        for (auto i = 0u; i < numFiles; ++i) {
			model::DownloadAction file{ { test::GenerateRandomByteArray<Hash256>() }, (i + 1) * 100 };
            memcpy(pData, static_cast<const void*>(&file), sizeof(model::DownloadAction));
            pData += sizeof(model::DownloadAction);
        }

        return pTransaction;
    }

    /// Creates a end file download transaction.
    template<typename TTransaction>
	model::UniqueEntityPtr<TTransaction> CreateEndFileDownloadTransaction(size_t numFiles) {
		auto pTransaction = CreateDriveTransaction<TTransaction>(model::Entity_Type_EndFileDownload, numFiles * sizeof(model::File));
		pTransaction->FileRecipient = test::GenerateRandomByteArray<Key>();
		pTransaction->OperationToken = test::GenerateRandomByteArray<Hash256>();
		pTransaction->FileCount = numFiles;

        auto* pData = reinterpret_cast<uint8_t*>(pTransaction.get() + 1);
        for (auto i = 0u; i < numFiles; ++i) {
			model::File file{ test::GenerateRandomByteArray<Hash256>() };
            memcpy(pData, static_cast<const void*>(&file), sizeof(model::File));
            pData += sizeof(model::File);
        }

        return pTransaction;
    }
}}


