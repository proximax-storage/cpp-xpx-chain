/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include <random>
#include "catapult/cache_core/AccountStateCacheDelta.h"
#include "boost/dynamic_bitset.hpp"
#include "boost/iterator/counting_iterator.hpp"
#include "catapult/model/EntityBody.h"
#include "catapult/crypto/KeyPair.h"
#include "catapult/crypto/PrivateKey.h"
#include "catapult/crypto/Signer.h"
#include "catapult/model/StorageNotifications.h"
#include "src/cache/BcDriveCache.h"
#include "src/cache/BcDriveCacheStorage.h"
#include "src/cache/DownloadChannelCache.h"
#include "src/cache/DownloadChannelCacheStorage.h"
#include "src/cache/QueueCacheStorage.h"
#include "src/cache/ReplicatorCache.h"
#include "src/cache/QueueCache.h"
#include "src/cache/ReplicatorCacheStorage.h"
#include "src/model/StorageEntityType.h"
#include "src/utils/StorageUtils.h"
#include "tests/test/cache/CacheTestUtils.h"
#include "tests/test/core/mocks/MockBlockchainConfigurationHolder.h"
#include "tests/test/other/MutableBlockchainConfiguration.h"
#include "tests/test/nodeps/Random.h"

namespace catapult { namespace test {

	/// Cache factory for creating a catapult cache composed of BC drive cache, download channel cache and replicator cache.
	struct StorageCacheFactory {
	private:
		static auto CreateStorageSubCaches(const config::BlockchainConfiguration& config) {
			std::vector<size_t> cacheIds = {
					cache::BcDriveCache::Id,
					cache::DownloadChannelCache::Id,
					cache::ReplicatorCache::Id};
			auto maxId = std::max_element(cacheIds.begin(), cacheIds.end());
			std::vector<std::unique_ptr<cache::SubCachePlugin>> subCaches(*maxId + 1);
			auto pConfigHolder = config::CreateMockConfigurationHolder(config);
			auto pReplicatorKeyCollector = std::make_shared<cache::ReplicatorKeyCollector>();
			subCaches[cache::BcDriveCache::Id] = MakeSubCachePlugin<cache::BcDriveCache, cache::BcDriveCacheStorage>(pConfigHolder);
			subCaches[cache::DownloadChannelCache::Id] = MakeSubCachePlugin<cache::DownloadChannelCache, cache::DownloadChannelCacheStorage>(pConfigHolder);
			subCaches[cache::ReplicatorCache::Id] = MakeSubCachePlugin<cache::ReplicatorCache, cache::ReplicatorCacheStorage>(pReplicatorKeyCollector, pConfigHolder);
			return subCaches;
		}

	public:
		/// Creates an empty catapult cache around default configuration.
		static cache::CatapultCache Create() {
			return Create(test::MutableBlockchainConfiguration().ToConst());
		}

		/// Creates an empty catapult cache around \a config.
		static cache::CatapultCache Create(const config::BlockchainConfiguration& config) {
			auto subCaches = CreateStorageSubCaches(config);
			CoreSystemCacheFactory::CreateSubCaches(config, subCaches);
			return cache::CatapultCache(std::move(subCaches));
		}
	};

    /// Creates test drive entry.
    state::BcDriveEntry CreateBcDriveEntry(
        Key key = test::GenerateRandomByteArray<Key>(),
        Key owner = test::GenerateRandomByteArray<Key>(),
        Hash256 rootHash = test::GenerateRandomByteArray<Hash256>(),
		uint64_t size = test::Random(),
		uint16_t replicatorCount = test::Random16(),
		uint16_t activeDataModificationsCount = 2,
		uint16_t completedDataModificationsCount = 2,
        uint16_t verificationsCount = 2,
        uint16_t activeDownloadsCount = 2,
        uint16_t completedDownloadsCount = 2,
        uint16_t downloadShardsCount = 3
    );

	/// Verifies that \a entry1 is equivalent to \a entry2.
    void AssertEqualBcDriveData(const state::BcDriveEntry& expectedEntry, const state::BcDriveEntry& entry);

    /// Cache factory for creating a catapult cache composed of bc drive cache and core caches.
    struct BcDriveCacheFactory {
        private:
            static auto CreateSubCacheWithBcDriveCache(const config::BlockchainConfiguration& config) {
                auto id = std::max(cache::BcDriveCache::Id, std::max(cache::DownloadChannelCache::Id, std::max(cache::ReplicatorCache::Id, cache::QueueCache::Id)));
                std::vector<std::unique_ptr<cache::SubCachePlugin>> subCaches(id + 1);
			    auto pConfigHolder = config::CreateMockConfigurationHolder(config);
                auto pReplicatorKeyCollector = std::make_shared<cache::ReplicatorKeyCollector>();
                subCaches[cache::BcDriveCache::Id] = MakeSubCachePlugin<cache::BcDriveCache, cache::BcDriveCacheStorage>(pConfigHolder);
			    subCaches[cache::DownloadChannelCache::Id] = MakeSubCachePlugin<cache::DownloadChannelCache, cache::DownloadChannelCacheStorage>(pConfigHolder);
                subCaches[cache::ReplicatorCache::Id] = MakeSubCachePlugin<cache::ReplicatorCache, cache::ReplicatorCacheStorage>(pReplicatorKeyCollector, pConfigHolder);
                subCaches[cache::QueueCache::Id] = MakeSubCachePlugin<cache::QueueCache, cache::QueueCacheStorage>(pConfigHolder);
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

    /// Creates test download entry.
    state::DownloadChannelEntry CreateDownloadChannelEntry(
        Hash256 id = test::GenerateRandomByteArray<Hash256>(),
        Key consumer = test::GenerateRandomByteArray<Key>(),
		Key drive = test::GenerateRandomByteArray<Key>(),
		uint64_t downloadSize = test::Random(),
		uint16_t downloadApprovalCount = test::Random16(),
		std::vector<Key> listOfPublicKeys = {test::GenerateRandomByteArray<Key>()},
		std::map<Key, Amount> cumulativePayments = {{test::GenerateRandomByteArray<Key>(),Amount{test::Random()}}}
    );

    /// Verifies that \a entry1 is equivalent to \a entry2.
    void AssertEqualDownloadChannelData(const state::DownloadChannelEntry& expectedEntry, const state::DownloadChannelEntry& entry);

    /// Cache factory for creating a catapult cache composed of download channel cache and core caches.
    struct DownloadChannelCacheFactory {
        private:
            static auto CreateSubCachesWithDriveCache(const config::BlockchainConfiguration& config) {
                auto id = std::max(cache::BcDriveCache::Id, std::max(cache::DownloadChannelCache::Id, cache::ReplicatorCache::Id));
                std::vector<std::unique_ptr<cache::SubCachePlugin>> subCaches(id + 1);
			    auto pConfigHolder = config::CreateMockConfigurationHolder(config);
				auto pReplicatorKeyCollector = std::make_shared<cache::ReplicatorKeyCollector>();
				subCaches[cache::BcDriveCache::Id] = MakeSubCachePlugin<cache::BcDriveCache, cache::BcDriveCacheStorage>(pConfigHolder);
				subCaches[cache::DownloadChannelCache::Id] = MakeSubCachePlugin<cache::DownloadChannelCache, cache::DownloadChannelCacheStorage>(pConfigHolder);
				subCaches[cache::ReplicatorCache::Id] = MakeSubCachePlugin<cache::ReplicatorCache, cache::ReplicatorCacheStorage>(pReplicatorKeyCollector, pConfigHolder);
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

    /// Creates test replicator entry.
    state::ReplicatorEntry CreateReplicatorEntry(
        Key key = test::GenerateRandomByteArray<Key>(),
        Amount capacity = test::GenerateRandomValue<Amount>(),
		uint16_t drivesCount = 2,
		uint16_t downloadChannelCount = 4);

    /// Verifies that \a entry1 is equivalent to \a entry2.
    void AssertEqualReplicatorData(const state::ReplicatorEntry& expectedEntry, const state::ReplicatorEntry& entry);

     /// Cache factory for creating a catapult cache composed of replicator cache and core caches.
    struct ReplicatorCacheFactory {
        private:
            static auto CreateSubCachesWithDriveCache(const config::BlockchainConfiguration& config) {
				std::vector<uint32_t> cacheIds = {cache::BcDriveCache::Id, cache::DownloadChannelCache::Id, cache::ReplicatorCache::Id};
				auto id = *std::max_element(cacheIds.begin(), cacheIds.end());
                std::vector<std::unique_ptr<cache::SubCachePlugin>> subCaches(id + 1);
			    auto pConfigHolder = config::CreateMockConfigurationHolder(config);
				auto pReplicatorKeyCollector = std::make_shared<cache::ReplicatorKeyCollector>();
				subCaches[cache::BcDriveCache::Id] = MakeSubCachePlugin<cache::BcDriveCache, cache::BcDriveCacheStorage>(pConfigHolder);
				subCaches[cache::DownloadChannelCache::Id] = MakeSubCachePlugin<cache::DownloadChannelCache, cache::DownloadChannelCacheStorage>(pConfigHolder);
				subCaches[cache::ReplicatorCache::Id] = MakeSubCachePlugin<cache::ReplicatorCache, cache::ReplicatorCacheStorage>(pReplicatorKeyCollector, pConfigHolder);
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

	/// Adds \a count unique replicator entries to the respective cache.
	/// Adds the key pairs of created replicators to \a keys.
	void AddReplicators(
			cache::CatapultCache& cache,
			std::vector<crypto::KeyPair>& replicatorKeyPairs,
			uint8_t count,
			Height height = Height(1));

	/// Common fields of opinion-based multisignature transactions (DownloadApproval, DataModificationApproval, EndDriveVerification).
	template<typename TOpinion>
	struct OpinionData {
		uint8_t JudgingKeysCount;
		uint8_t OverlappingKeysCount;
		uint8_t JudgedKeysCount;
		uint16_t OpinionElementCount;
		int16_t FilledPresenceRowIndex;
		std::vector<Key> PublicKeys;
		std::vector<Signature> Signatures;
		std::vector<std::vector<bool>> PresentOpinions;
		std::vector<std::vector<TOpinion>> Opinions;
	};

	/// Creates RawBuffer with given \a size and fills it with random data. Data pointer must be manually deleted after use.
	RawBuffer GenerateCommonDataBuffer(const size_t& size);

	/// Populates \a replicatorKeyPairs up to \a replicatorCount random key pairs.
	void PopulateReplicatorKeyPairs(std::vector<crypto::KeyPair>& replicatorKeyPairs, uint16_t replicatorCount);

	/// Adds account state with \a publicKey and provided \a mosaics to \a accountStateCache at height \a height.
	void AddAccountState(
			cache::AccountStateCacheDelta& accountStateCache,
			const Key& publicKey,
			const Height& height = Height(1),
			const std::vector<model::Mosaic>& mosaics = {});

	/// Creates an OpinionData filled with valid data.
	/// Tuple \c publicKeysCounts contains \a JudgingKeysCount, \a OverlappingKeysCount and \a JudgedKeysCount in that order.
	template<typename TOpinion>
	OpinionData<TOpinion> CreateValidOpinionData(
			const std::vector<crypto::KeyPair>& replicatorKeyPairs,
			const RawBuffer& commonDataBuffer,
			const std::tuple<uint8_t, uint8_t, uint8_t> publicKeysCounts = {0, 0, 0},
			const bool filledPresenceRow = false) {
		OpinionData<TOpinion> data;

		// Generating valid counts.
		const bool countsProvided = publicKeysCounts != std::tuple<uint8_t, uint8_t, uint8_t>(0, 0, 0);
		const auto totalKeysCount = countsProvided ?
				std::get<0>(publicKeysCounts) + std::get<1>(publicKeysCounts) + std::get<2>(publicKeysCounts)
				: test::RandomInRange<uint8_t>(1, replicatorKeyPairs.size());
		data.OverlappingKeysCount = countsProvided ? std::get<1>(publicKeysCounts) : test::RandomInRange<uint8_t>(totalKeysCount > 1 ? 0 : 1, totalKeysCount);
		data.JudgingKeysCount = countsProvided ? std::get<0>(publicKeysCounts) : test::RandomInRange<uint8_t>(data.OverlappingKeysCount ? 0 : 1, totalKeysCount - data.OverlappingKeysCount - (data.OverlappingKeysCount ? 0 : 1));
		data.JudgedKeysCount = countsProvided ? std::get<2>(publicKeysCounts) : totalKeysCount - data.OverlappingKeysCount - data.JudgingKeysCount;
		const auto totalJudgingKeysCount = totalKeysCount - data.JudgedKeysCount;
		const auto totalJudgedKeysCount = totalKeysCount - data.JudgingKeysCount;
		const auto presentOpinionCount = totalJudgingKeysCount * totalJudgedKeysCount;

		// Filling PublicKeys.
		for (auto i = 0u; i < totalKeysCount; ++i)
			data.PublicKeys.push_back(replicatorKeyPairs.at(i).publicKey());

		// Filling PresentOpinions and Opinions.
		data.PresentOpinions.resize(totalJudgingKeysCount);
		data.Opinions.resize(totalJudgingKeysCount);
		std::vector<uint8_t> predPresenceIndices;	// Vector of predetermined indices used to guarantee that every judged key is used. If Nth element in predPresenceIndices is M, then Mth element in Nth column of presentOpinions must be true.
		predPresenceIndices.reserve(totalJudgedKeysCount);
		data.FilledPresenceRowIndex = filledPresenceRow ? test::RandomInRange(0, std::max(totalJudgingKeysCount-2, 0)) : -1;	// The row in PresentOpinions filled with 1's is guaranteed not to be the last.
		for (auto i = 0u; i < totalJudgedKeysCount; ++i)
			predPresenceIndices.emplace_back(test::RandomInRange(0, totalJudgingKeysCount-1));
		for (auto i = 0u; i < totalJudgingKeysCount; ++i) {
			data.PresentOpinions.at(i).reserve(totalJudgedKeysCount);
			for (auto j = 0u; j < totalJudgedKeysCount; ++j) {
				const bool bit = data.FilledPresenceRowIndex == i || predPresenceIndices.at(j) == i || test::Random() % 10; // True according to FilledPresenceRowIndex, predPresenceIndices or with probability 0.9
				data.OpinionElementCount += bit;
				data.PresentOpinions.at(i).push_back(bit);
				if (bit) {
					TOpinion opinionElement;
					FillWithRandomData({ reinterpret_cast<uint8_t*>(&opinionElement), sizeof(TOpinion) });
					data.Opinions.at(i).push_back(opinionElement);
				}
			}
		}

		// Filling Signatures.
		const auto maxDataSize = commonDataBuffer.Size + (sizeof(Key) + sizeof(TOpinion)) * totalJudgedKeysCount;	// Guarantees that every possible individual opinion will fit in.
		const auto pDataBegin = std::unique_ptr<uint8_t[]>(new uint8_t[maxDataSize]);
		memcpy(pDataBegin.get(), commonDataBuffer.pData, commonDataBuffer.Size);
		auto* const pIndividualDataBegin = pDataBegin.get() + commonDataBuffer.Size;

		using OpinionElement = std::pair<Key, TOpinion>;
		const auto comparator = [](const OpinionElement& a, const OpinionElement& b){ return a.first < b.first; };
		std::set<OpinionElement, decltype(comparator)> individualPart(comparator);	// Set that represents complete opinion of one of the replicators. Opinion elements are sorted in ascending order of keys.
		for (auto i = 0; i < totalJudgingKeysCount; ++i) {
			individualPart.clear();
			for (auto j = 0, k = 0; j < totalJudgedKeysCount; ++j) {
				if (data.PresentOpinions.at(i).at(j)) {
					individualPart.emplace(data.PublicKeys.at(data.JudgingKeysCount + j), data.Opinions.at(i).at(k));
					++k;
				}
			}

			const auto dataSize = commonDataBuffer.Size + (sizeof(Key) + sizeof(TOpinion)) * individualPart.size();
			auto* pIndividualData = pIndividualDataBegin;
			for (const auto& opinionElement : individualPart) {
				utils::WriteToByteArray(pIndividualData, opinionElement.first);
				utils::WriteToByteArray(pIndividualData, opinionElement.second);
			}

			RawBuffer dataBuffer(pDataBegin.get(), dataSize);
			Signature signature;
			catapult::crypto::Sign(replicatorKeyPairs.at(i), dataBuffer, signature);
			data.Signatures.push_back(signature);
		}

		return data;
	};

    /// Creates a transaction.
    template<typename TTransaction>
	model::UniqueEntityPtr<TTransaction> CreateTransaction(model::EntityType type, size_t additionalSize = 0) {
        uint32_t entitySize = sizeof(TTransaction) + additionalSize;
        auto pTransaction = utils::MakeUniqueWithSize<TTransaction>(entitySize);
		pTransaction->Signer = test::GenerateRandomByteArray<Key>();
		pTransaction->Version = model::MakeVersion(model::NetworkIdentifier::Mijin_Test, 1);
        pTransaction->Type = type;
        pTransaction->Size = entitySize;

        return pTransaction;
    }

    /// Creates a prepare bc drive transaction.
    template<typename TTransaction>
    model::UniqueEntityPtr<TTransaction> CreatePrepareBcDriveTransaction() {
        auto pTransaction = CreateTransaction<TTransaction>(model::Entity_Type_PrepareBcDrive);
        pTransaction->DriveSize = test::Random();
        pTransaction->ReplicatorCount = test::Random16();
        return pTransaction;
    }

    /// Creates a data modification transaction.
    template<typename TTransaction>
    model::UniqueEntityPtr<TTransaction> CreateDataModificationTransaction() {
        auto pTransaction = CreateTransaction<TTransaction>(model::Entity_Type_DataModification);
        pTransaction->DriveKey = test::GenerateRandomByteArray<Key>();
        pTransaction->DownloadDataCdi = test::GenerateRandomByteArray<Hash256>();
        pTransaction->UploadSizeMegabytes = test::Random();
        return pTransaction;
    }

    /// Creates a download transaction.
    template<typename TTransaction>
    model::UniqueEntityPtr<TTransaction> CreateDownloadTransaction() {
        auto pTransaction = CreateTransaction<TTransaction>(model::Entity_Type_Download);
        pTransaction->DownloadSizeMegabytes = test::Random();
		pTransaction->FeedbackFeeAmount = Amount(test::Random());
		pTransaction->ListOfPublicKeysSize = test::Random16();
        return pTransaction;
    }

    /// Creates a data modification approval transaction.
    template<typename TTransaction>
    model::UniqueEntityPtr<TTransaction> CreateDataModificationApprovalTransaction() {
        auto pTransaction = CreateTransaction<TTransaction>(model::Entity_Type_DataModificationApproval);
        pTransaction->DriveKey = test::GenerateRandomByteArray<Key>();
        pTransaction->DataModificationId = test::GenerateRandomByteArray<Hash256>();
        pTransaction->FileStructureCdi = test::GenerateRandomByteArray<Hash256>();
        pTransaction->FileStructureSizeBytes = test::Random();
        pTransaction->UsedDriveSizeBytes = test::Random();
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
        pTransaction->Capacity = test::GenerateRandomValue<Amount>();
        return pTransaction;
    }

	/// Creates a drive closure transaction.
    template<typename TTransaction>
    model::UniqueEntityPtr<TTransaction> CreateDriveClosureTransaction() {
        auto pTransaction = CreateTransaction<TTransaction>(model::Entity_Type_DriveClosure);
        pTransaction->DriveKey = test::GenerateRandomByteArray<Key>();
        return pTransaction;
    }

    /// Creates a replicator offboarding transaction.
    template<typename TTransaction>
    model::UniqueEntityPtr<TTransaction> CreateReplicatorOffboardingTransaction() {
        auto pTransaction = CreateTransaction<TTransaction>(model::Entity_Type_ReplicatorOffboarding);
        return pTransaction;
    }

    /// Creates a end drive verification transaction.
    template<typename TTransaction>
    model::UniqueEntityPtr<TTransaction> CreateEndDriveVerificationTransaction(uint8_t keyCount, uint8_t judgingKeyCount) {
		size_t additionalSize = keyCount * Key_Size + judgingKeyCount * Signature_Size + (judgingKeyCount * keyCount + 7) / 8;
        auto pTransaction = CreateTransaction<TTransaction>(model::Entity_Type_EndDriveVerification, additionalSize);
        pTransaction->DriveKey = test::GenerateRandomByteArray<Key>();
        pTransaction->VerificationTrigger = test::GenerateRandomByteArray<Hash256>();
        pTransaction->ShardId = test::RandomByte();
		pTransaction->KeyCount = keyCount;
		pTransaction->JudgingKeyCount = judgingKeyCount;
        return pTransaction;
    }
}}
