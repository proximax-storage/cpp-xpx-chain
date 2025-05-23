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

#include "src/cache/NamespaceCacheStorage.h"
#include "tests/test/core/mocks/MockBlockchainConfigurationHolder.h"
#include "tests/test/NamespaceCacheTestUtils.h"
#include "tests/test/RootNamespaceHistoryLoadTests.h"
#include "tests/TestHarness.h"

namespace catapult { namespace cache {

#define TEST_CLASS NamespaceCacheStorageTests

	// region LoadInto

	namespace {
		auto CreateConfigHolder() {
			auto pluginConfig = config::NamespaceConfiguration::Uninitialized();
			pluginConfig.MaxNamespaceDuration = utils::BlockSpan::FromHours(100);
			auto networkConfig = model::NetworkConfiguration::Uninitialized();
			networkConfig.BlockGenerationTargetTime = utils::TimeSpan::FromHours(1);
			networkConfig.SetPluginConfiguration(pluginConfig);
			return config::CreateMockConfigurationHolder(networkConfig);
		}

		auto DefaultCacheOptions() {
			return NamespaceCacheTypes::Options{ CreateConfigHolder() };
		}

		void LoadInto(io::InputStream& inputStream, NamespaceCacheDelta& delta) {
			return NamespaceCacheStorage::LoadInto(NamespaceCacheStorage::Load(inputStream), delta);
		}

		state::NamespaceAlias GetAlias(const NamespaceCacheView& view, NamespaceId id) {
			return view.find(id).get().root().alias(id);
		}

		state::NamespaceAlias GetAlias(const NamespaceCacheDelta& delta, NamespaceId id) {
			return delta.find(id).get().root().alias(id);
		}

		struct SharedTraits {
			class CacheType : public NamespaceCache {
			public:
				CacheType() : NamespaceCache(CacheConfiguration(), NamespaceCacheTypes::Options{ CreateConfigHolder() })
				{}
			};

			using NamespaceHistoryHeader = test::NamespaceHistoryHeader;

			static NamespaceHistoryHeader CreateHistoryHeader(VersionType version, NamespaceId namespaceId, uint64_t depth) {
				return { version, depth, namespaceId };
			}
		};

		struct LoadTraits : public SharedTraits {
			template<typename TException>
			static void AssertCannotLoad(io::InputStream& inputStream) {
				// Assert:
				auto options = DefaultCacheOptions();
				CacheType cache;
				auto delta = cache.createDelta(Height{0});
				EXPECT_THROW(LoadInto(inputStream, *delta), TException);
			}

			static void AssertCanLoadEmptyHistory(io::InputStream& inputStream) {
				// Act:
				CacheType cache;
				{
					auto delta = cache.createDelta(Height{0});
					LoadInto(inputStream, *delta);
					cache.commit();
				}

				// Assert:
				auto view = cache.createView(Height{0});
				test::AssertCacheSizes(*view, 0, 0, 0);
			}

			static void AssertCanLoadHistoryWithDepthOneWithoutChildren(
					io::InputStream& inputStream,
					const Key& owner,
					const state::NamespaceAlias& alias) {
				// Act:
				CacheType cache;
				{
					auto delta = cache.createDelta(Height{0});
					LoadInto(inputStream, *delta);
					cache.commit();
				}

				// Assert:
				auto view = cache.createView(Height{0});
				test::AssertCacheSizes(*view, 1, 1, 1);

				ASSERT_TRUE(view->contains(NamespaceId(123)));
				test::AssertRootNamespace(view->find(NamespaceId(123)).get().root(), owner, Height(222), Height(333), 0, alias);
			}

			static void AssertCannotLoadHistoryWithDepthOneOutOfOrderChildren(io::InputStream& inputStream) {
				// Arrange:
				CacheType cache;
				auto delta = cache.createDelta(Height{0});

				// Act + Assert:
				EXPECT_THROW(LoadInto(inputStream, *delta), catapult_invalid_argument);
			}

			static void AssertCanLoadHistoryWithDepthOneWithChildren(
					io::InputStream& inputStream,
					const Key& owner,
					const std::vector<state::NamespaceAlias>& aliases) {
				// Act:
				CacheType cache;
				{
					auto delta = cache.createDelta(Height{0});
					LoadInto(inputStream, *delta);
					cache.commit();
				}

				// Assert:
				auto view = cache.createView(Height{0});
				test::AssertCacheSizes(*view, 1, 4, 4);

				ASSERT_TRUE(view->contains(NamespaceId(123)));
				test::AssertRootNamespace(view->find(NamespaceId(123)).get().root(), owner, Height(222), Height(333), 3);
				EXPECT_EQ(NamespaceId(123), view->find(NamespaceId(124)).get().ns().parentId());
				EXPECT_EQ(NamespaceId(124), view->find(NamespaceId(125)).get().ns().parentId());
				EXPECT_EQ(NamespaceId(123), view->find(NamespaceId(126)).get().ns().parentId());

				// - check aliases
				ASSERT_EQ(3u, aliases.size());
				test::AssertEqualAlias(aliases[0], GetAlias(*view, NamespaceId(124)), "124");
				test::AssertEqualAlias(aliases[1], GetAlias(*view, NamespaceId(125)), "125");
				test::AssertEqualAlias(aliases[2], GetAlias(*view, NamespaceId(126)), "126");
			}

			static void AssertCanLoadHistoryWithDepthGreaterThanOneSameOwner(
					io::InputStream& inputStream,
					const Key& owner,
					const std::vector<state::NamespaceAlias>& aliases) {
				// Act:
				CacheType cache;
				{
					auto delta = cache.createDelta(Height{0});
					LoadInto(inputStream, *delta);
					cache.commit();
				}

				// Assert:
				{
					auto view = cache.createView(Height{0});
					test::AssertCacheSizes(*view, 1, 5, 15);

					ASSERT_TRUE(view->contains(NamespaceId(123)));
					test::AssertRootNamespace(view->find(NamespaceId(123)).get().root(), owner, Height(444), Height(555), 4);
					EXPECT_EQ(NamespaceId(123), view->find(NamespaceId(124)).get().ns().parentId());
					EXPECT_EQ(NamespaceId(124), view->find(NamespaceId(125)).get().ns().parentId());
					EXPECT_EQ(NamespaceId(123), view->find(NamespaceId(126)).get().ns().parentId());
					EXPECT_EQ(NamespaceId(126), view->find(NamespaceId(129)).get().ns().parentId());

					// - check aliases
					ASSERT_EQ(4u, aliases.size());
					test::AssertEqualAlias(aliases[0], GetAlias(*view, NamespaceId(124)), "124");
					test::AssertEqualAlias(aliases[1], GetAlias(*view, NamespaceId(125)), "125");
					test::AssertEqualAlias(aliases[2], GetAlias(*view, NamespaceId(126)), "126");
					test::AssertEqualAlias(aliases[3], GetAlias(*view, NamespaceId(129)), "129");
				}

				// - check history (one back)
				auto delta = cache.createDelta(Height{0});
				delta->remove(NamespaceId(123));
				test::AssertCacheSizes(*delta, 1, 5, 10);
				test::AssertRootNamespace(delta->find(NamespaceId(123)).get().root(), owner, Height(222), Height(333), 4);

				// - check history (two back)
				delta->remove(NamespaceId(123));
				test::AssertCacheSizes(*delta, 1, 5, 5);
				test::AssertRootNamespace(delta->find(NamespaceId(123)).get().root(), owner, Height(11), Height(111), 4);
			}

			static void AssertCanLoadHistoryWithDepthGreaterThanOneDifferentOwner(
					io::InputStream& inputStream,
					const Key& owner1,
					const Key& owner2,
					const Key& owner3,
					const std::vector<state::NamespaceAlias>& aliases) {
				// Act:
				CacheType cache;
				{
					auto delta = cache.createDelta(Height{0});
					LoadInto(inputStream, *delta);
					cache.commit();
				}

				// Assert:
				{
					auto view = cache.createView(Height{0});
					test::AssertCacheSizes(*view, 1, 2, 7);

					ASSERT_TRUE(view->contains(NamespaceId(123)));
					test::AssertRootNamespace(view->find(NamespaceId(123)).get().root(), owner3, Height(444), Height(555), 1);
					EXPECT_EQ(NamespaceId(123), view->find(NamespaceId(126)).get().ns().parentId());

					// - check aliases
					ASSERT_EQ(4u, aliases.size());
					test::AssertEqualAlias(aliases[3], GetAlias(*view, NamespaceId(126)), "126 :: 0");
				}

				// - check history (one back)
				auto delta = cache.createDelta(Height{0});
				delta->remove(NamespaceId(123));
				test::AssertCacheSizes(*delta, 1, 4, 5);
				test::AssertRootNamespace(delta->find(NamespaceId(123)).get().root(), owner2, Height(222), Height(333), 3);
				EXPECT_EQ(NamespaceId(123), delta->find(NamespaceId(124)).get().ns().parentId());
				EXPECT_EQ(NamespaceId(124), delta->find(NamespaceId(125)).get().ns().parentId());
				EXPECT_EQ(NamespaceId(123), delta->find(NamespaceId(126)).get().ns().parentId());

				// - check aliases
				test::AssertEqualAlias(aliases[0], GetAlias(*delta, NamespaceId(124)), "124");
				test::AssertEqualAlias(aliases[1], GetAlias(*delta, NamespaceId(125)), "125");
				test::AssertEqualAlias(aliases[2], GetAlias(*delta, NamespaceId(126)), "126 :: 1");

				// - check history (two back)
				delta->remove(NamespaceId(123));
				test::AssertCacheSizes(*delta, 1, 1, 1);
				test::AssertRootNamespace(delta->find(NamespaceId(123)).get().root(), owner1, Height(11), Height(111), 0);
			}
		};
	}

	DEFINE_ROOT_NAMESPACE_HISTORY_LOAD_TESTS(LoadTraits,,1)

	// endregion

	// region Purge

	namespace {
		class PurgeTraits : public SharedTraits {
		public:
			static void AssertCanLoadHistoryWithDepthOneWithoutChildren(
					io::InputStream& inputStream,
					const Key&,
					const state::NamespaceAlias&) {
				// Arrange:
				CacheType cache;
				auto history = SeedCache(cache, inputStream);

				// Sanity:
				test::AssertCacheSizes(*cache.createView(Height{0}), 2, 2, 2);

				// Act + Assert:
				RunPurgeTest(cache, history);
			}

			static void AssertCanLoadHistoryWithDepthOneWithChildren(
					io::InputStream& inputStream,
					const Key&,
					const std::vector<state::NamespaceAlias>&) {
				// Arrange:
				CacheType cache;
				auto history = SeedCache(cache, inputStream);

				// Sanity:
				test::AssertCacheSizes(*cache.createView(Height{0}), 2, 5, 5);

				// Act + Assert:
				RunPurgeTest(cache, history);
			}

			static void AssertCanLoadHistoryWithDepthGreaterThanOneSameOwner(
					io::InputStream& inputStream,
					const Key&,
					const std::vector<state::NamespaceAlias>&) {
				// Arrange:
				CacheType cache;
				auto history = SeedCache(cache, inputStream);

				// Sanity:
				test::AssertCacheSizes(*cache.createView(Height{0}), 2, 6, 16);

				// Act + Assert:
				RunPurgeTest(cache, history);
			}

			static void AssertCanLoadHistoryWithDepthGreaterThanOneDifferentOwner(
					io::InputStream& inputStream,
					const Key&,
					const Key&,
					const Key&,
					const std::vector<state::NamespaceAlias>&) {
				// Arrange:
				CacheType cache;
				auto history = SeedCache(cache, inputStream);

				// Sanity:
				test::AssertCacheSizes(*cache.createView(Height{0}), 2, 3, 8);

				// Act + Assert:
				RunPurgeTest(cache, history);
			}

		private:
			static state::RootNamespaceHistory SeedCache(CacheType& cache, io::InputStream& inputStream) {
				// Arrange: add two histories one of which will be purged
				auto history = NamespaceCacheStorage::Load(inputStream);
				{
					auto delta = cache.createDelta(Height{0});
					auto owner = test::GenerateRandomByteArray<Key>();
					delta->insert(state::RootNamespace(NamespaceId(987), owner, test::CreateLifetime(100, 200)));
					NamespaceCacheStorage::LoadInto(history, *delta);
					cache.commit();
				}

				return history;
			}

			static void RunPurgeTest(CacheType& cache, const state::RootNamespaceHistory& history) {
				// Sanity:
				EXPECT_TRUE(cache.createView(Height{0})->contains(history.id()));
				EXPECT_TRUE(cache.createView(Height{0})->contains(NamespaceId(987)));

				// Act:
				{
					auto delta = cache.createDelta(Height{0});
					NamespaceCacheStorage::Purge(history, *delta);
					cache.commit();
				}

				// Assert:
				auto view = cache.createView(Height{0});
				test::AssertCacheSizes(*view, 1, 1, 1);
				EXPECT_FALSE(view->contains(history.id()));
				EXPECT_TRUE(view->contains(NamespaceId(987)));
			}
		};
	}

	TEST(TEST_CLASS, CanPurgeNonexistentHistoryFromCache) {
		// Arrange: add one value that will not be purged
		PurgeTraits::CacheType cache;
		{
			auto delta = cache.createDelta(Height{0});
			delta->insert(state::RootNamespace(NamespaceId(987), test::GenerateRandomByteArray<Key>(), test::CreateLifetime(100, 200)));
			cache.commit();
		}

		// Sanity:
		test::AssertCacheSizes(*cache.createView(Height{0}), 1, 1, 1);
		EXPECT_FALSE(cache.createView(Height{0})->contains(NamespaceId(123)));
		EXPECT_TRUE(cache.createView(Height{0})->contains(NamespaceId(987)));

		// Act:
		{
			auto delta = cache.createDelta(Height{0});
			NamespaceCacheStorage::Purge(state::RootNamespaceHistory(NamespaceId(123)), *delta);
			cache.commit();
		}

		// Assert:
		auto view = cache.createView(Height{0});
		test::AssertCacheSizes(*view, 1, 1, 1);
		EXPECT_FALSE(view->contains(NamespaceId(123)));
		EXPECT_TRUE(view->contains(NamespaceId(987)));
	}

	TEST(TEST_CLASS, CanPurgeHistoryWithDepthOneWithoutChildren) {
		// Assert:
		test::RootNamespaceHistoryLoadTests<PurgeTraits>::AssertCanLoadHistoryWithDepthOneWithoutChildren(VersionType{1});
	}

	TEST(TEST_CLASS, CanPurgeHistoryWithDepthOneWithChildren) {
		// Assert:
		test::RootNamespaceHistoryLoadTests<PurgeTraits>::AssertCanLoadHistoryWithDepthOneWithChildren(VersionType{1});
	}

	TEST(TEST_CLASS, CanPurgeHistoryWithDepthGreaterThanOneSameOwner) {
		// Assert:
		test::RootNamespaceHistoryLoadTests<PurgeTraits>::AssertCanLoadHistoryWithDepthGreaterThanOneSameOwner(VersionType{1});
	}

	TEST(TEST_CLASS, CanPurgeHistoryWithDepthGreaterThanOneDifferentOwner) {
		// Assert:
		test::RootNamespaceHistoryLoadTests<PurgeTraits>::AssertCanLoadHistoryWithDepthGreaterThanOneDifferentOwner(VersionType{1});
	}

	// endregion
}}
