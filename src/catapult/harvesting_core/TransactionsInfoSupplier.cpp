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

#include "TransactionsInfoSupplier.h"
#include "HarvestingUtFacadeFactory.h"
#include "TransactionFeeMaximizer.h"
#include "catapult/cache_tx/MemoryUtCacheUtils.h"
#include "catapult/model/FeeUtils.h"

namespace catapult { namespace harvesting {

	namespace {
		using TransactionInfoPointers = std::vector<const model::TransactionInfo*>;

		enum class SortDirection { Ascending, Descending };

		enum class LimitedTransactionsPlace { Left, Right };

		template<SortDirection Direction, LimitedTransactionsPlace LimitedTransactionsPlace>
		class MaxFeeMultiplierComparer {
		public:
			explicit MaxFeeMultiplierComparer(const model::TransactionFeeCalculator& transactionFeeCalculator)
				: m_transactionFeeCalculator(transactionFeeCalculator)
			{}

		public:
			bool operator()(const model::TransactionInfo* pLhs, const model::TransactionInfo* pRhs) const {
				// Move limited fee transactions (that are actually without fees) to the required end, left or right.
				bool isLeftLimitedFee = m_transactionFeeCalculator.isTransactionFeeLimited(pLhs->pEntity->Type, pLhs->pEntity->EntityVersion());
				bool isRightLimitedFee = m_transactionFeeCalculator.isTransactionFeeLimited(pRhs->pEntity->Type, pRhs->pEntity->EntityVersion());
				if (isLeftLimitedFee && isRightLimitedFee) {
					return false;
				} else if (isLeftLimitedFee) {
					return (LimitedTransactionsPlace == LimitedTransactionsPlace::Left);
				} else if (isRightLimitedFee) {
					return (LimitedTransactionsPlace == LimitedTransactionsPlace::Right);
				}

				// Sort all others.
				auto lhsMaxFeeMultiplier = model::CalculateTransactionMaxFeeMultiplier(*pLhs->pEntity);
				auto rhsMaxFeeMultiplier = model::CalculateTransactionMaxFeeMultiplier(*pRhs->pEntity);
				return SortDirection::Ascending == Direction
						? lhsMaxFeeMultiplier < rhsMaxFeeMultiplier
						: lhsMaxFeeMultiplier > rhsMaxFeeMultiplier;
			}

		private:
			const model::TransactionFeeCalculator& m_transactionFeeCalculator;
		};

		TransactionsInfo ToTransactionsInfo(TransactionInfoPointers&& transactionInfoPointers, BlockFeeMultiplier feeMultiplier) {
			TransactionsInfo transactionsInfo;
			transactionsInfo.FeeMultiplier = feeMultiplier;
			transactionsInfo.Transactions = std::move(transactionInfoPointers);

			CalculateBlockTransactionsHash(transactionsInfo.Transactions, transactionsInfo.TransactionsHash);
			return transactionsInfo;
		}

		TransactionsInfo SupplyOldest(const cache::MemoryUtCacheView& utCacheView, HarvestingUtFacade& utFacade, uint32_t count) {
			// 1. get first transactions from the ut cache
			auto candidates = cache::GetFirstTransactionInfoPointers(utCacheView, count, [&utFacade](const auto& transactionInfo) {
				return utFacade.apply(transactionInfo);
			});

			// 2. pick the smallest multiplier so that all transactions pass validation
			auto minFeeMultiplier = BlockFeeMultiplier();
			if (!candidates.empty()) {
				auto comparer = MaxFeeMultiplierComparer<SortDirection::Ascending, LimitedTransactionsPlace::Right>(utCacheView.transactionFeeCalculator());
				auto minIter = std::min_element(candidates.cbegin(), candidates.cend(), comparer);
				minFeeMultiplier = model::CalculateTransactionMaxFeeMultiplier(*(*minIter)->pEntity);
			}

			return ToTransactionsInfo(std::move(candidates), minFeeMultiplier);
		}

		TransactionsInfo SupplyMinimumFee(const cache::MemoryUtCacheView& utCacheView, HarvestingUtFacade& utFacade, uint32_t count) {
			// 1. get all transactions from the ut cache
			const auto& transactionFeeCalculator = utCacheView.transactionFeeCalculator();
			auto comparer = MaxFeeMultiplierComparer<SortDirection::Ascending, LimitedTransactionsPlace::Left>(transactionFeeCalculator);
			auto candidates = cache::GetFirstTransactionInfoPointers(utCacheView, count, comparer, [&utFacade](const auto& transactionInfo) {
				return utFacade.apply(transactionInfo);
			});

			// 2. pick the smallest multiplier so that all transactions pass validation
			auto minFeeMultiplier = BlockFeeMultiplier();
			for (const auto& pTransactionInfo : candidates) {
				const auto& pTransaction = pTransactionInfo->pEntity;
				if (!transactionFeeCalculator.isTransactionFeeLimited(pTransaction->Type, pTransaction->EntityVersion())) {
					minFeeMultiplier = model::CalculateTransactionMaxFeeMultiplier(*pTransaction);
					break;
				}
			}

			return ToTransactionsInfo(std::move(candidates), minFeeMultiplier);
		}

		TransactionsInfo SupplyMaximumFee(const cache::MemoryUtCacheView& utCacheView, HarvestingUtFacade& utFacade, uint32_t count) {
			// 1. get all transactions from the ut cache
			auto comparer = MaxFeeMultiplierComparer<SortDirection::Descending, LimitedTransactionsPlace::Left>(utCacheView.transactionFeeCalculator());
			auto maximizer = TransactionFeeMaximizer();
			auto numLimitedFeeTransactions = 0u;
			auto candidates = cache::GetFirstTransactionInfoPointers(utCacheView, count, comparer,
					[&utFacade, &maximizer, &numLimitedFeeTransactions, &transactionFeeCalculator=utCacheView.transactionFeeCalculator()](const auto& transactionInfo) {
				if (!utFacade.apply(transactionInfo))
					return false;

				auto pTransaction = transactionInfo.pEntity;
				if (transactionFeeCalculator.isTransactionFeeLimited(pTransaction->Type, pTransaction->EntityVersion())) {
					numLimitedFeeTransactions++;
				} else {
					maximizer.apply(transactionInfo, transactionFeeCalculator);
				}

				return true;
			});

			// 2. pick the best fee policy and truncate the transactions and facade
			const auto& bestFeePolicy = maximizer.best();
			auto numTransactions = bestFeePolicy.NumTransactions + numLimitedFeeTransactions;
			candidates.resize(numTransactions);
			while (utFacade.size() > numTransactions)
				utFacade.unapply();

			return ToTransactionsInfo(std::move(candidates), bestFeePolicy.FeeMultiplier);
		}
	}

	TransactionsInfoSupplier CreateTransactionsInfoSupplier(
			model::TransactionSelectionStrategy strategy,
			const cache::MemoryUtCache& utCache) {
		return [strategy, &utCache](auto& utFacade, auto count) {
			auto utCacheView = utCache.view();

			switch (strategy) {
			case model::TransactionSelectionStrategy::Minimize_Fee:
				return SupplyMinimumFee(utCacheView, utFacade, count);

			case model::TransactionSelectionStrategy::Maximize_Fee:
				return SupplyMaximumFee(utCacheView, utFacade, count);

			default:
				return SupplyOldest(utCacheView, utFacade, count);
			};
		};
	}
}}
