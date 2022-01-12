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

#include "catapult/cache_core/ImportanceView.h"
#include "catapult/cache_core/AccountStateCache.h"
#include "catapult/model/Address.h"
#include "catapult/model/NetworkInfo.h"
#include "tests/test/cache/ImportanceViewTestUtils.h"
#include "tests/test/core/mocks/MockBlockchainConfigurationHolder.h"
#include "tests/test/other/MutableBlockchainConfiguration.h"
#include "tests/TestHarness.h"

using catapult::model::ImportanceHeight;
using catapult::model::ConvertToImportanceHeight;

namespace catapult { namespace cache {

#define TEST_CLASS ImportanceViewTests

	// region utils

	namespace {
		constexpr auto Harvesting_Mosaic_Id = MosaicId(9876);
		constexpr auto Network_Identifier = model::NetworkIdentifier::Mijin_Test;

		auto CreateConfigHolder() {
			test::MutableBlockchainConfiguration config;
			config.Network.ImportanceGrouping = 123;
			config.Network.MinHarvesterBalance = Amount(std::numeric_limits<Amount::ValueType>::max());
			return config::CreateMockConfigurationHolder(config.ToConst());
		}

		struct AddressTraits {
			static auto AddAccount(AccountStateCacheDelta& delta, const Key& publicKey, Height height) {
				auto address = model::PublicKeyToAddress(publicKey, Network_Identifier);
				delta.addAccount(address, height);
				return delta.find(address);
			}
		};

		struct PublicKeyTraits {
			static auto AddAccount(AccountStateCacheDelta& delta, const Key& publicKey, Height height) {
				delta.addAccount(publicKey, height);
				return delta.find(publicKey);
			}
		};

		struct MainAccountTraits {
			static auto AddAccount(AccountStateCacheDelta& delta, const Key& publicKey, Height height) {
				// explicitly mark the account as a main account (local harvesting when remote harvesting is enabled)
				auto accountStateIter = PublicKeyTraits::AddAccount(delta, publicKey, height);
				accountStateIter.get().AccountType = state::AccountType::Main;
				accountStateIter.get().SupplementalPublicKeys.linked().unset();
				accountStateIter.get().SupplementalPublicKeys.linked().set(test::GenerateRandomByteArray<Key>());
				return accountStateIter;
			}
		};

		struct RemoteAccountTraits {
			static auto AddAccount(AccountStateCacheDelta& delta, const Key& publicKey, Height height) {
				// 1. add the main account with a balance
				auto mainAccountPublicKey = test::GenerateRandomByteArray<Key>();
				auto mainAccountStateIter = PublicKeyTraits::AddAccount(delta, mainAccountPublicKey, height);
				mainAccountStateIter.get().AccountType = state::AccountType::Main;
				mainAccountStateIter.get().SupplementalPublicKeys.linked().unset();
				mainAccountStateIter.get().SupplementalPublicKeys.linked().set(publicKey);

				// 2. add the remote account with specified key
				auto accountStateIter = PublicKeyTraits::AddAccount(delta, publicKey, height);
				accountStateIter.get().AccountType = state::AccountType::Remote;
				accountStateIter.get().SupplementalPublicKeys.linked().unset();
				accountStateIter.get().SupplementalPublicKeys.linked().set(mainAccountPublicKey);
				return mainAccountStateIter;
			}
		};

		template<typename TTraits>
		void AddAccount(
				AccountStateCache& cache,
				const Key& publicKey,
				Amount balance = Amount(0),
				Amount lockedBalance = Amount(0)) {
			auto delta = cache.createDelta(Height{0});
			auto accountStateIter = TTraits::AddAccount(*delta, publicKey, Height(100));
			auto& accountState = accountStateIter.get();
			accountState.Balances.credit(Harvesting_Mosaic_Id, balance+lockedBalance, Height(100));
			accountState.Balances.lock(Harvesting_Mosaic_Id, lockedBalance, Height(100));
			cache.commit();
		}

		auto CreateAccountStateCache() {
			return std::make_unique<AccountStateCache>(CacheConfiguration(), AccountStateCacheTypes::Options{
				CreateConfigHolder(), Network_Identifier, MosaicId(1111), Harvesting_Mosaic_Id });
		}
	}

	// endregion

	// region tryGetAccountImportance / getAccountImportanceOrDefault

#define KEY_TRAITS_BASED_TEST(TEST_NAME) \
	template<typename TTraits> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)(); \
	TEST(TEST_CLASS, TEST_NAME##_Address) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<AddressTraits>(); } \
	TEST(TEST_CLASS, TEST_NAME##_PublicKey) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<PublicKeyTraits>(); } \
	TEST(TEST_CLASS, TEST_NAME##_MainAccount) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<MainAccountTraits>(); } \
	TEST(TEST_CLASS, TEST_NAME##_RemoteAccount) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<RemoteAccountTraits>(); } \
	template<typename TTraits> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)()

	namespace {
		void AssertCannotFindImportance(const ImportanceView& view, const Key& key, Height height) {
			// Act:
			Importance importance;
			auto foundImportance = view.tryGetAccountImportance(key, height, importance);
			auto importanceOrDefault = view.getAccountImportanceOrDefault(key, height);

			// Assert:
			EXPECT_FALSE(foundImportance);
			EXPECT_EQ(Importance(0), importanceOrDefault);
		}
	}

	KEY_TRAITS_BASED_TEST(CannotRetrieveImportanceForUnknownAccount) {
		// Arrange:
		auto key = test::GenerateRandomByteArray<Key>();
		auto height = Height(1000);
		auto pCache = CreateAccountStateCache();
		AddAccount<TTraits>(*pCache, key);
		auto pView = test::CreateImportanceView(*pCache);

		// Act + Assert: mismatched key
		AssertCannotFindImportance(*pView, test::GenerateRandomByteArray<Key>(), height);
	}

	namespace {
		template<typename TTraits>
		void AssertCanFindImportance(Importance accountImportance) {
			// Arrange:
			auto key = test::GenerateRandomByteArray<Key>();
			auto height = Height(1000);
			auto pCache = CreateAccountStateCache();
			AddAccount<TTraits>(*pCache, key, Amount(accountImportance.unwrap()));
			auto pView = test::CreateImportanceView(*pCache);

			// Act:
			Importance importance;
			auto foundImportance = pView->tryGetAccountImportance(key, height, importance);
			auto importanceOrDefault = pView->getAccountImportanceOrDefault(key, height);

			// Assert:
			EXPECT_TRUE(foundImportance);
			EXPECT_EQ(accountImportance, importance);
			EXPECT_EQ(accountImportance, importanceOrDefault);
		}
		template<typename TTraits>
		void AssertCanFindImportance(Amount unlocked, Amount locked) {
			// Arrange:
			auto key = test::GenerateRandomByteArray<Key>();
			auto height = Height(1000);
			auto pCache = CreateAccountStateCache();
			AddAccount<TTraits>(*pCache, key, unlocked, locked);
			auto pView = test::CreateImportanceView(*pCache);

			// Act:
			Importance importance;
			auto foundImportance = pView->tryGetAccountImportance(key, height, importance);
			auto importanceOrDefault = pView->getAccountImportanceOrDefault(key, height);

			// Assert:
			EXPECT_TRUE(foundImportance);
			EXPECT_EQ(unlocked+locked, Amount(importance.unwrap()));
			EXPECT_EQ(unlocked+locked, Amount(importanceOrDefault.unwrap()));
		}
	}

	KEY_TRAITS_BASED_TEST(CanRetrieveZeroImportanceFromAccount) {
		// Assert:
		AssertCanFindImportance<TTraits>(Importance(0));
	}

	KEY_TRAITS_BASED_TEST(CanRetrieveNonZeroImportanceFromAccount) {
		// Assert:
		AssertCanFindImportance<TTraits>(Importance(1234));
	}

	KEY_TRAITS_BASED_TEST(CanRetrieveNonZeroSharedImportanceFromAccount) {
		// Assert:
		AssertCanFindImportance<TTraits>(Amount(1234), Amount(1234));
	}

	// endregion

	// region canHarvest

	namespace {
		struct CanHarvestViaMemberTraits {
			static bool CanHarvest(const AccountStateCache& cache, const Key& publicKey, Height height, Amount minBalance, Amount maxBalance) {
				auto pView = test::CreateImportanceView(cache);
				return pView->canHarvest(publicKey, height, minBalance, maxBalance);
			}
		};

		struct AddressCanHarvestViaMemberTraits : public AddressTraits, public CanHarvestViaMemberTraits {};
		struct PublicKeyCanHarvestViaMemberTraits : public PublicKeyTraits, public CanHarvestViaMemberTraits {};
		struct MainAccountCanHarvestViaMemberTraits : public MainAccountTraits, public CanHarvestViaMemberTraits {};
		struct RemoteAccountCanHarvestViaMemberTraits : public RemoteAccountTraits, public CanHarvestViaMemberTraits {};
	}

#define CAN_HARVEST_TRAITS_BASED_TEST(TEST_NAME) \
	template<typename TTraits> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)(); \
	TEST(TEST_CLASS, TEST_NAME##_Address) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<AddressCanHarvestViaMemberTraits>(); } \
	TEST(TEST_CLASS, TEST_NAME##_PublicKey) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<PublicKeyCanHarvestViaMemberTraits>(); } \
	TEST(TEST_CLASS, TEST_NAME##_MainAccount) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<MainAccountCanHarvestViaMemberTraits>(); } \
	TEST(TEST_CLASS, TEST_NAME##_RemoteAccount) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<RemoteAccountCanHarvestViaMemberTraits>(); } \
	template<typename TTraits> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)()

	CAN_HARVEST_TRAITS_BASED_TEST(CannotHarvestIfAccountIsUnknown) {
		// Arrange:
		auto key = test::GenerateRandomByteArray<Key>();
		auto height = Height(1000);
		auto pCache = CreateAccountStateCache();
		AddAccount<TTraits>(*pCache, key);

		// Act + Assert:
		EXPECT_FALSE(TTraits::CanHarvest(*pCache, test::GenerateRandomByteArray<Key>(), height, Amount(1234), Amount(UINT64_MAX)));
	}

	namespace {
		template<typename TTraits>
		bool CanHarvest(int64_t minBalanceDelta, int64_t lockAmount, Height testHeight) {
			// Arrange:
			auto key = test::GenerateRandomByteArray<Key>();
			auto pCache = CreateAccountStateCache();
			auto initialBalance = Amount(static_cast<Amount::ValueType>(1234 + minBalanceDelta));
			AddAccount<TTraits>(*pCache, key, initialBalance, Amount(lockAmount));

			// Act:
			return TTraits::CanHarvest(*pCache, key, testHeight, Amount(1234), Amount(UINT64_MAX));
		}
	}

	CAN_HARVEST_TRAITS_BASED_TEST(CannotHarvestIfBalanceIsBelowMinBalance) {
		// Assert:
		auto height = Height(10000);
		EXPECT_FALSE(CanHarvest<TTraits>(-1, 0, height));
		EXPECT_FALSE(CanHarvest<TTraits>(-100, 0, height));
		EXPECT_FALSE(CanHarvest<TTraits>(-101, 100, height));
	}

	CAN_HARVEST_TRAITS_BASED_TEST(CanHarvestIfAllCriteriaAreMet) {
		// Assert:
		auto height = Height(10000);
		EXPECT_TRUE(CanHarvest<TTraits>(0, 0, height));
		EXPECT_TRUE(CanHarvest<TTraits>(-250, 250, height));
		EXPECT_TRUE(CanHarvest<TTraits>(1, 0, height));
		EXPECT_TRUE(CanHarvest<TTraits>(12345, 0, height));
		EXPECT_TRUE(CanHarvest<TTraits>(-1234, 1234, height));
		EXPECT_TRUE(CanHarvest<TTraits>(1234, 1234, height));
	}

	// endregion

	// region improper links

	namespace {
		struct TryGetTraits {
			static void Act(const ImportanceView& view, const Key& publicKey) {
				Importance importance;
				view.tryGetAccountImportance(publicKey, Height(111), importance);
			}
		};

		struct GetTraits {
			static void Act(const ImportanceView& view, const Key& publicKey) {
				view.getAccountImportanceOrDefault(publicKey, Height(111));
			}
		};

		struct CanHarvestTraits {
			static void Act(const ImportanceView& view, const Key& publicKey) {
				view.canHarvest(publicKey, Height(111), Amount(), Amount(UINT64_MAX));
			}
		};
	}

#define IMPROPER_LINKS_TRAITS_BASED_TEST(TEST_NAME) \
	template<typename TTraits> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)(); \
	TEST(TEST_CLASS, TEST_NAME##_TryGet) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<TryGetTraits>(); } \
	TEST(TEST_CLASS, TEST_NAME##_Get) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<GetTraits>(); } \
	TEST(TEST_CLASS, TEST_NAME##_CanHarvest) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<CanHarvestTraits>(); } \
	template<typename TTraits> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)()

	namespace {
		template<typename TTraits, typename TMutator>
		void AssertImproperLink(TMutator mutator) {
			// Arrange:
			auto publicKey = test::GenerateRandomByteArray<Key>();
			auto pCache = CreateAccountStateCache();

			{
				auto delta = pCache->createDelta(Height{0});
				auto accountStateIter = RemoteAccountTraits::AddAccount(*delta, publicKey, Height(100));
				mutator(accountStateIter.get());
				pCache->commit();
			}

			auto pView = test::CreateImportanceView(*pCache);

			// Act + Assert:
			EXPECT_THROW(TTraits::Act(*pView, publicKey), catapult_runtime_error);
		}
	}

	IMPROPER_LINKS_TRAITS_BASED_TEST(FailureIfLinkedAccountHasWrongType) {
		// Assert:
		AssertImproperLink<TTraits>([](auto& accountState) {
			// Arrange: change the main account to have the wrong type
			accountState.AccountType = state::AccountType::Remote;
		});
	}

	IMPROPER_LINKS_TRAITS_BASED_TEST(FailureIfLinkedAccountDoesNotReferenceRemoteAccount) {
		// Assert:
		AssertImproperLink<TTraits>([](auto& accountState) {
			// Arrange: change the main account to point to a different account
			Key temp;
			test::FillWithRandomData(temp);
		  	accountState.SupplementalPublicKeys.linked().unset();
		  	accountState.SupplementalPublicKeys.linked().set(std::move(temp));
		});
	}

	// endregion
}}
