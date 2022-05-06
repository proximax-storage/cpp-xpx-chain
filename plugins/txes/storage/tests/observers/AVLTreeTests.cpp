/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "tests/test/StorageTestUtils.h"
#include "src/observers/Observers.h"
#include "src/utils/AVLTree.h"
#include "tests/test/plugins/ObserverTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace observers {

#define TEST_CLASS AVLTreeTests

	namespace {
		using ObserverTestContext = test::ObserverTestContextT<test::BcDriveCacheFactory>;

		constexpr Height Current_Height(20);

		auto CreateConfig() {
			test::MutableBlockchainConfiguration config;

			return config.ToConst();
		}
	} // namespace

	TEST(TEST_CLASS, AVLTreeForVerifications) {
		// Arrange:
		using ObserverTestContext = test::ObserverTestContextT<test::BcDriveCacheFactory>;
		ObserverTestContext context(NotifyMode::Commit, Current_Height, CreateConfig());
		auto& bcDriveCache = context.cache().sub<cache::BcDriveCache>();
		auto& replicatorCache = context.cache().sub<cache::ReplicatorCache>();
		auto& accountStateCache = context.cache().sub<cache::AccountStateCache>();
		auto& queueCache = context.cache().sub<cache::QueueCache>();

		utils::AVLTreeAdapter<Key> treeAdapter(
				queueCache,
				state::DriveVerificationsTree,
				[](const Key& key) {
					return key;
				},
				[&bcDriveCache](const Key& key) -> state::AVLTreeNode {
					return bcDriveCache.find(key).get().verificationNode();
				},
				[&bcDriveCache](const Key& key, const state::AVLTreeNode& node) {
					bcDriveCache.find(key).get().verificationNode() = node;
				});

		std::set<Key> keys;
		for (int i = 0; i < 100000; i++) {
			Key key = test::GenerateRandomByteArray<Key>();
			state::BcDriveEntry driveEntry(key);
			bcDriveCache.insert(driveEntry);
			keys.insert(key);
			treeAdapter.insert(key);
		}

		for (auto it = keys.begin(); it != keys.end();) {
			bool shouldRemove = (rand() % 2) > 0;
			if (shouldRemove) {
				treeAdapter.remove(*it);
				bcDriveCache.remove(*it);
				it = keys.erase(it);
			}
			else {
				it++;
			}
		}

		ASSERT_TRUE(treeAdapter.checkTreeValidity());

		for (int i = 0; i < 100000 / 720; i++) {
			Key key = test::GenerateRandomByteArray<Key>();
			auto it = keys.lower_bound(key);
			auto h = treeAdapter.lowerBound(key);
			if (h == Key()) {
				ASSERT_EQ(it, keys.end());
			}
			else {
				ASSERT_EQ(*it, bcDriveCache.find(h).get().key());
			}
		}
	}

	namespace {
		struct CacheValue {
			Hash256 key;
			state::AVLTreeNode node;
		};
	} // namespace

	TEST(TEST_CLASS, AVLTreeNumberOfLess) {
		using ObserverTestContext = test::ObserverTestContextT<test::BcDriveCacheFactory>;
		ObserverTestContext context(NotifyMode::Commit, Current_Height, CreateConfig());
		auto& queueCache = context.cache().sub<cache::QueueCache>();

		std::map<Key, CacheValue> cache;

		auto treeAdapter = utils::AVLTreeAdapter<Hash256>(
				queueCache,
				state::DriveVerificationsTree,
				[&cache](const Key& key) { return cache[key].key; },
				[&cache](const Key& key) -> state::AVLTreeNode { return cache[key].node; },
				[&cache](const Key& key, const state::AVLTreeNode& node) { cache[key].node = node; });

		for (int i = 0; i < 100000; i++) {
			auto key = test::GenerateRandomByteArray<Key>();
			auto value = test::GenerateRandomByteArray<Hash256>();
			cache[key].key = value;
			treeAdapter.insert(key);
		}

		for (int i = 0; i < 20000; i++) {
			auto h = test::GenerateRandomByteArray<Key>();
			auto it = cache.lower_bound(h);
			if (it != cache.end()) {
				treeAdapter.remove(it->second.key);
				cache.erase(it);
			}
		}

		std::vector<Hash256> sortedValues;
		for (const auto& [key, value]: cache) {
			sortedValues.push_back(value.key);
		}

		std::sort(sortedValues.begin(), sortedValues.end());

		for (int i = 0; i < 20000; i++) {
			auto v = test::GenerateRandomByteArray<Hash256>();
			auto it = std::lower_bound(sortedValues.begin(), sortedValues.end(), v);
			auto lessExpected = std::distance(sortedValues.begin(), it);
			auto lessActual = treeAdapter.numberOfLess(v);
			ASSERT_EQ(lessExpected, lessActual);
		}

		ASSERT_TRUE(treeAdapter.checkTreeValidity());
	}

	TEST(TEST_CLASS, AVLTreeExctract) {
		using ObserverTestContext = test::ObserverTestContextT<test::BcDriveCacheFactory>;
		ObserverTestContext context(NotifyMode::Commit, Current_Height, CreateConfig());
		auto& queueCache = context.cache().sub<cache::QueueCache>();

		std::map<Key, CacheValue> cache;

		auto treeAdapter = utils::AVLTreeAdapter<Hash256>(
				queueCache,
				state::DriveVerificationsTree,
				[&cache](const Key& key) { return cache[key].key; },
				[&cache](const Key& key) -> state::AVLTreeNode { return cache[key].node; },
				[&cache](const Key& key, const state::AVLTreeNode& node) { cache[key].node = node; });

		for (int i = 0; i < 10000; i++) {
			auto key = test::GenerateRandomByteArray<Key>();
			auto value = test::GenerateRandomByteArray<Hash256>();
			cache[key].key = value;
			treeAdapter.insert(key);
		}

		std::vector<std::pair<Hash256, Key>> sortedValues;
		for (const auto& [key, value]: cache) {
			sortedValues.push_back({value.key, key});
		}

		std::sort(sortedValues.begin(), sortedValues.end());

		auto rounds = sortedValues.size();
		for (uint32_t i = 0; i < rounds; i++) {
			uint32_t j = test::Random() % sortedValues.size();
			Key pointer = treeAdapter.extract(j);
			ASSERT_EQ(pointer, sortedValues[j].second);
			sortedValues.erase(sortedValues.begin() + j);
		}

		ASSERT_TRUE(treeAdapter.checkTreeValidity());
	}
}
}