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

#include "src/validators/Validators.h"
#include "tests/test/AliasTestUtils.h"
#include "tests/test/NamespaceCacheTestUtils.h"
#include "tests/test/NamespaceTestUtils.h"
#include "tests/test/plugins/ValidatorTestUtils.h"

using namespace catapult::model;

namespace catapult { namespace validators {

#define TEST_CLASS AliasAvailabilityValidatorTests

	DEFINE_COMMON_VALIDATOR_TESTS(AliasAvailability,)

	// region basic tests

	namespace {
		constexpr auto Default_Namespace_Id = NamespaceId(123);

		template<typename TSeedCacheFunc>
		auto CreateAndSeedCache(TSeedCacheFunc seedCache) {
			auto cache = test::NamespaceCacheFactory::Create();
			auto cacheDelta = cache.createDelta();
			auto& namespaceCacheDelta = cacheDelta.sub<cache::NamespaceCache>();
			seedCache(namespaceCacheDelta);
			cache.commit(Height());
			return cache;
		}

		template<typename TSeedCacheFunc>
		void RunAvailabilityTest(ValidationResult expectedResult, const AliasOwnerNotification<1>& notification, TSeedCacheFunc seedCache) {
			// Arrange:
			auto cache = CreateAndSeedCache(seedCache);

			auto pValidator = CreateAliasAvailabilityValidator();

			// Act:
			auto result = test::ValidateNotification(*pValidator, notification, cache, config::BlockchainConfiguration::Uninitialized(), Height(200));

			// Assert:
			EXPECT_EQ(expectedResult, result);
		}
	}

	TEST(TEST_CLASS, FailureWhenNamespaceIsUnknown) {
		// Arrange:
		auto owner = test::GenerateRandomByteArray<Key>();
		AliasOwnerNotification<1> notification(owner, Default_Namespace_Id, AliasAction::Link);

		// Assert:
		RunAvailabilityTest(Failure_Namespace_Alias_Namespace_Unknown, notification, [](const auto&) {});
	}

	TEST(TEST_CLASS, FailureWhenOwnerDoesNotMatch) {
		// Arrange:
		auto owner = test::GenerateRandomByteArray<Key>();
		AliasOwnerNotification<1> notification(owner, Default_Namespace_Id, AliasAction::Link);

		// Assert:
		RunAvailabilityTest(Failure_Namespace_Alias_Owner_Conflict, notification, [&owner](auto& cache) {
			auto namespaceOwner = owner;
			namespaceOwner[0] ^= 0xFF;
			cache.insert(state::RootNamespace(Default_Namespace_Id, namespaceOwner, test::CreateLifetime(100, 300)));
		});
	}

	TEST(TEST_CLASS, FailureWhenNamespaceExpired) {
		// Arrange:
		auto owner = test::GenerateRandomByteArray<Key>();
		AliasOwnerNotification<1> notification(owner, Default_Namespace_Id, AliasAction::Link);

		// Assert: notification is at height 200, so limit lifetime to 150
		RunAvailabilityTest(Failure_Namespace_Expired, notification, [&owner](auto& cache) {
			cache.insert(state::RootNamespace(Default_Namespace_Id, owner, test::CreateLifetime(100, 150)));
		});
	}

	// endregion

	// region alias existence tests

	namespace {
		enum class LinkState {
			Unset,
			Set
		};

		struct RootTraits {
			static constexpr auto Notification_Namespace_Id = Default_Namespace_Id;

			static void Prepare(const cache::NamespaceCacheDelta&)
			{}
		};

		constexpr auto Child_Namespace_Id = NamespaceId(234);

		struct ChildTraits {
			static constexpr auto Notification_Namespace_Id = Child_Namespace_Id;

			static void Prepare(cache::NamespaceCacheDelta& cache) {
				// Arrange: always set some link in root namespace
				// (this is not required for tests, this is additional thing, to make sure validator is working correctly)
				test::SetRandomAlias<MosaicId>(cache, Default_Namespace_Id);
			}
		};

		template<typename TTraits>
		void RunTest(ValidationResult expectedResult, AliasAction aliasAction, LinkState linkState) {
			// Arrange:
			auto owner = test::GenerateRandomByteArray<Key>();
			AliasOwnerNotification<1> notification(owner, TTraits::Notification_Namespace_Id, aliasAction);

			// Assert:
			RunAvailabilityTest(expectedResult, notification, [&owner, linkState](auto& cache) {
				cache.insert(state::RootNamespace(Default_Namespace_Id, owner, test::CreateLifetime(100, 300)));
				cache.insert(state::Namespace(test::CreatePath({ 123, 234 })));
				TTraits::Prepare(cache);

				// type of alias does not matter
				if (LinkState::Set == linkState)
					test::SetRandomAlias<MosaicId>(cache, TTraits::Notification_Namespace_Id);
			});
		}
	}

#define MAKE_ALIAS_AVAILABILITY_TEST(TEST_NAME) \
	template<typename TTraits> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)(); \
	TEST(TEST_CLASS, TEST_NAME##_Root) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<RootTraits>(); } \
	TEST(TEST_CLASS, TEST_NAME##_Child) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<ChildTraits>(); } \
	template<typename TTraits> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)()

	MAKE_ALIAS_AVAILABILITY_TEST(SuccessWhenActionLinkAndLinkDoesNotExist) {
		RunTest<TTraits>(ValidationResult::Success, AliasAction::Link, LinkState::Unset);
	}

	MAKE_ALIAS_AVAILABILITY_TEST(SuccessWhenActionUnlinkAndLinkExists) {
		RunTest<TTraits>(ValidationResult::Success, AliasAction::Unlink, LinkState::Set);
	}

	MAKE_ALIAS_AVAILABILITY_TEST(FailureWhenActionLinkAndLinkAlreadyExists) {
		RunTest<TTraits>(Failure_Namespace_Alias_Already_Exists, AliasAction::Link, LinkState::Set);
	}

	MAKE_ALIAS_AVAILABILITY_TEST(FailureWhenActionUnlinkAndLinkDoesNotExist) {
		RunTest<TTraits>(Failure_Namespace_Alias_Does_Not_Exist, AliasAction::Unlink, LinkState::Unset);
	}

	// endregion
}}
