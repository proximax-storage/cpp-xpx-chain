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

#include "catapult/cache_tx/MemoryCacheProxy.h"
#include "catapult/cache_tx/MemoryUtCache.h"
#include "tests/test/core/TransactionInfoTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace cache {

#define TEST_CLASS MemoryCacheProxyTests

	// tests use MemoryUtCache types
	using MemoryCache = MemoryUtCache;
	using Cache = UtCache;
	using CacheModifierProxy = UtCacheModifierProxy;
	using CacheProxy = MemoryCacheProxy<MemoryCache, Cache, CacheModifierProxy>;

	TEST(TEST_CLASS, CanCreateProxyAroundMemoryCache) {
		// Arrange:
		CacheProxy cache(MemoryCacheOptions(1'000'000, 1'000));

		// Act: add some infos
		for (const auto& info : test::CreateTransactionInfos(5))
			cache.modifier().add(info);

		// Assert: check view sizes
		EXPECT_EQ(5u, cache.view().size());
		EXPECT_EQ(5u, static_cast<const MemoryCache&>(cache).view().size());
	}

	namespace {
		class MockMutableCache : public Cache {
		public:
			explicit MockMutableCache(MemoryCache& memoryCache, size_t& numModifyCalls)
					: m_memoryCache(memoryCache)
					, m_numModifierCalls(numModifyCalls)
			{}

		public:
			CacheModifierProxy modifier() override {
				++m_numModifierCalls;
				return m_memoryCache.modifier();
			}

		private:
			MemoryCache& m_memoryCache;
			size_t& m_numModifierCalls;
		};

		std::unique_ptr<Cache> CreateMockMutableCache(MemoryCache& memoryCache, size_t& numModifyCalls) {
			return std::make_unique<MockMutableCache>(memoryCache, numModifyCalls);
		}
	}

	TEST(TEST_CLASS, CanCreateProxyAroundMemoryCacheAggregate) {
		// Arrange:
		size_t numModifierCalls = 0;
		CacheProxy cache(MemoryCacheOptions(1'000'000, 1'000), CreateMockMutableCache, numModifierCalls);

		// Act: add some infos
		for (const auto& info : test::CreateTransactionInfos(5))
			cache.modifier().add(info);

		// Assert: check view sizes
		EXPECT_EQ(5u, cache.view().size());
		EXPECT_EQ(5u, static_cast<const MemoryCache&>(cache).view().size());

		// - importantly notice that modifier() was called on the wrapper, which delegated to the memory cache modifier()
		EXPECT_EQ(5u, numModifierCalls);
	}
}}
