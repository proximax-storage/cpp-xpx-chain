/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "src/cache/BcDriveCache.h"
#include "src/cache/BcDriveCacheStorage.h"
#include "src/cache/DownloadChannelCache.h"
#include "src/cache/DownloadChannelCacheStorage.h"
#include "src/cache/ReplicatorCache.h"
#include "src/cache/ReplicatorCacheStorage.h"
#include "src/model/StorageEntityType.h"
#include "tests/test/cache/CacheTestUtils.h"
#include "tests/test/core/mocks/MockBlockchainConfigurationHolder.h"
#include "tests/test/other/MutableBlockchainConfiguration.h"
#include "tests/test/nodeps/Random.h"

namespace catapult { namespace test {

    /// Creates test drive entry.
    state::BcDriveEntry CreateBcDriveEntry(
        Key key = test::GenerateRandomByteArray<Key>(),
        Key owner = test::GenerateRandomByteArray<Key>(),
        Hash256 rootHash = test::GenerateRandomByteArray<Hash256>(),
		uint64_t size = test::Random(),
		uint16_t replicatorCount = test::Random16(),
		uint16_t activeDataModificationsCount = 2,
		uint16_t completedDataModificationsCount = 2,
		uint16_t activeDownloadsCount = 2,
		uint16_t completedDownloadsCount = 2
    );

	/// Verifies that \a entry1 is equivalent to \a entry2.
    void AssertEqualBcDriveData(const state::BcDriveEntry& expectedEntry, const state::BcDriveEntry& entry);

    /// Cache factory for creating a catapult cache composed of bc drive cache and core caches.
    struct BcDriveCacheFactory {
        private:
            static auto CreateSubCacheWithBcDriveCache(const config::BlockchainConfiguration& config) {
                auto id = std::max(cache::BcDriveCache::Id, std::max(cache::DownloadChannelCache::Id, cache::ReplicatorCache::Id));
                std::vector<std::unique_ptr<cache::SubCachePlugin>> subCaches(id + 1);
			    auto pConfigHolder = config::CreateMockConfigurationHolder(config);
			    subCaches[cache::BcDriveCache::Id] = MakeSubCachePlugin<cache::BcDriveCache, cache::BcDriveCacheStorage>(pConfigHolder);
			    subCaches[cache::DownloadChannelCache::Id] = MakeSubCachePlugin<cache::DownloadChannelCache, cache::DownloadChannelCacheStorage>(pConfigHolder);
                subCaches[cache::ReplicatorCache::Id] = MakeSubCachePlugin<cache::ReplicatorCache, cache::ReplicatorCacheStorage>(pConfigHolder);
			    return subCaches;
            }

        public:
            /// Creates an empty catapult cache around default configuration.
            static cache::CatapultCache Create() {
                return Create(test::MutableBlockchainConfiguration().ToConst());
            }

            /// Creates an empty catapult cache around \a config.
		    static cache::CatapultCache Create(const config::BlockchainConfiguration& config) {
                auto subCaches = CreateSubCacheWithBcDriveCache(config);
                CoreSystemCacheFactory::CreateSubCaches(config, subCaches);
                return cache::CatapultCache(std::move(subCaches));
		    }
    };

    /// Creates test drive entry.
    state::DownloadChannelEntry CreateDownloadChannelEntry(
        Hash256 id = test::GenerateRandomByteArray<Hash256>(),
        Key consumer = test::GenerateRandomByteArray<Key>(),
        Key drive = test::GenerateRandomByteArray<Key>(),
        Amount transactionFee = test::GenerateRandomByteArray<Amount>(),
        Amount storageUnits  = test::GenerateRandomByteArray<Amount>()
    );

    /// Verifies that \a entry1 is equivalent to \a entry2.
    void AssertEqualDownloadChannelData(const state::DownloadChannelEntry& expectedEntry, const state::DownloadChannelEntry& entry);

    /// Cache factory for creating a catapult cache composed of download channel cache and core caches.
    struct DownloadChannelFactory {
        private:
            static auto CreateSubCachesWithDriveCache(const config::BlockchainConfiguration& config) {
                auto id = std::max(cache::BcDriveCache::Id, std::max(cache::DownloadChannelCache::Id, cache::ReplicatorCache::Id));
                std::vector<std::unique_ptr<cache::SubCachePlugin>> subCaches(id + 1);
			    auto pConfigHolder = config::CreateMockConfigurationHolder(config);
			    subCaches[cache::BcDriveCache::Id] = MakeSubCachePlugin<cache::BcDriveCache, cache::BcDriveCacheStorage>(pConfigHolder);
			    subCaches[cache::DownloadChannelCache::Id] = MakeSubCachePlugin<cache::DownloadChannelCache, cache::DownloadChannelCacheStorage>(pConfigHolder);
                subCaches[cache::ReplicatorCache::Id] = MakeSubCachePlugin<cache::ReplicatorCache, cache::ReplicatorCacheStorage>(pConfigHolder);
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
    state::ReplicatorEntry CreateReplicatorEntry(
        Key key = test::GenerateRandomByteArray<Key>(),
        Amount capacity = test::GenerateRandomByteArray<Amount>(),
		uint16_t drivesCount = 2
    );

    /// Verifies that \a entry1 is equivalent to \a entry2.
    void AssertEqualReplicatorData(const state::ReplicatorEntry& expectedEntry, const state::ReplicatorEntry& entry);

     /// Cache factory for creating a catapult cache composed of replicator cache and core caches.
    struct ReplicatorFactory {
        private:
            static auto CreateSubCachesWithDriveCache(const config::BlockchainConfiguration& config) {
                auto id = std::max(cache::BcDriveCache::Id, std::max(cache::DownloadChannelCache::Id, cache::ReplicatorCache::Id));
                std::vector<std::unique_ptr<cache::SubCachePlugin>> subCaches(id + 1);
			    auto pConfigHolder = config::CreateMockConfigurationHolder(config);
			    subCaches[cache::BcDriveCache::Id] = MakeSubCachePlugin<cache::BcDriveCache, cache::BcDriveCacheStorage>(pConfigHolder);
			    subCaches[cache::DownloadChannelCache::Id] = MakeSubCachePlugin<cache::DownloadChannelCache, cache::DownloadChannelCacheStorage>(pConfigHolder);
                subCaches[cache::ReplicatorCache::Id] = MakeSubCachePlugin<cache::ReplicatorCache, cache::ReplicatorCacheStorage>(pConfigHolder);
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

    /// Creates a prepare bc drive transaction.
    template<typename TTransaction>
    model::UniqueEntityPtr<TTransaction> CreatePrepareBcDriveTransaction() {
        auto pTransaction = CreateTransaction<TTransaction>(model::Entity_Type_PrepareBcDrive);
        pTransaction->Owner = test::GenerateRandomByteArray<Key>();
        pTransaction->DriveKey = test::GenerateRandomByteArray<Key>();
        pTransaction->DriveSize = test::Random();
        pTransaction->ReplicatorCount = test::Random16();
        return pTransaction;
    }

    /// Creates a data modification transaction.
    template<typename TTransaction>
    model::UniqueEntityPtr<TTransaction> CreateDataModificationTransaction() {
        auto pTransaction = CreateTransaction<TTransaction>(model::Entity_Type_DataModification);
        pTransaction->DataModificationId = test::GenerateRandomByteArray<Hash256>();
        pTransaction->DriveKey = test::GenerateRandomByteArray<Key>();
        pTransaction->Owner = test::GenerateRandomByteArray<Key>();
        pTransaction->DownloadDataCdi = test::GenerateRandomByteArray<Hash256>();
        pTransaction->UploadSize = test::Random();
        return pTransaction;
    }

    /// Creates a download transaction.
    template<typename TTransaction>
    model::UniqueEntityPtr<TTransaction> CreateDownloadTransaction() {
        auto pTransaction = CreateTransaction<TTransaction>(model::Entity_Type_Download);
        pTransaction->DriveKey = test::GenerateRandomByteArray<Key>();
        pTransaction->DownloadSize = test::Random();
        pTransaction->TransactionFee = test::GenerateRandomByteArray<Amount>();
        return pTransaction;
    }

    /// Creates a data modification approval transaction.
    template<typename TTransaction>
    model::UniqueEntityPtr<TTransaction> CreateDataModificationApprovalTransaction() {
        auto pTransaction = CreateTransaction<TTransaction>(model::Entity_Type_DataModificationApproval);
        pTransaction->DriveKey = test::GenerateRandomByteArray<Key>();
        pTransaction->DataModificationId = test::GenerateRandomByteArray<Hash256>();
        pTransaction->FileStructureCdi = test::GenerateRandomByteArray<Hash256>();
        pTransaction->FileStructureSize = test::Random();
        pTransaction->UsedDriveSize = test::Random();
        return pTransaction;
    }

    /// Creates a data modification cancel transaction.
    template<typename TTransaction>
    model::UniqueEntityPtr<TTransaction> CreateDataModificationCancelTransaction() {
        auto pTransaction = CreateTransaction<TTransaction>(model::Entity_Type_DataModificationCancel);
        pTransaction->DriveKey = test::GenerateRandomByteArray<Key>();
        pTransaction->DataModificationId = test::GenerateRandomByteArray<Hash256>();
        return pTransaction;
    }

    /// Creates a replicator onboarding transaction.
    template<typename TTransaction>
    model::UniqueEntityPtr<TTransaction> CreateReplicatorOnboardingTransaction() {
        auto pTransaction = CreateTransaction<TTransaction>(model::Entity_Type_ReplicatorOnboarding);
        pTransaction->Capacity = test::GenerateRandomByteArray<Amount>();
        return pTransaction;
    }

    /// Creates a drive closure transaction.
    template<typename TTransaction>
    model::UniqueEntityPtr<TTransaction> CreateDriveClosureTransaction() {
        auto pTransaction = CreateTransaction<TTransaction>(model::Entity_Type_DriveClosure);
        pTransaction->DriveKey = test::GenerateRandomByteArray<Key>();
        return pTransaction;
    }

}}
