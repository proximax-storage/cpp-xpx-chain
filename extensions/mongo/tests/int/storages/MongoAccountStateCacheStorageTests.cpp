/**
*** Copyright (c) 2016-present,
*** Jaguar0625, gimre, BloodyRookie, Tech Bureau, Corp. All rights reserved.
***
*** This file is part of Catapult.
***
*** Catapult is free software: you can redistribute it and/or modify
*** it under the terms of the GNU Lesser General Public License as published by
*** the Free Software Foundation, either version 3 of the License, or
*** (at your option) any later version.
***
*** Catapult is distributed in the hope that it will be useful,
*** but WITHOUT ANY WARRANTY; without even the implied warranty of
*** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*** GNU Lesser General Public License for more details.
***
*** You should have received a copy of the GNU Lesser General Public License
*** along with Catapult. If not, see <http://www.gnu.org/licenses/>.
**/

#include "catapult/state/StakingRecord.h"
#include "mongo/src/storages/MongoAccountStateCacheStorage.h"
#include "mongo/src/mappers/MapperUtils.h"
#include "catapult/cache_core/AccountStateCache.h"
#include "catapult/model/Address.h"
#include "catapult/model/NetworkConfiguration.h"
#include "mongo/tests/test/MapperTestUtils.h"
#include "mongo/tests/test/MongoFlatCacheStorageTests.h"
#include "tests/test/cache/CacheTestUtils.h"
#include "tests/test/core/AccountStateTestUtils.h"
#include "tests/test/other/MutableBlockchainConfiguration.h"
#include "tests/TestHarness.h"

using namespace bsoncxx::builder::stream;

namespace catapult { namespace mongo { namespace storages {

#define TEST_CLASS MongoAccountStateCacheStorageTests

	namespace {


		struct AccountStateCacheTraits {
			using CacheType = cache::AccountStateCache;
			using ModelType = state::AccountState;
			constexpr static auto Currency_Mosaic_Id = MosaicId(1234);
			static constexpr auto Collection_Name = "accounts";
			static constexpr auto Network_Id = static_cast<model::NetworkIdentifier>(0x5A);
			static constexpr auto CreateCacheStorage = CreateMongoAccountStateCacheStorage;
			static constexpr auto Additional_Collection_Name = "staking_accounts";
			static constexpr auto Id_Additional_Property_Name = "stakingAccount.address";

			static cache::CatapultCache CreateCache() {
				test::MutableBlockchainConfiguration config;
				config.Immutable.NetworkIdentifier = model::NetworkIdentifier::Mijin_Test;
				config.Immutable.HarvestingMosaicId = Currency_Mosaic_Id;
				return test::CreateEmptyCatapultCache(config.ToConst());
			}

			static ModelType GenerateRandomElement(uint32_t id) {
				auto height = Height(id);
				auto publicKey = test::GenerateRandomByteArray<Key>();
				auto address = model::PublicKeyToAddress(publicKey, model::NetworkIdentifier::Mijin_Test);
				auto accountState = state::AccountState(address, Height(1234567) + height, 2);
				accountState.PublicKey = publicKey;
				accountState.PublicKeyHeight = Height(1234567) + height;
				accountState.SupplementalPublicKeys.linked().set(test::GenerateRandomByteArray<Key>());
				auto randomAmount = Amount((test::Random() % 1'000'000 + 1'000) * 1'000'000);
				accountState.Balances.track(Currency_Mosaic_Id);
				accountState.Balances.credit(Currency_Mosaic_Id, randomAmount, accountState.PublicKeyHeight);
				accountState.Balances.commitSnapshots();
				return accountState;
			}

			static void Add(cache::CatapultCacheDelta& delta, const ModelType& accountState) {
				auto& accountStateCacheDelta = delta.sub<cache::AccountStateCache>();
				accountStateCacheDelta.addAccount(accountState.PublicKey, accountState.PublicKeyHeight, 2);

				auto& accountStateFromCache = accountStateCacheDelta.find(accountState.PublicKey).get();
				accountStateFromCache.SupplementalPublicKeys.linked().set(accountState.SupplementalPublicKeys.linked().get());
				accountStateFromCache.Balances.credit(Currency_Mosaic_Id, accountState.Balances.get(Currency_Mosaic_Id), accountState.PublicKeyHeight);
				accountStateFromCache.Balances.commitSnapshots();
			}

			static void Remove(cache::CatapultCacheDelta& delta, const ModelType& accountState) {
				auto& accountStateCacheDelta = delta.sub<cache::AccountStateCache>();
				accountStateCacheDelta.queueRemove(accountState.PublicKey, accountState.PublicKeyHeight);
				accountStateCacheDelta.commitRemovals();
			}

			static void Mutate(cache::CatapultCacheDelta& delta, ModelType& accountState) {
				// update expected
				accountState.Balances.credit(Currency_Mosaic_Id, Amount(12'345'000'000), accountState.AddressHeight + Height(1));

				// update cache
				auto& accountStateCacheDelta = delta.sub<cache::AccountStateCache>();
				auto& accountStateFromCache = accountStateCacheDelta.find(accountState.PublicKey).get();
				accountStateFromCache.Balances.credit(Currency_Mosaic_Id, Amount(12'345'000'000), accountState.AddressHeight + Height(1));
			}

			static void AddCb(cache::CatapultCacheDelta& delta, ModelType& accountState) {
				Add(delta, accountState);
				auto& accountStateCacheDelta = delta.sub<cache::AccountStateCache>();
				auto& accountStateFromCache = accountStateCacheDelta.find(accountState.PublicKey).get();
				accountState.Balances.lock(Currency_Mosaic_Id, Amount(1000), accountState.AddressHeight+ Height(1));
				accountStateFromCache.Balances.lock(Currency_Mosaic_Id, Amount(1000), accountState.AddressHeight+ Height(1));
				accountState.Balances.commitSnapshots();
				accountStateFromCache.Balances.commitSnapshots();
			}

			static void RemoveCb(cache::CatapultCacheDelta& delta, const ModelType& accountState) {
				Remove(delta, accountState);
			}

			static void MutateCb(cache::CatapultCacheDelta& delta, ModelType& accountState) {
				Mutate(delta, accountState);
				auto& accountStateCacheDelta = delta.sub<cache::AccountStateCache>();
				auto& accountStateFromCache = accountStateCacheDelta.find(accountState.PublicKey).get();
				accountStateFromCache.Balances.lock(Currency_Mosaic_Id, Amount(1000), accountState.AddressHeight+ Height(1));
				accountStateFromCache.Balances.commitSnapshots();

				accountState.Balances.lock(Currency_Mosaic_Id, Amount(1000), accountState.AddressHeight+ Height(1));
				accountState.Balances.commitSnapshots();
			}

			static auto GetFindFilter(const ModelType& accountState) {
				return document() << "account.address" << mappers::ToBinary(accountState.Address) << finalize;
			}

			static auto GetAdditionalFindFilter(const state::StakingRecord& stakingRecord) {
				return document() << std::string(Id_Additional_Property_Name) << mappers::ToBinary(stakingRecord.Address) << finalize;
			}

			static void AssertEqual(const ModelType& accountState, const bsoncxx::document::view& view) {
				test::AssertEqualAccountState(accountState, view["account"].get_document().view());
			}

			static void AssertEqual(const state::StakingRecord& stakingRecord, const bsoncxx::document::view& view) {
				test::AssertEqualStakingRecord(stakingRecord, view["stakingAccount"].get_document().view());
			}

			static void AssertAddedCallbackExecution(std::vector<ModelType> modifiedRecords, std::vector<ModelType> expectedRecords)
			{
				std::vector<state::StakingRecord> records;
				for(const auto& record : expectedRecords)
				{
					records.emplace_back(record, Currency_Mosaic_Id);
				}
				test::MongoCacheStorageTestUtils<AccountStateCacheTraits>::AssertDbContents(Additional_Collection_Name, records, GetAdditionalFindFilter);
			}

			static void AssertRemovedCallbackExecution(std::vector<ModelType> modifiedRecords, std::vector<ModelType> expectedRecords)
			{
				std::vector<state::StakingRecord> records;
				for(const auto& record : expectedRecords)
				{
					records.emplace_back(record, Currency_Mosaic_Id);
				}
				test::MongoCacheStorageTestUtils<AccountStateCacheTraits>::AssertDbContents(Additional_Collection_Name, records, GetAdditionalFindFilter);
			}
		};
	}

	DEFINE_FLAT_CACHE_STORAGE_TESTS_WITH_CALLBACKS(AccountStateCacheTraits,)

}}}
