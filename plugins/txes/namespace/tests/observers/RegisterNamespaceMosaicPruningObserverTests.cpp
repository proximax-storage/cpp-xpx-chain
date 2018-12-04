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

#include "src/observers/Observers.h"
#include "src/cache/MosaicCache.h"
#include "src/cache/NamespaceCache.h"
#include "tests/test/MosaicCacheTestUtils.h"
#include "tests/test/NamespaceCacheTestUtils.h"
#include "tests/test/NamespaceTestUtils.h"
#include "tests/test/TransactionTestUtils.h"
#include "tests/test/plugins/ObserverTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace observers {

#define TEST_CLASS RegisterNamespaceMosaicPruningObserverTests

	DEFINE_COMMON_OBSERVER_TESTS(RegisterNamespaceMosaicPruning, BlockDuration(5))

	namespace {
		constexpr auto Max_Rollback_Blocks = 15u;

		class ObserverTestContext : public test::ObserverTestContextT<test::MosaicCacheFactory> {
		public:
			ObserverTestContext(observers::NotifyMode mode, Height height)
					: test::ObserverTestContextT<test::MosaicCacheFactory>(mode, height, CreateConfiguration())
			{}

		private:
			static model::BlockChainConfiguration CreateConfiguration() {
				auto config = model::BlockChainConfiguration::Uninitialized();
				config.Plugins.emplace("namespace::ex", utils::ConfigurationBag({
					{
						"",
						{
							{ "gracePeriodDuration", "31" }
						}
					}
				}));
				return config;
			}
		};

		auto SeedCacheWithRoot25TreeSigner(const Key& signer) {
			return [&signer](auto& namespaceCacheDelta) {
				// Arrange: create a cache with { 25 } and { 25, 36 }
				namespaceCacheDelta.insert(state::RootNamespace(NamespaceId(25), signer, test::CreateLifetime(10, 123)));
				namespaceCacheDelta.insert(state::Namespace(test::CreatePath({ 25, 36 })));

				// Sanity:
				test::AssertCacheContents(namespaceCacheDelta, { 25, 36 });
			};
		}

		void SeedCache(cache::CatapultCacheDelta& cache, const Key& owner) {
			auto& namespaceCacheDelta = cache.sub<cache::NamespaceCache>();
			SeedCacheWithRoot25TreeSigner(owner)(namespaceCacheDelta);

			test::AddEternalMosaic(cache, NamespaceId(25), MosaicId(26), Height(500), owner);
			test::AddEternalMosaic(cache, NamespaceId(36), MosaicId(37), Height(500), owner);
		}

		model::RootNamespaceNotification CreateNotification(const Key& key, NamespaceId id) {
			return model::RootNamespaceNotification(key, id, BlockDuration());
		}

		template<typename TSeedCacheFunc, typename TCheckCacheFunc>
		void RunTest(
				const model::RootNamespaceNotification& notification,
				const Key& owner,
				ObserverTestContext&& context,
				TSeedCacheFunc seedCache,
				TCheckCacheFunc checkCache) {
			// Arrange:
			auto pObserver = CreateRegisterNamespaceMosaicPruningObserver(BlockDuration(Max_Rollback_Blocks));

			// - seed the cache
			seedCache(context.cache(), owner);

			// Act:
			test::ObserveNotification(*pObserver, notification, context);

			// Assert: check the cache
			checkCache(context.cache().sub<cache::MosaicCache>());
		}
	}

	// region no operation

	TEST(TEST_CLASS, ObserverDoesNothingInModeRollback) {
		// Arrange:
		auto notification = CreateNotification(Key(), NamespaceId(25));

		// Act:
		RunTest(
				notification,
				test::GenerateRandomData<Key_Size>(),
				ObserverTestContext(NotifyMode::Rollback, Height(888)),
				SeedCache,
				[](auto& mosaicCacheDelta) {
					// Assert: rollback has no effect
					test::AssertCacheContents(mosaicCacheDelta, { 26, 37 });
				});
	}

	TEST(TEST_CLASS, ObserverDoesNothingWhenRegisteringUnknownRoot) {
		// Arrange:
		auto notification = CreateNotification(Key(), NamespaceId(246));

		// Act:
		RunTest(
				notification,
				test::GenerateRandomData<Key_Size>(),
				ObserverTestContext(NotifyMode::Commit, Height(888)),
				SeedCache,
				[](auto& mosaicCacheDelta) {
					// Assert: the mosaics were not removed because the owning namespace was not affected
					test::AssertCacheContents(mosaicCacheDelta, { 26, 37 });
				});
	}

	TEST(TEST_CLASS, ObserverDoesNothingWhenHeightIsLessThanMaxRollbackBlocks) {
		// Arrange:
		auto owner = test::GenerateRandomData<Key_Size>();
		auto notification = CreateNotification(owner, NamespaceId(25));

		// Act:
		RunTest(
				notification,
				owner,
				ObserverTestContext(NotifyMode::Commit, Height(Max_Rollback_Blocks - 1)),
				SeedCache,
				[](auto& mosaicCacheDelta) {
					// Assert: the mosaics were not removed
					test::AssertCacheContents(mosaicCacheDelta, { 26, 37 });
				});
	}

	TEST(TEST_CLASS, ObserverDoesNothingWhenRegisteringRootWithinGracePeriodOrMaxRollbackBlocks) {
		// Arrange:
		auto owner = test::GenerateRandomData<Key_Size>();
		auto notification = CreateNotification(owner, NamespaceId(25));

		// Act: root expires at height 123, grace period duration is 31
		for (auto height : { 87u, 122u, 123u, 135u, 153u, 153u + Max_Rollback_Blocks })
			RunTest(
					notification,
					owner,
					ObserverTestContext(NotifyMode::Commit, Height(height)),
					SeedCache,
					[](auto& mosaicCacheDelta) {
						// Assert: the mosaics were not removed because the owning namespace had not expired
						test::AssertCacheContents(mosaicCacheDelta, { 26, 37 });
					});
	}

	// endregion

	// region pruning

	TEST(TEST_CLASS, ObserverPrunesMosaicsWhenRegisteringRootOutsideGracePeriodAndMaxRollbackBlocks) {
		// Arrange:
		auto owner = test::GenerateRandomData<Key_Size>();
		auto notification = CreateNotification(owner, NamespaceId(25));

		// Act: root expires at height 123, grace period duration is 31
		for (auto height : { 154u + Max_Rollback_Blocks, 200u }) {
			RunTest(
					notification,
					owner,
					ObserverTestContext(NotifyMode::Commit, Height(height)),
					SeedCache,
					[](auto& mosaicCacheDelta) {
						// Assert: the mosaics owned by the expired namespace were removed
						test::AssertCacheContents(mosaicCacheDelta, {});
					});
		}
	}

	TEST(TEST_CLASS, ObserverPrunesMosaicsWhenRegisteringRootWithDifferentOwner) {
		// Arrange:
		auto owner = test::GenerateRandomData<Key_Size>();
		auto notification = CreateNotification(owner, NamespaceId(25));

		// Act: root expires at height 123, grace period duration is 31
		RunTest(
				notification,
				test::GenerateRandomData<Key_Size>(),
				ObserverTestContext(NotifyMode::Commit, Height(135)),
				SeedCache,
				[](auto& mosaicCacheDelta) {
					// Assert: the mosaics owned by the expired namespace were removed
					test::AssertCacheContents(mosaicCacheDelta, {});
				});
	}

	TEST(TEST_CLASS, ObserverDoesNotPruneMosaicsOwnedByDifferentNamespace) {
		// Arrange: notice that notification signer is not important in this test
		auto signer = test::GenerateRandomData<Key_Size>();
		auto notification = CreateNotification(signer, NamespaceId(25));

		// Act: root expires at height 123, grace period duration is 31
		RunTest(
				notification,
				test::GenerateRandomData<Key_Size>(),
				ObserverTestContext(NotifyMode::Commit, Height(200)),
				[](auto& cache, const auto& owner) {
					SeedCache(cache, owner);
					auto otherOwner = test::GenerateRandomData<Key_Size>();
					test::AddEternalMosaic(cache, NamespaceId(357), MosaicId(321), Height(500), otherOwner);
				},
				[](auto& mosaicCacheDelta) {
					// Assert: prune should not remove mosaic 321 because it has a different owner than namespace 25
					test::AssertCacheContents(mosaicCacheDelta, { 321 });
				});
	}

	// endregion
}}
