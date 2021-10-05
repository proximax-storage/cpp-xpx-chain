/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include <random>
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
#include "src/cache/ReplicatorCache.h"
#include "src/cache/ReplicatorCacheStorage.h"
#include "src/cache/BlsKeysCache.h"
#include "src/cache/BlsKeysCacheStorage.h"
#include "src/model/StorageEntityType.h"
#include "src/utils/StorageUtils.h"
#include "tests/test/cache/CacheTestUtils.h"
#include "tests/test/core/mocks/MockBlockchainConfigurationHolder.h"
#include "tests/test/other/MutableBlockchainConfiguration.h"
#include "tests/test/nodeps/Random.h"

namespace catapult { namespace test {

	/// Cache factory for creating a catapult cache composed of BC drive cache, BLS keys cache, download channel cache and replicator cache.
	struct StorageCacheFactory {
	private:
		static auto CreateStorageSubCaches(const config::BlockchainConfiguration& config) {
			std::vector<size_t> cacheIds = {
					cache::BcDriveCache::Id,
					cache::BlsKeysCache::Id,
					cache::DownloadChannelCache::Id,
					cache::ReplicatorCache::Id};
			auto maxId = std::max_element(cacheIds.begin(), cacheIds.end());
			std::vector<std::unique_ptr<cache::SubCachePlugin>> subCaches(*maxId + 1);
			auto pConfigHolder = config::CreateMockConfigurationHolder(config);
			auto pKeyCollector = std::make_shared<cache::ReplicatorKeyCollector>();
			subCaches[cache::BcDriveCache::Id] = MakeSubCachePlugin<cache::BcDriveCache, cache::BcDriveCacheStorage>(pConfigHolder);
			subCaches[cache::BlsKeysCache::Id] = MakeSubCachePlugin<cache::BlsKeysCache, cache::BlsKeysCacheStorage>(pConfigHolder);
			subCaches[cache::DownloadChannelCache::Id] = MakeSubCachePlugin<cache::DownloadChannelCache, cache::DownloadChannelCacheStorage>(pConfigHolder);
			subCaches[cache::ReplicatorCache::Id] = MakeSubCachePlugin<cache::ReplicatorCache, cache::ReplicatorCacheStorage>(pKeyCollector, pConfigHolder);
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
                auto pKeyCollector = std::make_shared<cache::ReplicatorKeyCollector>();
			    subCaches[cache::BcDriveCache::Id] = MakeSubCachePlugin<cache::BcDriveCache, cache::BcDriveCacheStorage>(pConfigHolder);
			    subCaches[cache::DownloadChannelCache::Id] = MakeSubCachePlugin<cache::DownloadChannelCache, cache::DownloadChannelCacheStorage>(pConfigHolder);
                subCaches[cache::ReplicatorCache::Id] = MakeSubCachePlugin<cache::ReplicatorCache, cache::ReplicatorCacheStorage>(pKeyCollector, pConfigHolder);
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
		uint16_t downloadApprovalCount = test::Random16()
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
                auto pKeyCollector = std::make_shared<cache::ReplicatorKeyCollector>();
			    subCaches[cache::BcDriveCache::Id] = MakeSubCachePlugin<cache::BcDriveCache, cache::BcDriveCacheStorage>(pConfigHolder);
			    subCaches[cache::DownloadChannelCache::Id] = MakeSubCachePlugin<cache::DownloadChannelCache, cache::DownloadChannelCacheStorage>(pConfigHolder);
                subCaches[cache::ReplicatorCache::Id] = MakeSubCachePlugin<cache::ReplicatorCache, cache::ReplicatorCacheStorage>(pKeyCollector, pConfigHolder);
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
		BLSPublicKey blsKey = test::GenerateRandomByteArray<BLSPublicKey>(),
        Amount capacity = test::GenerateRandomValue<Amount>(),
		uint16_t drivesCount = 2
    );

    /// Verifies that \a entry1 is equivalent to \a entry2.
    void AssertEqualReplicatorData(const state::ReplicatorEntry& expectedEntry, const state::ReplicatorEntry& entry);

     /// Cache factory for creating a catapult cache composed of replicator cache and core caches.
    struct ReplicatorCacheFactory {
        private:
            static auto CreateSubCachesWithDriveCache(const config::BlockchainConfiguration& config) {
                auto id = std::max(cache::BcDriveCache::Id, std::max(cache::DownloadChannelCache::Id, cache::ReplicatorCache::Id));
                std::vector<std::unique_ptr<cache::SubCachePlugin>> subCaches(id + 1);
			    auto pConfigHolder = config::CreateMockConfigurationHolder(config);
                auto pKeyCollector = std::make_shared<cache::ReplicatorKeyCollector>();
			    subCaches[cache::BcDriveCache::Id] = MakeSubCachePlugin<cache::BcDriveCache, cache::BcDriveCacheStorage>(pConfigHolder);
			    subCaches[cache::DownloadChannelCache::Id] = MakeSubCachePlugin<cache::DownloadChannelCache, cache::DownloadChannelCacheStorage>(pConfigHolder);
                subCaches[cache::ReplicatorCache::Id] = MakeSubCachePlugin<cache::ReplicatorCache, cache::ReplicatorCacheStorage>(pKeyCollector, pConfigHolder);
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

	/// Creates test BLS keys entry.
	state::BlsKeysEntry CreateBlsKeysEntry(
			const BLSPublicKey& blsKey = test::GenerateRandomByteArray<BLSPublicKey>(),
			const Key& key = test::GenerateRandomByteArray<Key>());

	/// Adds \a count unique replicator entries and BLS key entries to respective caches.
	/// Adds the keys of created replicators to \a keys.
	void AddReplicators(
			cache::CatapultCache& cache,
			std::vector<std::pair<Key, crypto::BLSKeyPair>>& replicatorKeyPairs,
			uint8_t count,
			Height height = Height(1));

	/// Common fields of opinion-based multisignature transactions (DownloadApproval, DataModificationApproval, EndDriveVerification).
	template<typename TOpinion>
	struct OpinionData {
		uint8_t OpinionCount;
		uint8_t JudgingCount;
		uint8_t JudgedCount;
		uint8_t OpinionElementCount;
		uint8_t FilledPresenceRowIndex;
		std::vector<Key> PublicKeys;
		std::vector<uint8_t> OpinionIndices;
		std::vector<BLSSignature> BlsSignatures;
		std::vector<std::vector<bool>> PresentOpinions;
		std::vector<std::vector<TOpinion>> Opinions;
	};

	/// Creates an OpinionData filled with valid data.
	template<typename TOpinion>
	OpinionData<TOpinion> CreateValidOpinionData(
			const std::vector<std::pair<Key, crypto::BLSKeyPair>>& replicatorKeyPairs,
			const RawBuffer& commonDataBuffer,
			const uint8_t judgedCount = 0,
			const uint8_t judgingCount = 0,
			const uint8_t opinionCount = 0,
			const bool filledPresenceRow = false) {
		OpinionData<TOpinion> data;

		// Generating valid counts.
		data.JudgedCount = judgedCount ? judgedCount : test::RandomInRange<uint8_t>(1, replicatorKeyPairs.size());
		data.JudgingCount = judgingCount ? judgingCount : test::RandomInRange<uint8_t>(1, data.JudgedCount);
		data.OpinionCount = opinionCount ? opinionCount : test::RandomInRange<uint8_t>(1, data.JudgingCount);
		const auto presentOpinionCount = data.OpinionCount * data.JudgedCount;

		// Filling PublicKeys and OpinionIndices.
		std::vector<std::vector<const crypto::BLSKeyPair*>> blsKeyPairs(data.OpinionCount);	// Nth vector in blsKeyPairs contains pointers to all BLS key pairs of replicators that provided Nth opinion.
		data.OpinionIndices.reserve(data.JudgingCount);
		std::vector<uint8_t> predOpinionIndices(boost::counting_iterator<uint8_t>(0u), boost::counting_iterator<uint8_t>(data.JudgingCount));	// Vector of predetermined indices used to guarantee that every opinion is used. If Nth element in predOpinionIndices is M and M < opinionCount, then Nth element in opinionIndices must be M.
		std::shuffle(std::begin(predOpinionIndices), std::end(predOpinionIndices), std::default_random_engine {});
		for (auto i = 0u; i < data.JudgedCount; ++i) {
			const auto& pair = replicatorKeyPairs.at(i);
			data.PublicKeys.push_back(pair.first);
			if (i < data.JudgingCount) {
				const auto opinionIndex = predOpinionIndices.at(i) < data.OpinionCount ? predOpinionIndices.at(i) : test::RandomInRange(0, data.OpinionCount-1);
				data.OpinionIndices.push_back(opinionIndex);
				blsKeyPairs.at(opinionIndex).push_back(&pair.second);
			}
		}

		// Filling PresentOpinions and Opinions.
		data.PresentOpinions.resize(data.OpinionCount);
		data.Opinions.resize(data.OpinionCount);
		std::vector<uint8_t> predPresenceIndices;	// Vector of predetermined indices used to guarantee that every key is used. If Nth element in predPresenceIndices is M, then Mth element in Nth column of presentOpinions must be true.
		predPresenceIndices.reserve(data.JudgedCount);
		data.FilledPresenceRowIndex = test::RandomInRange(0, std::max(data.OpinionCount-2, 0));	// The row in PresentOpinions filled with 1's is guaranteed not to be the last.
		for (auto i = 0u; i < data.JudgedCount; ++i)
			predPresenceIndices.emplace_back(test::RandomInRange(0, data.OpinionCount-1));
		for (auto i = 0u; i < data.OpinionCount; ++i) {
			data.PresentOpinions.at(i).reserve(data.JudgedCount);
			for (auto j = 0u; j < data.JudgedCount; ++j) {
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

		// Filling BlsSignatures.
		const auto maxDataSize = commonDataBuffer.Size + (sizeof(Key) + sizeof(TOpinion)) * data.JudgedCount;	// Guarantees that every possible individual opinion will fit in.
		const auto pDataBegin = std::unique_ptr<uint8_t[]>(new uint8_t[maxDataSize]);
		memcpy(pDataBegin.get(), commonDataBuffer.pData, commonDataBuffer.Size);
		auto* const pIndividualDataBegin = pDataBegin.get() + commonDataBuffer.Size;

		using OpinionElement = std::pair<Key, TOpinion>;
		const auto comparator = [](const OpinionElement& a, const OpinionElement& b){ return a.first < b.first; };
		std::set<OpinionElement, decltype(comparator)> individualPart(comparator);	// Set that represents complete opinion of one of the replicators. Opinion elements are sorted in ascending order of keys.
		for (auto i = 0; i < data.OpinionCount; ++i) {
			individualPart.clear();
			for (auto j = 0, k = 0; j < data.JudgedCount; ++j) {
				if (data.PresentOpinions.at(i).at(j)) {
					individualPart.emplace(data.PublicKeys.at(j), data.Opinions.at(i).at(k));
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

			const auto blsSignaturesCount = blsKeyPairs.at(i).size();
			const auto pBlsSignatures = std::unique_ptr<BLSSignature[]>(new BLSSignature[blsSignaturesCount]);
			std::vector<const BLSSignature*> signatures;
			signatures.reserve(blsSignaturesCount);
			for (auto j = 0u; j < blsSignaturesCount; ++j) {
				catapult::crypto::Sign(*blsKeyPairs.at(i).at(j), dataBuffer, pBlsSignatures[j]);
				signatures.push_back(&pBlsSignatures[j]);
			}

			data.BlsSignatures.push_back(catapult::crypto::Aggregate(signatures));
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
        pTransaction->UploadSize = test::Random();
        return pTransaction;
    }

    /// Creates a download transaction.
    template<typename TTransaction>
    model::UniqueEntityPtr<TTransaction> CreateDownloadTransaction() {
        auto pTransaction = CreateTransaction<TTransaction>(model::Entity_Type_Download);
        pTransaction->DownloadSize = test::Random();
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
}}
