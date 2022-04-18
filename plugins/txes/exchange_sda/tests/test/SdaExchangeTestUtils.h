/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once

#include "catapult/cache_core/AccountStateCacheDelta.h"
#include "plugins/txes/mosaic/src/cache/MosaicCache.h"
#include "catapult/utils/MemoryUtils.h"
#include "catapult/model/EntityBody.h"
#include "catapult/model/NetworkInfo.h"
#include "src/cache/SdaExchangeCache.h"
#include "src/cache/SdaExchangeCacheStorage.h"
#include "src/cache/SdaOfferGroupCache.h"
#include "src/cache/SdaOfferGroupCacheStorage.h"
#include "tests/test/cache/CacheTestUtils.h"
#include "tests/test/core/mocks/MockBlockchainConfigurationHolder.h"
#include "tests/test/core/ResolverTestUtils.h"
#include "tests/test/nodeps/Random.h"
#include "tests/test/other/MutableBlockchainConfiguration.h"

namespace catapult { namespace test {

    /// Generates an offer for calculation.
    state::SdaOfferBalance GenerateSdaOfferBalance();

    /// Creates test SDA-SDA exchange entry.
    state::SdaExchangeEntry CreateSdaExchangeEntry(uint8_t sdaOfferCount = 5, uint8_t expiredSdaOfferCount = 5, Key key = test::GenerateRandomByteArray<Key>(), VersionType version = 1);

    /// Verifies that \a offer1 is equivalent to \a offer2.
    void AssertSdaOffer(const model::SdaOffer& offer1, const model::SdaOffer& offer2);
    void AssertSdaOfferBalance(const state::SdaOfferBalance& offer1, const state::SdaOfferBalance& offer2);

    /// Verifies that \a entry1 is equivalent to \a entry2.
    void AssertEqualExchangeData(const state::SdaExchangeEntry& entry1, const state::SdaExchangeEntry& entry2);

    /// Creates a SDA-SDA exchange transaction with \a offers.
    template<typename TTransaction, typename TOffer>
    model::UniqueEntityPtr<TTransaction> CreateSdaExchangeOfferTransaction(std::initializer_list<TOffer> offers, VersionType version = 1) {
        uint32_t entitySize = sizeof(TTransaction) + offers.size() * sizeof(TOffer);
        auto pTransaction = utils::MakeUniqueWithSize<TTransaction>(entitySize);
        pTransaction->Version = model::MakeVersion(model::NetworkIdentifier::Mijin_Test, version);
        pTransaction->Signer = test::GenerateRandomByteArray<Key>();
        pTransaction->Size = entitySize;
        pTransaction->SdaOfferCount = offers.size();

        auto* pData = reinterpret_cast<uint8_t*>(pTransaction.get() + 1);
        for (const auto& offer : offers) {
            memcpy(pData, static_cast<const void*>(&offer), sizeof(TOffer));
            pData += sizeof(TOffer);
        }

        return pTransaction;
    }

    /// Cache factory for creating a catapult cache composed of offer cache and core caches.
    struct SdaExchangeCacheFactory {
    private:
        static auto CreateSubCachesWithSdaExchangeCache(const config::BlockchainConfiguration& config) {
            std::vector<size_t> cacheIds = {
                    cache::SdaExchangeCache::Id,
                    cache::SdaOfferGroupCache::Id,
                    cache::MosaicCache::Id};
            auto maxId = std::max_element(cacheIds.begin(), cacheIds.end());
            std::vector<std::unique_ptr<cache::SubCachePlugin>> subCaches(*maxId + 1);
            auto pConfigHolder = config::CreateMockConfigurationHolder(config);
            subCaches[cache::SdaExchangeCache::Id] = MakeSubCachePlugin<cache::SdaExchangeCache, cache::SdaExchangeCacheStorage>(pConfigHolder);
            subCaches[cache::SdaOfferGroupCache::Id] = MakeSubCachePlugin<cache::SdaOfferGroupCache, cache::SdaOfferGroupCacheStorage>(pConfigHolder);
            subCaches[cache::MosaicCache::Id] = MakeSubCachePlugin<cache::MosaicCache, cache::MosaicCacheStorage>();
            return subCaches;
        }

    public:
        /// Creates an empty catapult cache around default configuration.
        static cache::CatapultCache Create() {
            return Create(test::MutableBlockchainConfiguration().ToConst());
        }

        /// Creates an empty catapult cache around \a config.
        static cache::CatapultCache Create(const config::BlockchainConfiguration& config) {
            auto subCaches = CreateSubCachesWithSdaExchangeCache(config);
            CoreSystemCacheFactory::CreateSubCaches(config, subCaches);
            return cache::CatapultCache(std::move(subCaches));
        }
    };

    /// Generates an offer for arrangement.
    state::SdaOfferBasicInfo GenerateSdaOfferBasicInfo();

    /// Creates test SDA-SDA offer group entry.
    state::SdaOfferGroupEntry CreateSdaOfferGroupEntry(uint16_t offerCount = 5, Hash256 groupHash = test::GenerateRandomByteArray<Hash256>());

    /// Verifies that \a offerGroup1 is equivalent to \a offerGroup2.
    void AssertSdaOfferGroupInfo(const state::SdaOfferGroupVector& offerGroup1, const state::SdaOfferGroupVector& offerGroup2);

    /// Verifies that \a entry1 is equivalent to \a entry2.
    void AssertEqualSdaOfferGroupData(const state::SdaOfferGroupEntry& entry1, const state::SdaOfferGroupEntry& entry2);

    /// Cache factory for creating a catapult cache composed of offer cache, group offer cache and core caches.
    struct SdaOfferGroupCacheFactory {
    private:
        static auto CreateSubCachesWithSdaOfferGroupCache(const config::BlockchainConfiguration& config) {
            std::vector<size_t> cacheIds = {
                    cache::SdaExchangeCache::Id,
                    cache::SdaOfferGroupCache::Id,
                    cache::MosaicCache::Id};
            auto maxId = std::max_element(cacheIds.begin(), cacheIds.end());
            std::vector<std::unique_ptr<cache::SubCachePlugin>> subCaches(*maxId + 1);       
            auto pConfigHolder = config::CreateMockConfigurationHolder(config);
            subCaches[cache::SdaExchangeCache::Id] = MakeSubCachePlugin<cache::SdaExchangeCache, cache::SdaExchangeCacheStorage>(pConfigHolder);
            subCaches[cache::SdaOfferGroupCache::Id] = MakeSubCachePlugin<cache::SdaOfferGroupCache, cache::SdaOfferGroupCacheStorage>(pConfigHolder);
            subCaches[cache::MosaicCache::Id] = MakeSubCachePlugin<cache::MosaicCache, cache::MosaicCacheStorage>();
            return subCaches;
        }

    public:
        /// Creates an empty catapult cache around default configuration.
        static cache::CatapultCache Create() {
            return Create(test::MutableBlockchainConfiguration().ToConst());
        }

        /// Creates an empty catapult cache around \a config.
        static cache::CatapultCache Create(const config::BlockchainConfiguration& config) {
            auto subCaches = CreateSubCachesWithSdaOfferGroupCache(config);
            CoreSystemCacheFactory::CreateSubCaches(config, subCaches);
            return cache::CatapultCache(std::move(subCaches));
        }
    };

    /// Adds account state with \a owner and provided \a mosaics to \a accountStateCache at height \a height.
    void AddAccountState(cache::AccountStateCacheDelta& accountStateCache, cache::SdaExchangeCacheDelta& sdaExchangeCache, const Key& owner, const Height& height, const std::vector<model::SdaOfferMosaic>& mosaics = {});
}}


