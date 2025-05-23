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

#include "UtTestUtils.h"
#include "catapult/cache_tx/MemoryUtCache.h"
#include "tests/test/core/EntityTestUtils.h"
#include "catapult/model/TransactionFeeCalculator.h"

namespace catapult { namespace test {

	std::unique_ptr<cache::MemoryUtCache> CreateSeededMemoryUtCache(uint32_t count) {
		auto pTransactionFeeCalculator = std::make_shared<model::TransactionFeeCalculator>();
		auto pCache = std::make_unique<cache::MemoryUtCache>(
				cache::MemoryCacheOptions(1000, 1000), pTransactionFeeCalculator);
		auto transactionInfos = CreateTransactionInfos(count);
		AddAll(*pCache, transactionInfos);
		return pCache;
	}

	void AddAll(cache::UtCache& cache, const std::vector<model::TransactionInfo>& transactionInfos) {
		auto modifier = cache.modifier();
		for (const auto& transactionInfo : transactionInfos)
			modifier.add(transactionInfo);
	}

	void RemoveAll(cache::UtCache& cache, const std::vector<Hash256>& hashes) {
		auto modifier = cache.modifier();
		for (const auto& hash : hashes)
			modifier.remove(hash);
	}

	std::vector<Timestamp::ValueType> ExtractRawDeadlines(const cache::MemoryUtCache& cache) {
		std::vector<Timestamp::ValueType> rawDeadlines;

		auto view = cache.view();
		view.forEach([&rawDeadlines](const auto& info) {
			rawDeadlines.push_back(info.pEntity->Deadline.unwrap());
			return true;
		});
		return rawDeadlines;
	}

	std::vector<model::UniqueEntityPtr<const model::Transaction>> ExtractTransactions(const cache::MemoryUtCache& cache, size_t count) {
		std::vector<model::UniqueEntityPtr<const model::Transaction>> transactions;

		auto view = cache.view();
		if (0 != count) {
			view.forEach([count, &transactions](const auto& info) {
				transactions.push_back(CopyEntity(*info.pEntity));
				return count != transactions.size();
			});
		}

		return transactions;
	}

	std::vector<const model::TransactionInfo*> ExtractTransactionInfos(const cache::MemoryUtCacheView& view, size_t count) {
		std::vector<const model::TransactionInfo*> infos;

		if (0 != count) {
			view.forEach([count, &infos](const auto& info) {
				infos.push_back(&info);
				return count != infos.size();
			});
		}

		return infos;
	}

	void AssertDeadlines(const cache::MemoryUtCache& cache, const std::vector<Timestamp::ValueType>& expectedDeadlines) {
		EXPECT_EQ(expectedDeadlines, ExtractRawDeadlines(cache));
	}

	namespace {
		void AssertContainsAll(const cache::MemoryUtCacheView& cacheView, const std::vector<Hash256>& hashes, bool shouldContain) {
			auto i = 0u;
			for (const auto& hash : hashes) {
				if (shouldContain)
					EXPECT_TRUE(cacheView.contains(hash)) << "hash at " << i;
				else
					EXPECT_FALSE(cacheView.contains(hash)) << "hash at " << i;

				++i;
			}
		}
	}

	void AssertContainsAll(const cache::MemoryUtCache& cache, const std::vector<Hash256>& hashes) {
		AssertContainsAll(cache.view(), hashes, true);
	}

	void AssertContainsNone(const cache::MemoryUtCache& cache, const std::vector<Hash256>& hashes) {
		AssertContainsAll(cache.view(), hashes, false);
	}

	void AssertContainsAll(const cache::MemoryUtCacheView& cacheView, const std::vector<Hash256>& hashes) {
		AssertContainsAll(cacheView, hashes, true);
	}

	void AssertContainsNone(const cache::MemoryUtCacheView& cacheView, const std::vector<Hash256>& hashes) {
		AssertContainsAll(cacheView, hashes, false);
	}

	namespace {
		void AssertContainsAll(
				const cache::MemoryUtCache& cache,
				const std::vector<model::TransactionInfo>& transactionInfos,
				bool shouldContain) {
			auto i = 0u;
			auto view = cache.view();
			for (const auto& transactionInfo : transactionInfos) {
				if (shouldContain)
					EXPECT_TRUE(view.contains(transactionInfo.EntityHash)) << "info at " << i;
				else
					EXPECT_FALSE(view.contains(transactionInfo.EntityHash)) << "info at " << i;

				++i;
			}
		}
	}

	void AssertContainsAll(const cache::MemoryUtCache& cache, const std::vector<model::TransactionInfo>& transactionInfos) {
		AssertContainsAll(cache, transactionInfos, true);
	}

	void AssertContainsNone(const cache::MemoryUtCache& cache, const std::vector<model::TransactionInfo>& transactionInfos) {
		AssertContainsAll(cache, transactionInfos, false);
	}
}}
