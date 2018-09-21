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

#include "catapult/api/LocalChainApi.h"
#include "catapult/model/EntityHasher.h"
#include "tests/test/core/mocks/MockMemoryBasedStorage.h"
#include "tests/TestHarness.h"

namespace catapult { namespace api {

#define TEST_CLASS LocalChainApiTests

	namespace {
		std::unique_ptr<ChainApi> CreateLocalChainApi(const io::BlockStorageCache& storage, uint32_t maxHashes) {
			return api::CreateLocalChainApi(storage, maxHashes);
		}
	}

	// region chainInfo

	TEST(TEST_CLASS, CanRetrieveChainInfo) {
		// Arrange:
		auto numSupplierCalls = 0;

		auto pStorage = mocks::CreateMemoryBasedStorageCache(12);
		auto pApi = api::CreateLocalChainApi(*pStorage, 5);

		// Act:
		auto info = pApi->chainInfo().get();

		// Assert:
		EXPECT_EQ(1u, numSupplierCalls);
		EXPECT_EQ(Height(12), info.Height);
	}

	// endregion

	// region height-based errors

	namespace {
		struct HashesFromTraits {
			static auto Invoke(ChainApi& api, Height height) {
				return api.hashesFrom(height);
			}
		};

		template<typename TTraits>
		void AssertApiErrorForHeight(uint32_t numBlocks, Height requestHeight) {
			// Arrange:
			auto pStorage = mocks::CreateMemoryBasedStorageCache(numBlocks);
			auto pApi = CreateLocalChainApi(*pStorage, 5);

			// Act + Assert:
			auto future = TTraits::Invoke(*pApi, requestHeight);
			EXPECT_THROW(future.get(), catapult_api_error);
		}
	}

#define CHAIN_API_HEIGHT_ERROR_TRAITS_BASED_TEST(TEST_NAME) \
	template<typename TTraits> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)(); \
	TEST(TEST_CLASS, TEST_NAME##_HashesFrom) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<HashesFromTraits>(); } \
	template<typename TTraits> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)()

	CHAIN_API_HEIGHT_ERROR_TRAITS_BASED_TEST(RequestAtHeightZeroFails) {
		// Assert:
		AssertApiErrorForHeight<TTraits>(12, Height(0));
	}

	CHAIN_API_HEIGHT_ERROR_TRAITS_BASED_TEST(RequestAtHeightGreaterThanLocalChainHeightFails) {
		// Assert:
		AssertApiErrorForHeight<TTraits>(12, Height(13));
		AssertApiErrorForHeight<TTraits>(12, Height(100));
	}

	// endregion

	// region hashesFrom

	namespace {
		void AssertCanRetrieveHashes(
				uint32_t numBlocks,
				uint32_t maxHashes,
				Height requestHeight,
				const std::vector<Height>& expectedHeights) {
			// Arrange:
			auto pStorage = mocks::CreateMemoryBasedStorageCache(numBlocks);
			auto pApi = CreateLocalChainApi(*pStorage, maxHashes);

			// Act:
			auto hashes = pApi->hashesFrom(requestHeight).get();

			// Assert:
			ASSERT_EQ(expectedHeights.size(), hashes.size());

			auto i = 0u;
			auto storageView = pStorage->view();
			for (const auto& hash : hashes) {
				// - calculate the expected hash
				auto pBlock = storageView.loadBlock(expectedHeights[i]);
				auto expectedHash = CalculateHash(*pBlock);

				// - compare
				EXPECT_EQ(expectedHash, hash) << "comparing hashes at " << i << " from " << requestHeight;
				++i;
			}
		}
	}

	TEST(TEST_CLASS, CanRetrieveAtMostMaxHashes) {
		// Assert:
		AssertCanRetrieveHashes(12, 5, Height(3), { Height(3), Height(4), Height(5), Height(6), Height(7) });
	}

	TEST(TEST_CLASS, RetreivedHashesAreBoundedByLastBlock) {
		// Assert:
		AssertCanRetrieveHashes(12, 10, Height(10), { Height(10), Height(11), Height(12) });
	}

	TEST(TEST_CLASS, CanRetrieveLastBlockHash) {
		// Assert:
		AssertCanRetrieveHashes(12, 5, Height(12), { Height(12) });
	}

	// endregion
}}
