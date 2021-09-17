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

#include "harvesting/src/UnlockedAccounts.h"
#include "tests/test/core/AddressTestUtils.h"
#include "tests/test/core/KeyPairTestUtils.h"
#include "tests/test/nodeps/LockTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace harvesting {

#define TEST_CLASS UnlockedAccountsTests

	namespace {
		struct AccountDescriptorWrapper {
		public:
			BlockGeneratorAccountDescriptor Descriptor;
			Key SigningPublicKey;
			Key VrfPublicKey;

		public:
			AccountDescriptorWrapper(uint32_t accountVersion)
					: Descriptor(test::GenerateKeyPair(accountVersion), test::GenerateVrfKeyPair(), accountVersion)
					, SigningPublicKey(Descriptor.signingKeyPair().publicKey())
					, VrfPublicKey(Descriptor.vrfKeyPair().publicKey())
			{}
		};

		AccountDescriptorWrapper GenerateAccountDescriptorWrapper(uint32_t accountVersion) {
			return AccountDescriptorWrapper(accountVersion);
		}

		struct TestContext {
		public:
			explicit TestContext(size_t maxSize) : Accounts(maxSize, [this](const auto& publicKey) {
			  auto iter = CustomPrioritizationMap.find(publicKey);
			  return CustomPrioritizationMap.cend() != iter ? iter->second : 0;
			})
			{}

		public:
			UnlockedAccounts Accounts;
			std::unordered_map<Key, size_t, utils::ArrayHasher<Key>> CustomPrioritizationMap;
		};

		UnlockedAccountsAddResult AddAccount(TestContext& context, BlockGeneratorAccountDescriptor&& descriptor) {
			return context.Accounts.modifier().add(std::move(descriptor));
		}

		UnlockedAccountsAddResult AddRandomAccount(TestContext& context, uint32_t accountVersion) {
			return AddAccount(context, GenerateAccountDescriptorWrapper(accountVersion).Descriptor);
		}

	}

	namespace {
		std::pair<Key, size_t> GetPrioritizedVrfPublicKey(const UnlockedAccountsView& accountsView, const Key& signingPublicKey) {
			auto index = 0u;
			auto vrfPublicKey = Key();
			accountsView.forEach([&signingPublicKey, &index, &vrfPublicKey](const auto& descriptor) {
			  if (descriptor.signingKeyPair().publicKey() != signingPublicKey) {
				  ++index;
				  return true;
			  }

			  vrfPublicKey = descriptor.vrfKeyPair().publicKey();
			  return false;
			});

			return std::make_pair(vrfPublicKey, index);
		}

		Key GetVrfPublicKey(const UnlockedAccountsView& accountsView, const Key& signingPublicKey) {
			return GetPrioritizedVrfPublicKey(accountsView, signingPublicKey).first;
		}
	}

	namespace {
		using test_types = ::testing::Types<
				std::integral_constant<uint32_t,1>,
				std::integral_constant<uint32_t,2>>;

		template<typename TBaseAccountVersion>
		struct UnlockedAccountsTest : public ::testing::Test {};
	}

	TYPED_TEST_CASE(UnlockedAccountsTest, test_types);

	TEST(TEST_CLASS, InitiallyContainerIsEmpty) {
		// Arrange:
		TestContext context(8);
		const auto& accounts = context.Accounts;

		// Assert:
		EXPECT_EQ(0u, accounts.view().size());
	}

	TYPED_TEST(UnlockedAccountsTest, CanAddHarvestingEligibleAccount) {
		// Arrange:
		auto accountDescriptorWrapper = GenerateAccountDescriptorWrapper(TypeParam::value);
		TestContext context(8);
		auto& accounts = context.Accounts;

		// Act:
		auto result = accounts.modifier().add(std::move(accountDescriptorWrapper.Descriptor));

		// Assert:
		auto view = accounts.view();
		EXPECT_EQ(UnlockedAccountsAddResult::Success_New, result);
		EXPECT_EQ(1u, view.size());
		EXPECT_TRUE(view.contains(accountDescriptorWrapper.SigningPublicKey));

		// - vrf public key is properly associated
		EXPECT_FALSE(view.contains(accountDescriptorWrapper.VrfPublicKey));
		EXPECT_EQ(accountDescriptorWrapper.VrfPublicKey, GetVrfPublicKey(view, accountDescriptorWrapper.SigningPublicKey));
	}

	TYPED_TEST(UnlockedAccountsTest, CanAddMultipleAccountsToContainer) {
		// Arrange(ONLY ONE ACCOUNT CAN BE V1 IN ANY UNLOCKEDACCOUNTS CONTAINER):
		auto accountDescriptorWrapper1 = GenerateAccountDescriptorWrapper(TypeParam::value);
		auto accountDescriptorWrapper2 = GenerateAccountDescriptorWrapper(2);
		TestContext context(8);
		auto& accounts = context.Accounts;

		// Act:
		{
			auto modifier = accounts.modifier();
			modifier.add(std::move(accountDescriptorWrapper1.Descriptor));
			modifier.add(std::move(accountDescriptorWrapper2.Descriptor));
		}

		// Assert:
		auto view = accounts.view();
		EXPECT_EQ(2u, view.size());
		EXPECT_TRUE(view.contains(accountDescriptorWrapper1.SigningPublicKey));
		EXPECT_TRUE(view.contains(accountDescriptorWrapper2.SigningPublicKey));

		// - vrf public keys are properly associated
		EXPECT_FALSE(view.contains(accountDescriptorWrapper1.VrfPublicKey));
		EXPECT_FALSE(view.contains(accountDescriptorWrapper2.VrfPublicKey));
		EXPECT_EQ(accountDescriptorWrapper1.VrfPublicKey, GetVrfPublicKey(view, accountDescriptorWrapper1.SigningPublicKey));
		EXPECT_EQ(accountDescriptorWrapper2.VrfPublicKey, GetVrfPublicKey(view, accountDescriptorWrapper2.SigningPublicKey));
	}

	TYPED_TEST(UnlockedAccountsTest, AdditionOfAlreadyAddedAccountUpdatesVrfPublicKey) {
		// Arrange:
		auto signingKeyPair = test::GenerateKeyPair(TypeParam::value);
		auto vrfKeyPair1 = test::GenerateVrfKeyPair();
		auto vrfKeyPair2 = test::GenerateVrfKeyPair();

		TestContext context(8);
		auto& accounts = context.Accounts;

		// Act:
		auto result1 = accounts.modifier().add(BlockGeneratorAccountDescriptor(
				test::CopyKeyPair(signingKeyPair),
				test::CopyKeyPair(vrfKeyPair1), TypeParam::value));
		auto result2 = accounts.modifier().add(BlockGeneratorAccountDescriptor(
				test::CopyKeyPair(signingKeyPair),
				test::CopyKeyPair(vrfKeyPair2), TypeParam::value));

		// Assert:
		auto view = accounts.view();
		EXPECT_EQ(UnlockedAccountsAddResult::Success_New, result1);
		EXPECT_EQ(UnlockedAccountsAddResult::Success_Update, result2);
		EXPECT_EQ(1u, view.size());
		EXPECT_TRUE(view.contains(signingKeyPair.publicKey()));

		// - last vrf public key is properly associated
		EXPECT_FALSE(view.contains(vrfKeyPair1.publicKey()));
		EXPECT_FALSE(view.contains(vrfKeyPair2.publicKey()));
		EXPECT_EQ(vrfKeyPair2.publicKey(), GetVrfPublicKey(view, signingKeyPair.publicKey()));
	}

	TYPED_TEST(UnlockedAccountsTest, AdditionOfAlreadyAddedAccountUpdatesVrfPublicKeyAndPreservesPriority) {
		// Arrange:
		auto signingKeyPair1 = test::GenerateKeyPair(TypeParam::value);
		auto signingKeyPair2 = test::GenerateKeyPair(2);
		auto vrfKeyPair1 = test::GenerateVrfKeyPair();
		auto vrfKeyPair2 = test::GenerateVrfKeyPair();

		TestContext context(8);
		auto& accounts = context.Accounts;

		// - add account 1 with priority 3
		context.CustomPrioritizationMap[signingKeyPair1.publicKey()] = 3;
		accounts.modifier().add(BlockGeneratorAccountDescriptor(test::CopyKeyPair(signingKeyPair1), test::CopyKeyPair(vrfKeyPair1), TypeParam::value));

		// Act: lower account 1 priority and update VRF
		context.CustomPrioritizationMap[signingKeyPair1.publicKey()] = 1;
		auto result = accounts.modifier().add(BlockGeneratorAccountDescriptor(
				test::CopyKeyPair(signingKeyPair1),
				test::CopyKeyPair(vrfKeyPair2), TypeParam::value));

		// - add account 2 with in-between priority
		context.CustomPrioritizationMap[signingKeyPair2.publicKey()] = 2;
		accounts.modifier().add(BlockGeneratorAccountDescriptor(test::CopyKeyPair(signingKeyPair2), test::GenerateKeyPair(), 2));

		// Assert:
		auto view = accounts.view();
		EXPECT_EQ(UnlockedAccountsAddResult::Success_Update, result);
		EXPECT_EQ(2u, view.size());
		EXPECT_TRUE(view.contains(signingKeyPair1.publicKey()));
		EXPECT_TRUE(view.contains(signingKeyPair2.publicKey()));

		// - last vrf public key is properly associated and retains original (highest) priority
		EXPECT_FALSE(view.contains(vrfKeyPair1.publicKey()));
		EXPECT_FALSE(view.contains(vrfKeyPair2.publicKey()));

		const auto& vrfPrioritizedPublicKey = GetPrioritizedVrfPublicKey(view, signingKeyPair1.publicKey());
		EXPECT_EQ(vrfKeyPair2.publicKey(), vrfPrioritizedPublicKey.first);
		EXPECT_EQ(0u, vrfPrioritizedPublicKey.second);
	}

	TYPED_TEST(UnlockedAccountsTest, CanRemoveAccountFromContainer) {
		// Arrange:
		auto accountDescriptorWrapper1 = GenerateAccountDescriptorWrapper(TypeParam::value);
		auto accountDescriptorWrapper2 = GenerateAccountDescriptorWrapper(2);
		TestContext context(8);
		auto& accounts = context.Accounts;

		{
			auto modifier = accounts.modifier();
			modifier.add(std::move(accountDescriptorWrapper1.Descriptor));
			modifier.add(std::move(accountDescriptorWrapper2.Descriptor));
		}

		// Sanity:
		EXPECT_EQ(2u, accounts.view().size());

		// Act:
		auto removeResult = accounts.modifier().remove(accountDescriptorWrapper1.SigningPublicKey);

		// Assert:
		EXPECT_TRUE(removeResult);

		auto view = accounts.view();
		EXPECT_EQ(1u, view.size());
		EXPECT_FALSE(view.contains(accountDescriptorWrapper1.SigningPublicKey));
		EXPECT_TRUE(view.contains(accountDescriptorWrapper2.SigningPublicKey));
	}

	TYPED_TEST(UnlockedAccountsTest, RemovalOfAccountNotInContainerHasNoEffect) {
		// Arrange:
		auto accountDescriptorWrapper = GenerateAccountDescriptorWrapper(TypeParam::value);
		TestContext context(8);
		auto& accounts = context.Accounts;

		// Act:
		auto result = accounts.modifier().add(std::move(accountDescriptorWrapper.Descriptor));
		auto removeResult = accounts.modifier().remove(test::GenerateKeyPair(TypeParam::value).publicKey());

		// Assert:
		EXPECT_FALSE(removeResult);

		auto view = accounts.view();
		EXPECT_EQ(UnlockedAccountsAddResult::Success_New, result);
		EXPECT_EQ(1u, view.size());
		EXPECT_TRUE(view.contains(accountDescriptorWrapper.SigningPublicKey));
	}


	// region forEach iteration

	namespace {

		struct AccountPublicKeys {
		public:
			Key SigningPublicKey;
			Key VrfPublicKey;
			uint32_t AccountVersion;

		public:
			constexpr bool operator==(const AccountPublicKeys& rhs) const {
				return SigningPublicKey == rhs.SigningPublicKey && VrfPublicKey == rhs.VrfPublicKey;
			}
		};
		template<uint32_t TAutoAccountVersion>
		std::vector<AccountPublicKeys> AddAccounts(TestContext& context, size_t numAccounts) {
			std::vector<AccountPublicKeys> publicKeys;
			for (auto i = 0u; i < numAccounts; ++i) {
				if(i == 0)
				{
					auto accountDescriptorWrapper = GenerateAccountDescriptorWrapper(TAutoAccountVersion);
					publicKeys.push_back({ accountDescriptorWrapper.SigningPublicKey, accountDescriptorWrapper.VrfPublicKey, TAutoAccountVersion });
					AddAccount(context, std::move(accountDescriptorWrapper.Descriptor));

				}
				else
				{
					auto accountDescriptorWrapper = GenerateAccountDescriptorWrapper(2);
					publicKeys.push_back({ accountDescriptorWrapper.SigningPublicKey, accountDescriptorWrapper.VrfPublicKey, 2 });
					AddAccount(context, std::move(accountDescriptorWrapper.Descriptor));
				}
			}

			return publicKeys;
		}

		std::vector<AccountPublicKeys> ExtractAllPublicKeysOrdered(
				const UnlockedAccountsView& view,
				size_t maxPublicKeys = std::numeric_limits<size_t>::max()) {
			std::vector<AccountPublicKeys> publicKeys;
			view.forEach([maxPublicKeys, &publicKeys](const auto& descriptor) {
			  publicKeys.push_back({ descriptor.signingKeyPair().publicKey(), descriptor.vrfKeyPair().publicKey() });
			  return publicKeys.size() < maxPublicKeys;
			});

			return publicKeys;
		}
	}

	TYPED_TEST(UnlockedAccountsTest, CanIterateOverAllAccounts) {
		// Arrange:
		TestContext context(8);
		const auto& accounts = context.Accounts;
		auto expectedPublicKeys = AddAccounts<TypeParam::value>(context, 4);

		// Act:
		auto view = accounts.view();
		auto actualPublicKeys = ExtractAllPublicKeysOrdered(view);

		// Assert:
		EXPECT_EQ(4u, view.size());
		EXPECT_EQ(expectedPublicKeys, actualPublicKeys);
	}

	TYPED_TEST(UnlockedAccountsTest, CanShortCircuitIterateOverAllAccounts) {
		// Arrange:
		TestContext context(8);
		const auto& accounts = context.Accounts;

		auto expectedPublicKeys = AddAccounts<TypeParam::value>(context, 4);
		expectedPublicKeys.pop_back();
		expectedPublicKeys.pop_back();

		// Act:
		auto view = accounts.view();
		auto actualPublicKeys = ExtractAllPublicKeysOrdered(view, 2);

		// Assert:
		EXPECT_EQ(4u, view.size());
		EXPECT_EQ(2u, actualPublicKeys.size());
		EXPECT_EQ(expectedPublicKeys, actualPublicKeys);
	}

	TYPED_TEST(UnlockedAccountsTest, ForEachReturnsAccountsInStableDecreasingOrderOfPriority) {
		// Arrange:
		constexpr auto Num_Accounts = 12u;
		TestContext context(15);
		const auto& accounts = context.Accounts;

		// -     priorities: 2 1 0 2 1 0 2 1 0 2 1 0
		// - sorted indexes: 0 4 8 1 5 9 2 6 A 3 7 B
		std::vector<Key> expectedPublicKeys;
		for (auto i = 0u; i < Num_Accounts; ++i) {
			if(i == 0) //remove repeated code
			{
				auto accountDescriptorWrapper = GenerateAccountDescriptorWrapper(TypeParam::value);
				expectedPublicKeys.push_back(accountDescriptorWrapper.SigningPublicKey);
				context.CustomPrioritizationMap.emplace(accountDescriptorWrapper.SigningPublicKey, 2 - (i % 3));
				AddAccount(context, std::move(accountDescriptorWrapper.Descriptor));
			}
			else
			{
				auto accountDescriptorWrapper = GenerateAccountDescriptorWrapper(2);
				expectedPublicKeys.push_back(accountDescriptorWrapper.SigningPublicKey);
				context.CustomPrioritizationMap.emplace(accountDescriptorWrapper.SigningPublicKey, 2 - (i % 3));
				AddAccount(context, std::move(accountDescriptorWrapper.Descriptor));
			}

		}

		// Act:
		auto view = accounts.view();
		auto actualPublicKeys = ExtractAllPublicKeysOrdered(view);

		// Assert:
		EXPECT_EQ(Num_Accounts, view.size());
		EXPECT_EQ(Num_Accounts, actualPublicKeys.size());
		for (auto i = 0u; i < Num_Accounts; ++i) {
			auto expectedIndex = (i / 4) + 3 * (i % 4);
			EXPECT_EQ(expectedPublicKeys[expectedIndex], actualPublicKeys[i].SigningPublicKey)
							<< "expected index = " << expectedIndex
							<< ", actual index = " << i;
		}
	}

	TYPED_TEST(UnlockedAccountsTest, RemovedAccountsAreNotIterated) {
		// Arrange:
		TestContext context(8);
		auto& accounts = context.Accounts;

		auto expectedPublicKeys = AddAccounts<TypeParam::value>(context, 4);
		{
			auto modifier = accounts.modifier();
			modifier.remove((++expectedPublicKeys.cbegin())->SigningPublicKey);
			modifier.remove((--expectedPublicKeys.cend())->SigningPublicKey);
		}

		expectedPublicKeys.erase(++expectedPublicKeys.begin());
		expectedPublicKeys.erase(--expectedPublicKeys.cend());

		// Act:
		auto view = accounts.view();
		auto actualPublicKeys = ExtractAllPublicKeysOrdered(view);

		// Assert:
		EXPECT_EQ(2u, view.size());
		EXPECT_EQ(expectedPublicKeys, actualPublicKeys);
	}

	// endregion

	// region max accounts

	TYPED_TEST(UnlockedAccountsTest, CanAddMaxAccounts) {
		// Arrange:
		TestContext context(8);
		const auto& accounts = context.Accounts;

		// Act:
		for (auto i = 0u; i < 8; ++i)
			if(i == 0) AddRandomAccount(context, TypeParam::value);
			else AddRandomAccount(context, 2);

		// Assert:
		EXPECT_EQ(8u, accounts.view().size());
	}

	TYPED_TEST(UnlockedAccountsTest, CannotAddMoreThanMaxAccounts) {
		// Arrange:
		TestContext context(8);
		const auto& accounts = context.Accounts;

		for (auto i = 0u; i < 8; ++i)
			if(i == 0) AddRandomAccount(context, TypeParam::value);
			else AddRandomAccount(context, 2);

		// Act:
		auto result = AddRandomAccount(context, 2);

		// Assert:
		EXPECT_EQ(UnlockedAccountsAddResult::Failure_Server_Limit, result);
		EXPECT_EQ(8u, accounts.view().size());
	}

	TEST(TEST_CLASS, RemovedAccountsDoNotCountTowardsLimit) {
		// Arrange:
		auto accountDescriptorWrapper = GenerateAccountDescriptorWrapper(2);
		TestContext context(8);
		auto& accounts = context.Accounts;

		// Act:
		for (auto i = 0u; i < 4; ++i) AddRandomAccount(context, 2);
		AddAccount(context, std::move(accountDescriptorWrapper.Descriptor));
		for (auto i = 0u; i < 3; ++i) AddRandomAccount(context, 2);

		// Sanity:
		auto result = AddRandomAccount(context, 2);
		EXPECT_EQ(UnlockedAccountsAddResult::Failure_Server_Limit, result);

		// Act:
		accounts.modifier().remove(accountDescriptorWrapper.SigningPublicKey);
		result = AddRandomAccount(context, 2);

		// Assert:
		EXPECT_EQ(UnlockedAccountsAddResult::Success_New, result);
		EXPECT_EQ(8u, accounts.view().size());
	}

	namespace {
		template<uint32_t TBaseAccountVersion>
		void AssertCannotAddMoreThanMaxAccounts(const std::function<size_t (size_t)>& indexToPriorityMap) {
			// Arrange:
			TestContext context(8);
			const auto& accounts = context.Accounts;

			std::vector<AccountPublicKeys> expectedPublicKeys;
			for (auto i = 0u; i < 8; ++i) {
				if(i == 0)
				{
					auto accountDescriptorWrapper = GenerateAccountDescriptorWrapper(TBaseAccountVersion);
					expectedPublicKeys.push_back({ accountDescriptorWrapper.SigningPublicKey, accountDescriptorWrapper.VrfPublicKey });
					context.CustomPrioritizationMap.emplace(accountDescriptorWrapper.SigningPublicKey, indexToPriorityMap(i));
					AddAccount(context, std::move(accountDescriptorWrapper.Descriptor));
				}
				else
				{
					auto accountDescriptorWrapper = GenerateAccountDescriptorWrapper(2);
					expectedPublicKeys.push_back({ accountDescriptorWrapper.SigningPublicKey, accountDescriptorWrapper.VrfPublicKey });
					context.CustomPrioritizationMap.emplace(accountDescriptorWrapper.SigningPublicKey, indexToPriorityMap(i));
					AddAccount(context, std::move(accountDescriptorWrapper.Descriptor));
				}
			}

			// Act:
			auto accountDescriptorWrapper = GenerateAccountDescriptorWrapper(2);
			context.CustomPrioritizationMap.emplace(accountDescriptorWrapper.SigningPublicKey, indexToPriorityMap(8));
			auto result = AddAccount(context, std::move(accountDescriptorWrapper.Descriptor));

			auto view = accounts.view();
			auto actualPublicKeys = ExtractAllPublicKeysOrdered(view);

			// Assert:
			EXPECT_EQ(UnlockedAccountsAddResult::Failure_Server_Limit, result);
			EXPECT_EQ(8u, view.size());
			EXPECT_EQ(expectedPublicKeys, actualPublicKeys);
		}
	}

	TYPED_TEST(UnlockedAccountsTest, CannotAddMoreThanMaxAccounts_EqualPriority) {
		AssertCannotAddMoreThanMaxAccounts<TypeParam::value>([](auto) { return 7; });
	}

	TYPED_TEST(UnlockedAccountsTest, CannotAddMoreThanMaxAccounts_PriorityLessThan) {
		AssertCannotAddMoreThanMaxAccounts<TypeParam::value>([](auto i) { return 8 - i; });
	}

	TYPED_TEST(UnlockedAccountsTest, CannotAddMoreThanMaxAccounts_PriorityGreaterThan) {
		// Arrange:
		constexpr auto Num_Accounts = 8u;
		TestContext context(Num_Accounts);
		const auto& accounts = context.Accounts;

		// -     priorities: 4 3 2 1 0 4 3 2
		// - sorted indexes: 0 2 4 6 7 1 3 5
		std::vector<AccountPublicKeys> expectedPublicKeys;
		for (auto i = 0u; i < Num_Accounts; ++i) {
			auto accountDescriptorWrapper = GenerateAccountDescriptorWrapper(TypeParam::value);
			expectedPublicKeys.push_back({ accountDescriptorWrapper.SigningPublicKey, accountDescriptorWrapper.VrfPublicKey });
			context.CustomPrioritizationMap.emplace(accountDescriptorWrapper.SigningPublicKey, 4 - (i % 5));
			AddAccount(context, std::move(accountDescriptorWrapper.Descriptor));
		}

		// Act:
		auto accountDescriptorWrapper = GenerateAccountDescriptorWrapper(TypeParam::value);
		context.CustomPrioritizationMap.emplace(accountDescriptorWrapper.SigningPublicKey, 2);

		// - lowest is popped and new descriptor is second lowest
		expectedPublicKeys[4] = expectedPublicKeys[3];
		expectedPublicKeys[3] = { accountDescriptorWrapper.SigningPublicKey, accountDescriptorWrapper.VrfPublicKey };

		auto result = AddAccount(context, std::move(accountDescriptorWrapper.Descriptor));

		auto view = accounts.view();
		auto actualPublicKeys = ExtractAllPublicKeysOrdered(view);

		// Assert:
		EXPECT_EQ(UnlockedAccountsAddResult::Success_New, result);
		EXPECT_EQ(Num_Accounts, view.size());

		auto expectedIndexes = std::vector<size_t>{ 0, 5, 1, 6, 2, 7, 3, 4 };
		for (auto i = 0u; i < Num_Accounts; ++i) {
			auto expectedIndex = expectedIndexes[i];
			EXPECT_EQ(expectedPublicKeys[expectedIndex], actualPublicKeys[i])
							<< "expected index = " << expectedIndex
							<< ", actual index = " << i;
		}
	}
	// endregion

	// region removeIf

	TYPED_TEST(UnlockedAccountsTest, RemoveIfRemovesAllAccountsWhenPredicateReturnsTrueForAllAccounts) {
		// Arrange:
		TestContext context(8);
		auto& accounts = context.Accounts;
		for (auto i = 0u; i < 3; ++i)
			AddRandomAccount(context, TypeParam::value);

		// Sanity:
		EXPECT_EQ(3u, accounts.view().size());

		// Act:
		accounts.modifier().removeIf([](const auto&) { return true; });

		// Assert:
		EXPECT_EQ(0u, accounts.view().size());
	}

	TYPED_TEST(UnlockedAccountsTest, RemoveIfRemovesNoAccountsWhenPredicateReturnsFalseForAllAccounts) {
		// Arrange:
		TestContext context(8);
		auto& accounts = context.Accounts;
		for (auto i = 0u; i < 3; ++i)
			if(i == 0) AddRandomAccount(context, TypeParam::value);
			else AddRandomAccount(context, 2);

		// Sanity:
		EXPECT_EQ(3u, accounts.view().size());

		// Act:
		accounts.modifier().removeIf([](const auto&) { return false; });

		// Assert:
		EXPECT_EQ(3u, accounts.view().size());
	}

	TYPED_TEST(UnlockedAccountsTest, RemoveIfOnlyRemovesAccountsForWhichPredicateReturnsTrue) {
		// Arrange:
		TestContext context(8);
		auto& accounts = context.Accounts;
		for (auto i = 0u; i < 5; ++i)
			if(i == 0) AddRandomAccount(context, TypeParam::value);
			else AddRandomAccount(context, 2);

		// Sanity:
		EXPECT_EQ(5u, accounts.view().size());

		// Act:
		auto i = 0u;
		std::vector<Key> removedKeys;
		accounts.modifier().removeIf([&](const auto& descriptor) {
			if (0 == i++ % 2)
				return false;

		  removedKeys.push_back(descriptor.signingKeyPair().publicKey());
			return true;
		});

		// Assert:
		auto view = accounts.view();
		EXPECT_EQ(3u, view.size());
		EXPECT_EQ(2u, removedKeys.size());
		for (const auto& key : removedKeys)
			EXPECT_FALSE(view.contains(key));
	}

	// endregion

	// region synchronization

	namespace {
		auto CreateLockProvider() {
			return std::make_unique<UnlockedAccounts>(7, [](const auto&) { return 0; });
		}
	}

	DEFINE_LOCK_PROVIDER_TESTS(TEST_CLASS)

	// endregion
}}
