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
        Key consumer = test::GenerateRandomByteArray<Key>()
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
			uint8_t count,
			std::vector<std::pair<Key, crypto::BLSKeyPair>>& replicatorKeyPairs,
			const Height& height = Height(10));

	/// Common fields of opinion-based multisignature transactions (DownloadApproval, DataModificationApproval, EndDriveVerification).
	template<typename TOpinion>
	struct OpinionData {
		uint8_t OpinionCount;
		uint8_t JudgingCount;
		uint8_t JudgedCount;
		std::vector<Key> PublicKeys;
		std::vector<uint8_t> OpinionIndices;
		std::vector<BLSSignature> BlsSignatures;
		std::vector<bool> PresentOpinions;
		std::vector<TOpinion> Opinions;
	};

	/// Creates an OpinionData filled with valid data.
	template<typename TOpinion>
	OpinionData<TOpinion> CreateValidOpinionData(
			const std::vector<std::pair<Key, crypto::BLSKeyPair>>& replicatorKeyPairs,
			uint8_t* const pCommonDataBegin,
			const size_t commonDataSize) {
		OpinionData<TOpinion> data;

		// Generating valid counts.
		data.JudgedCount = test::RandomInRange(static_cast<size_t>(1), replicatorKeyPairs.size());
		data.JudgingCount = test::RandomInRange(static_cast<uint8_t>(1), data.JudgedCount);
		data.OpinionCount = test::RandomInRange(static_cast<uint8_t>(1), data.JudgingCount);
		const auto presentOpinionCount = data.OpinionCount * data.JudgedCount;
		auto opinionElementCount = 0;

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

		// Filling PresentOpinions.
		data.PresentOpinions.reserve(presentOpinionCount);
		std::vector<uint8_t> predPresenceIndices;	// Vector of predetermined indices used to guarantee that every key is used. If Nth element in predPresenceIndices is M, then Mth element in Nth column of presentOpinions must be true.
		predPresenceIndices.reserve(data.JudgedCount);
		for (auto i = 0u; i < data.JudgedCount; ++i) predPresenceIndices.emplace_back(test::RandomInRange(0, data.OpinionCount-1));
		for (auto i = 0u; i < presentOpinionCount; ++i) {
			const bool bit = (predPresenceIndices.at(i % data.JudgedCount) == i / data.JudgedCount) || test::Random() % 10;	// True with probability 0.9
			opinionElementCount += bit;
			data.PresentOpinions.push_back(bit);
		}

		// Filling Opinions.
		data.Opinions = test::GenerateUniqueRandomDataVector<TOpinion>(opinionElementCount);	// TODO: Not necessarily unique

		// Filling BlsSignatures.
		const auto maxDataSize = commonDataSize + (sizeof(Key) + sizeof(TOpinion)) * data.JudgedCount;	// Guarantees that every possible individual opinion will fit in.
		auto* const pDataBegin = new uint8_t[maxDataSize];	// TODO: Make smart pointer
		memcpy(pDataBegin, pCommonDataBegin, commonDataSize);
		auto* const pIndividualDataBegin = pDataBegin + commonDataSize;

		using OpinionElement = std::pair<Key, TOpinion>;
		const auto comparator = [](const OpinionElement& a, const OpinionElement& b){ return a.first < b.first; };
		std::set<OpinionElement, decltype(comparator)> individualPart(comparator);	// Set that represents complete opinion of one of the replicators. Opinion elements are sorted in ascending order of keys.
		for (auto i = 0, k = 0; i < data.OpinionCount; ++i) {
			individualPart.clear();
			for (auto j = 0; j < data.JudgedCount; ++j) {
				if (data.PresentOpinions.at(i * data.JudgedCount + j)) {
					individualPart.emplace(data.PublicKeys.at(j), data.Opinions.at(k));
					++k;
				}
			}

			const auto dataSize = commonDataSize + (sizeof(Key) + sizeof(TOpinion)) * individualPart.size();
			auto* pIndividualData = pIndividualDataBegin;
			for (const auto& opinionElement : individualPart) {
				utils::WriteToByteArray(pIndividualData, opinionElement.first);
				utils::WriteToByteArray(pIndividualData, opinionElement.second);
			}

			RawBuffer dataBuffer(pDataBegin, dataSize);

			const auto blsSignaturesCount = blsKeyPairs.at(i).size();
			auto* const pBlsSignatures = new BLSSignature[blsSignaturesCount];	// TODO: Make smart pointer
			std::vector<const BLSSignature*> signatures;
			signatures.reserve(blsSignaturesCount);
			for (auto j = 0u; j < blsSignaturesCount; ++j) {
				catapult::crypto::Sign(*blsKeyPairs.at(i).at(j), dataBuffer, pBlsSignatures[j]);
				signatures.push_back(&pBlsSignatures[j]);
			}

			data.BlsSignatures.push_back(catapult::crypto::Aggregate(signatures));
			delete pBlsSignatures;
		}
		delete pDataBegin;

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

	/// Creates a download approval transaction.
	template<typename TTransaction>
	model::UniqueEntityPtr<TTransaction> CreateDownloadApprovalTransaction(
			const Hash256& downloadChannelId,
			const uint16_t sequenceNumber,
			const bool response,
			const OpinionData<uint64_t>& opinionData) {
		const auto presentOpinionByteCount = (opinionData.OpinionCount * opinionData.JudgedCount + 7) / 8;
		const auto bodySize = sizeof(TTransaction);
		const auto payloadSize = opinionData.JudgedCount * sizeof(Key)
								 + opinionData.JudgingCount * sizeof(uint8_t)
								 + opinionData.OpinionCount * sizeof(BLSSignature)
								 + presentOpinionByteCount * sizeof(uint8_t)
								 + opinionData.Opinions.size() * sizeof(uint64_t);
		uint32_t entitySize = bodySize + payloadSize;
		auto pTransaction = utils::MakeUniqueWithSize<TTransaction>(entitySize);
		pTransaction->Signer = test::GenerateRandomByteArray<Key>();
		pTransaction->Version = model::MakeVersion(model::NetworkIdentifier::Mijin_Test, 1);
		pTransaction->Type = model::Entity_Type_DownloadApproval;
		pTransaction->Size = entitySize;

		pTransaction->DownloadChannelId = downloadChannelId;
		pTransaction->SequenceNumber = sequenceNumber;
		pTransaction->ResponseToFinishDownloadTransaction = response;
		pTransaction->OpinionCount = opinionData.OpinionCount;
		pTransaction->JudgingCount = opinionData.JudgingCount;
		pTransaction->JudgedCount = opinionData.JudgedCount;
		pTransaction->OpinionElementCount = opinionData.Opinions.size();

		auto* const pPublicKeysBegin = reinterpret_cast<Key*>(pTransaction.get() + 1);
		for (auto i = 0u; i < opinionData.PublicKeys.size(); ++i)
			memcpy(static_cast<void*>(&pPublicKeysBegin[i]), static_cast<const void*>(&opinionData.PublicKeys.at(i)), sizeof(Key));

		auto* const pOpinionIndicesBegin = reinterpret_cast<uint8_t*>(pPublicKeysBegin + opinionData.PublicKeys.size());
		for (auto i = 0u; i < opinionData.OpinionIndices.size(); ++i)
			memcpy(static_cast<void*>(&pOpinionIndicesBegin[i]), static_cast<const void*>(&opinionData.OpinionIndices.at(i)), sizeof(uint8_t));

		auto* const pBlsSignaturesBegin = reinterpret_cast<BLSSignature*>(pOpinionIndicesBegin + opinionData.OpinionIndices.size());
		for (auto i = 0u; i < opinionData.BlsSignatures.size(); ++i)
			memcpy(static_cast<void*>(&pBlsSignaturesBegin[i]), static_cast<const void*>(&opinionData.BlsSignatures.at(i)), sizeof(BLSSignature));

		auto* const pPresentOpinionsBegin = reinterpret_cast<uint8_t*>(pBlsSignaturesBegin + opinionData.BlsSignatures.size());
		for (auto i = 0u; i < presentOpinionByteCount; ++i) {
			boost::dynamic_bitset<uint8_t> byte(8, 0u);
			for (auto j = 0u; j < std::min(8ul, opinionData.PresentOpinions.size() - i*8); ++j)
				byte[j] = opinionData.PresentOpinions.at(j + i*8);
			boost::to_block_range(byte, &pPresentOpinionsBegin[i]);
		}

		auto* const pOpinionsBegin = reinterpret_cast<uint64_t*>(pPresentOpinionsBegin + presentOpinionByteCount);
		for (auto i = 0u; i < opinionData.Opinions.size(); ++i)
			memcpy(static_cast<void*>(&pOpinionsBegin[i]), static_cast<const void*>(&opinionData.Opinions.at(i)), sizeof(uint64_t));

		return pTransaction;
	}
}}
