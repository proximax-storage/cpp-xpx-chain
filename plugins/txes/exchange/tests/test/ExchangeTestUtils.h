/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/utils/MemoryUtils.h"
#include "catapult/model/EntityBody.h"
#include "catapult/model/NetworkInfo.h"
#include "src/cache/ExchangeCache.h"
#include "src/cache/ExchangeCacheStorage.h"
#include "tests/test/cache/CacheTestUtils.h"
#include "tests/test/core/mocks/MockBlockchainConfigurationHolder.h"
#include "tests/test/nodeps/Random.h"
#include "tests/test/other/MutableBlockchainConfiguration.h"

namespace catapult { namespace test {

	/// Generates a base offer.
	state::OfferBase GenerateOffer();

	/// Creates test exchange entry.
	state::ExchangeEntry CreateExchangeEntry(uint8_t offerCount = 5, uint8_t expiredOfferCount = 5, Key key = test::GenerateRandomByteArray<Key>());

	/// Verifies that \a offer1 is equivalent to \a offer2.
	void AssertOffer(const model::Offer& offer1, const model::Offer& offer2);
	void AssertOffer(const state::OfferBase& offer1, const state::OfferBase& offer2);
	void AssertOffer(const state::BuyOffer& offer1, const state::BuyOffer& offer2);
	void AssertOffer(const state::SellOffer& offer1, const state::SellOffer& offer2);

	/// Verifies that \a entry1 is equivalent to \a entry2.
	void AssertEqualExchangeData(const state::ExchangeEntry& entry1, const state::ExchangeEntry& entry2);

    /// Creates an exchange transaction with \a offers.
    template<typename TTransaction, typename TOffer>
	model::UniqueEntityPtr<TTransaction> CreateExchangeTransaction(std::initializer_list<TOffer> offers) {
        uint32_t entitySize = sizeof(TTransaction) + offers.size() * sizeof(TOffer);
        auto pTransaction = utils::MakeUniqueWithSize<TTransaction>(entitySize);
		pTransaction->Version = model::MakeVersion(model::NetworkIdentifier::Mijin_Test, 1);
		pTransaction->Signer = test::GenerateRandomByteArray<Key>();
        pTransaction->Size = entitySize;
		pTransaction->OfferCount = offers.size();

        auto* pData = reinterpret_cast<uint8_t*>(pTransaction.get() + 1);
        for (const auto& offer : offers) {
            memcpy(pData, static_cast<const void*>(&offer), sizeof(TOffer));
            pData += sizeof(TOffer);
        }

        return pTransaction;
    }

	/// Cache factory for creating a catapult cache composed of offer cache and core caches.
	struct ExchangeCacheFactory {
	private:
		static auto CreateSubCachesWithExchangeCache(const config::BlockchainConfiguration& config) {
			auto cacheId = cache::ExchangeCache::Id;
			std::vector<std::unique_ptr<cache::SubCachePlugin>> subCaches(cacheId + 1);
			auto pConfigHolder = config::CreateMockConfigurationHolder(config);
			subCaches[cacheId] = MakeSubCachePlugin<cache::ExchangeCache, cache::ExchangeCacheStorage>(pConfigHolder);
			return subCaches;
		}

	public:
		/// Creates an empty catapult cache around default configuration.
		static cache::CatapultCache Create() {
			return Create(test::MutableBlockchainConfiguration().ToConst());
		}

		/// Creates an empty catapult cache around \a config.
		static cache::CatapultCache Create(const config::BlockchainConfiguration& config) {
			auto subCaches = CreateSubCachesWithExchangeCache(config);
			CoreSystemCacheFactory::CreateSubCaches(config, subCaches);
			return cache::CatapultCache(std::move(subCaches));
		}
	};
}}


