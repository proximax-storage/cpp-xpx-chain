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
#include "src/cache/NamespaceCache.h"
#include "src/model/NamespaceConstants.h"
#include "src/model/NamespaceLifetimeConstraints.h"
#include "tests/test/core/mocks/MockBlockchainConfigurationHolder.h"
#include "tests/test/NamespaceCacheTestUtils.h"
#include "tests/test/NamespaceTestUtils.h"
#include "tests/test/plugins/ValidatorTestUtils.h"
#include "tests/TestHarness.h"
#include "catapult/cache_core/AccountStateCache.h"

namespace catapult { namespace validators {

#define ROOT_TEST_CLASS RootNamespaceAvailabilityValidatorTests
#define CHILD_TEST_CLASS ChildNamespaceAvailabilityValidatorTests

	DEFINE_COMMON_VALIDATOR_TESTS(RootNamespaceAvailability,)
	DEFINE_COMMON_VALIDATOR_TESTS(ChildNamespaceAvailability,)

	namespace {
		constexpr BlockDuration Max_Duration(105);
		constexpr BlockDuration Default_Duration(10);
		constexpr BlockDuration Grace_Period_Duration(25);
		const auto Nemesis_Signer = test::GenerateRandomByteArray<Key>();

		template<typename TSeedCacheFunc>
		auto CreateAndSeedCache(TSeedCacheFunc seedCache) {
			auto cache = test::NamespaceCacheFactory::Create(Grace_Period_Duration);
			{
				auto cacheDelta = cache.createDelta();
				seedCache(cacheDelta);
				cache.commit(Height());
			}

			return cache;
		}

		template<typename TSeedCacheFunc>
		void RunRootTest(
				ValidationResult expectedResult,
				const model::RootNamespaceNotification<1>& notification,
				Height height,
				TSeedCacheFunc seedCache) {
			// Arrange:
			auto cache = CreateAndSeedCache(seedCache);
			auto pluginConfig = config::NamespaceConfiguration::Uninitialized();
			pluginConfig.MaxNamespaceDuration = utils::BlockSpan::FromHours(Max_Duration.unwrap());
			pluginConfig.NamespaceGracePeriodDuration = utils::BlockSpan::FromHours(Grace_Period_Duration.unwrap());
			auto networkConfig = model::NetworkConfiguration::Uninitialized();
			networkConfig.BlockGenerationTargetTime = utils::TimeSpan::FromHours(1);
			networkConfig.Info.PublicKey = Nemesis_Signer;
			networkConfig.SetPluginConfiguration(pluginConfig);

			auto pConfigHolder = config::CreateMockConfigurationHolder(networkConfig);
			auto pValidator = CreateRootNamespaceAvailabilityValidator();

			// Act:
			auto result = test::ValidateNotification(*pValidator, notification, cache, pConfigHolder->Config(), height);

			// Assert:
			EXPECT_EQ(expectedResult, result) << "height " << height << ", duration " << notification.Duration;
		}

		template<typename TSeedCacheFunc>
		void RunChildTest(
				ValidationResult expectedResult,
				const model::ChildNamespaceNotification<1>& notification,
				Height height,
				TSeedCacheFunc seedCache) {
			// Arrange:
			auto cache = CreateAndSeedCache(seedCache);
			auto pValidator = CreateChildNamespaceAvailabilityValidator();

			// Act:
			auto result = test::ValidateNotification(*pValidator, notification, cache, config::BlockchainConfiguration::Uninitialized(), height);

			// Assert:
			EXPECT_EQ(expectedResult, result) << "height " << height;
		}

		state::RootNamespace CreateRootNamespace(NamespaceId id, const Key& owner, const state::NamespaceLifetime& lifetime) {
			return state::RootNamespace(id, owner, lifetime);
		}
	}

	namespace {
		void SeedCacheWithRoot25(cache::CatapultCacheDelta& cacheDelta) {
			// Arrange: create a cache with { 25 }
			auto& namespaceCacheDelta = cacheDelta.sub<cache::NamespaceCache>();
			auto& accountStateCacheDelta = cacheDelta.sub<cache::AccountStateCache>();
			auto owner = test::CreateRandomOwner();
			accountStateCacheDelta.addAccount(owner, Height(1));
			namespaceCacheDelta.insert(CreateRootNamespace(NamespaceId(25), owner, test::CreateLifetime(10, 20)));


			// Sanity:
			test::AssertCacheContents(namespaceCacheDelta, { 25 });
		}

		auto SeedCacheWithRoot25Signer(const Key& signer) {
			return [&signer](auto& cacheDelta) {
				// Arrange: create a cache with { 25 }
				auto& namespaceCacheDelta = cacheDelta.template sub<cache::NamespaceCache>();
				auto& accountStateCacheDelta = cacheDelta.template sub<cache::AccountStateCache>();
				accountStateCacheDelta.addAccount(signer, Height(1));
				namespaceCacheDelta.insert(state::RootNamespace(NamespaceId(25), signer, test::CreateLifetime(10, 20)));

				// Sanity:
				test::AssertCacheContents(namespaceCacheDelta, { 25 });
			};
		}
	}

	// region root - eternal namespace duration check

	TEST(ROOT_TEST_CLASS, CanAddRootNamespaceWithEternalDurationInNemesis) {
		// Act: try to create a root with an eternal duration by signer different from nemesis signer
		auto notification = model::RootNamespaceNotification<1>(test::GenerateRandomByteArray<Key>(), NamespaceId(26), Eternal_Artifact_Duration);
		RunRootTest(ValidationResult::Success, notification, Height(1), SeedCacheWithRoot25);
	}

	TEST(ROOT_TEST_CLASS, CanAddRootNamespaceWithEternalDurationAfterNemesisByNemesisSigner) {
		// Act: try to create a root with an eternal duration by nemesis signer
		auto notification = model::RootNamespaceNotification<1>(Nemesis_Signer, NamespaceId(26), Eternal_Artifact_Duration);
		RunRootTest(ValidationResult::Success, notification, Height(15), SeedCacheWithRoot25);
	}

	TEST(ROOT_TEST_CLASS, CannotAddRootNamespaceWithEternalDurationAfterNemesis) {
		// Act: try to create a root with an eternal duration by signer different from nemesis signer
		auto notification = model::RootNamespaceNotification<1>(test::GenerateRandomByteArray<Key>(), NamespaceId(26), Eternal_Artifact_Duration);
		RunRootTest(Failure_Namespace_Eternal_After_Nemesis_Block, notification, Height(15), SeedCacheWithRoot25);
	}

	TEST(ROOT_TEST_CLASS, CannotRenewNonEternalRootNamespaceWithEternalDurationAfterNemesis) {
		// Act: try to renew a root with an eternal duration by random signer
		auto signer = test::GenerateRandomByteArray<Key>();
		auto notification = model::RootNamespaceNotification<1>(signer, NamespaceId(25), Eternal_Artifact_Duration);
		RunRootTest(Failure_Namespace_Eternal_After_Nemesis_Block, notification, Height(15), SeedCacheWithRoot25Signer(signer));
	}

	// endregion

	// region root - non-eternal (new)

	TEST(ROOT_TEST_CLASS, CanAddNewRootNamespaceWithNonEternalDuration) {
		// Arrange:
		for (auto height : { Height(1), Height(15) }) {
			// Act: try to create a root with a non-eternal duration
			auto notification = model::RootNamespaceNotification<1>(Key(), NamespaceId(26), Default_Duration);
			RunRootTest(ValidationResult::Success, notification, height, SeedCacheWithRoot25);
		}
	}

	// endregion

	// region root - renew (owner grace period not expired)

	TEST(ROOT_TEST_CLASS, CanRenewRootNamespaceWithSameOwnerBeforeGracePeriodExpiration) {
		// Arrange: namespace is deactivated at height 20 and grace period is 25, so it is available starting at 45
		for (auto height : { Height(15), Height(20), Height(44) }) {
			// Act: try to renew the owner of a root that has not yet exceeded its grace period
			auto signer = test::GenerateRandomByteArray<Key>();
			auto notification = model::RootNamespaceNotification<1>(signer, NamespaceId(25), Default_Duration);
			RunRootTest(ValidationResult::Success, notification, height, SeedCacheWithRoot25Signer(signer));
		}
	}

	TEST(ROOT_TEST_CLASS, CannotChangeRootNamespaceOwnerBeforeGracePeriodExpiration) {
		// Arrange: namespace is deactivated at height 20 and grace period is 25, so it is available starting at 45
		for (auto height : { Height(15), Height(20), Height(44) }) {
			// Act: try to change the owner of a root that has not yet exceeded its grace period
			auto signer = test::GenerateRandomByteArray<Key>();
			auto notification = model::RootNamespaceNotification<1>(signer, NamespaceId(25), Default_Duration);
			RunRootTest(Failure_Namespace_Owner_Conflict, notification, height, SeedCacheWithRoot25);
		}
	}

	// endregion

	// region root - renew (owner grace period expired)

	TEST(ROOT_TEST_CLASS, CanRenewRootNamespaceWithSameOwnerAfterGracePeriodExpiration) {
		// Arrange: namespace is deactivated at height 20 and grace period is 25, so it is available starting at 45
		for (auto height : { Height(45), Height(100) }) {
			// Act: try to renew the owner of a root that has expired and exceeded its grace period
			auto signer = test::GenerateRandomByteArray<Key>();
			auto notification = model::RootNamespaceNotification<1>(signer, NamespaceId(25), Default_Duration);
			RunRootTest(ValidationResult::Success, notification, height, SeedCacheWithRoot25Signer(signer));
		}
	}

	TEST(ROOT_TEST_CLASS, CanRenewRootNamespaceWithUpgradedOwnerBeforeGracePeriodExpiration) {
		// Arrange: namespace is deactivated at height 20 and grace period is 25, so it is available starting at 45
		for (auto height : { Height(45), Height(100) }) {
			// Act: try to renew the owner of a root that has expired and exceeded its grace period
			auto signer = test::GenerateRandomByteArray<Key>();
			auto owner = test::GenerateRandomByteArray<Key>();
			auto notification = model::RootNamespaceNotification<1>(signer, NamespaceId(25), Default_Duration);
			RunRootTest(ValidationResult::Success, notification, height, [&signer, &owner](auto& cacheDelta) {
				// Arrange: create a cache with { 25 }
				auto& namespaceCacheDelta = cacheDelta.template sub<cache::NamespaceCache>();
				auto& accountStateCacheDelta = cacheDelta.template sub<cache::AccountStateCache>();
				namespaceCacheDelta.insert(state::RootNamespace(NamespaceId(25), signer, test::CreateLifetime(10, 20)));
				accountStateCacheDelta.addAccount(owner, Height(1));
				accountStateCacheDelta.addAccount(signer, Height(1));

				auto &ownerAcc = accountStateCacheDelta.find(owner).get();
				auto &signerAcc = accountStateCacheDelta.find(signer).get();
				ownerAcc.SupplementalPublicKeys.upgrade().set(signer);
				signerAcc.OldState = std::make_shared<state::AccountState>(ownerAcc);

				// Sanity:
				test::AssertCacheContents(namespaceCacheDelta, { 25 });
			});
		}
	}

	TEST(ROOT_TEST_CLASS, CanChangeRootNamespaceOwnerAfterGracePeriodExpiration) {
		// Arrange: namespace is deactivated at height 20 and grace period is 25, so it is available starting at 45
		for (auto height : { Height(45), Height(100) }) {
			// Act: try to change the owner of a root that has expired and exceeded its grace period
			auto notification = model::RootNamespaceNotification<1>(Key(), NamespaceId(25), Default_Duration);
			RunRootTest(ValidationResult::Success, notification, height, SeedCacheWithRoot25);
		}
	}

	// endregion

	// region root - renew duration

	namespace {
		void AssertChangeDuration(
				ValidationResult expectedResult,
				const Key& signer,
				Height height,
				const state::NamespaceLifetime& lifetime,
				BlockDuration duration) {
			// Act: try to extend a root that is already in the cache
			auto notification = model::RootNamespaceNotification<1>(signer, NamespaceId(25), duration);
			RunRootTest(
				expectedResult,
				notification,
				height,
				[&signer, &lifetime](auto& cacheDelta) {
					// Arrange: create a cache with { 25 }
					auto& namespaceCacheDelta = cacheDelta.template sub<cache::NamespaceCache>();
					namespaceCacheDelta.insert(state::RootNamespace(NamespaceId(25), signer, lifetime));

					// Sanity:
					test::AssertCacheContents(namespaceCacheDelta, { 25 });
				});
		}

		void AssertNemesisSignerCanChangeDuration(Height height, const state::NamespaceLifetime& lifetime, BlockDuration duration) {
			AssertChangeDuration(ValidationResult::Success, Nemesis_Signer, height, lifetime, duration);
		}

		void AssertCannotChangeDuration(const Key& signer, Height height, const state::NamespaceLifetime& lifetime, BlockDuration duration) {
			AssertChangeDuration(Failure_Namespace_Invalid_Duration, signer, height, lifetime, duration);
		}
	}

	TEST(ROOT_TEST_CLASS, CanRenewNonEternalRootNamespaceWithEternalDurationByNemesisSigner) {
		// Assert: extend a non-eternal namespace as eternal
		AssertNemesisSignerCanChangeDuration(Height(1), test::CreateLifetime(10, 20), Eternal_Artifact_Duration);
		AssertNemesisSignerCanChangeDuration(Height(100), test::CreateLifetime(10, 20), Eternal_Artifact_Duration);
	}

	TEST(ROOT_TEST_CLASS, CannotRenewNonEternalRootNamespaceWithEternalDurationInNemesis) {
		// Assert: extend a non-eternal namespace as eternal
		AssertCannotChangeDuration(test::GenerateRandomByteArray<Key>(), Height(1), test::CreateLifetime(10, 20), Eternal_Artifact_Duration);
	}

	TEST(ROOT_TEST_CLASS, CannotRenewRootNamespaceWithEternalDuration) {
		// Assert: "extend" an external namespace
		AssertCannotChangeDuration(test::GenerateRandomByteArray<Key>(), Height(1), test::CreateLifetime(10, 0xFFFF'FFFF'FFFF'FFFF), Eternal_Artifact_Duration);
		AssertCannotChangeDuration(test::GenerateRandomByteArray<Key>(), Height(100), test::CreateLifetime(10, 0xFFFF'FFFF'FFFF'FFFF), BlockDuration(2));
		AssertCannotChangeDuration(Nemesis_Signer, Height(1), test::CreateLifetime(10, 0xFFFF'FFFF'FFFF'FFFF), Eternal_Artifact_Duration);
		AssertCannotChangeDuration(Nemesis_Signer, Height(100), test::CreateLifetime(10, 0xFFFF'FFFF'FFFF'FFFF), BlockDuration(2));
	}

	// endregion

	namespace {
		auto SeedCacheWithRoot25TreeSigner(const Key& signer) {
			return [&signer](auto& cacheDelta) {

				auto& namespaceCacheDelta = cacheDelta.template sub<cache::NamespaceCache>();
				// Arrange: create a cache with { 25 }, { 25, 36 } and { 25, 36, 49 }
				auto& accountStateCacheDelta = cacheDelta.template sub<cache::AccountStateCache>();
				accountStateCacheDelta.addAccount(signer, Height(1));
				namespaceCacheDelta.insert(state::RootNamespace(NamespaceId(25), signer, test::CreateLifetime(10, 20)));
				namespaceCacheDelta.insert(state::Namespace(test::CreatePath({ 25, 36 })));
				namespaceCacheDelta.insert(state::Namespace(test::CreatePath({ 25, 36, 49 })));

				// Sanity:
				test::AssertCacheContents(namespaceCacheDelta, { 25, 36, 49 });
			};
		}
	}

	// region child - existence

	TEST(CHILD_TEST_CLASS, CanAddChildNamespaceThatDoesNotExistToRootParent) {
		// Arrange:
		for (auto height : { Height(10), Height(15), Height(19) }) {
			// Act: try to create a child that is not in the cache (parent is root 25)
			auto signer = test::GenerateRandomByteArray<Key>();
			auto notification = model::ChildNamespaceNotification<1>(signer, NamespaceId(38), NamespaceId(25));
			RunChildTest(ValidationResult::Success, notification, height, SeedCacheWithRoot25TreeSigner(signer));
		}
	}

	TEST(CHILD_TEST_CLASS, CanAddChildNamespaceThatDoesNotExistToNonRootParent) {
		// Arrange:
		for (auto height : { Height(10), Height(15), Height(19) }) {
			// Act: try to create a child that is not in the cache (parent is non-root 36)
			auto signer = test::GenerateRandomByteArray<Key>();
			auto notification = model::ChildNamespaceNotification<1>(signer, NamespaceId(50), NamespaceId(36));
			RunChildTest(ValidationResult::Success, notification, height, SeedCacheWithRoot25TreeSigner(signer));
		}
	}

	TEST(CHILD_TEST_CLASS, CannotAddChildNamespaceThatAlreadyExists) {
		// Act: try to create a child that is already in the cache
		for (auto height : { Height(15), Height(19) }) {
			auto signer = test::GenerateRandomByteArray<Key>();
			auto notification = model::ChildNamespaceNotification<1>(signer, NamespaceId(36), NamespaceId(25));
			RunChildTest(Failure_Namespace_Already_Exists, notification, height, SeedCacheWithRoot25TreeSigner(signer));
		};
	}

	// endregion

	// region child - parent

	TEST(CHILD_TEST_CLASS, CannotAddChildNamespaceThatHasUnknownParent) {
		// Act: try to create a child with an unknown (root) parent
		auto signer = test::GenerateRandomByteArray<Key>();
		auto notification = model::ChildNamespaceNotification<1>(signer, NamespaceId(38), NamespaceId(26));
		RunChildTest(Failure_Namespace_Parent_Unknown, notification, Height(15), SeedCacheWithRoot25TreeSigner(signer));
	}

	TEST(CHILD_TEST_CLASS, CannotAddChildNamespaceToParentWithMaxNamespaceDepth) {
		// Act: try to create a child that is too deep
		auto signer = test::GenerateRandomByteArray<Key>();
		auto notification = model::ChildNamespaceNotification<1>(signer, NamespaceId(64), NamespaceId(49));
		RunChildTest(Failure_Namespace_Too_Deep, notification, Height(15), SeedCacheWithRoot25TreeSigner(signer));
	}

	// endregion

	// region child - root

	TEST(CHILD_TEST_CLASS, CannotAddChildNamespaceToExpiredRoot) {
		// Arrange:
		for (auto height : { Height(20), Height(25), Height(100) }) {
			// Act: try to create a child attached to a root that has expired (root namespace expires at height 20)
			auto signer = test::GenerateRandomByteArray<Key>();
			auto notification = model::ChildNamespaceNotification<1>(signer, NamespaceId(50), NamespaceId(36));
			RunChildTest(Failure_Namespace_Expired, notification, height, SeedCacheWithRoot25TreeSigner(signer));
		}
	}

	TEST(CHILD_TEST_CLASS, CannotAddChildNamespaceToRootWithConflictingOwner) {
		// Arrange:
		for (auto height : { Height(10), Height(15), Height(19) }) {
			// Act: try to create a child attached to a root with a different owner
			auto signer = test::GenerateRandomByteArray<Key>();
			auto notification = model::ChildNamespaceNotification<1>(signer, NamespaceId(50), NamespaceId(36));
			auto rootSigner = test::CreateRandomOwner();
			RunChildTest(Failure_Namespace_Owner_Conflict, notification, height, SeedCacheWithRoot25TreeSigner(rootSigner));
		}
	}

	// endregion
}}
