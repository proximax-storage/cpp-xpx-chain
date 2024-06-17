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

#include "UtSynchronizer.h"
#include "EntitiesSynchronizer.h"
#include "catapult/api/RemoteTransactionApi.h"

namespace catapult { namespace chain {

	namespace {
		struct UtTraits {
		public:
			using RemoteApiType = api::RemoteTransactionApi;
			static constexpr auto Name = "unconfirmed transactions";

		public:
			explicit UtTraits(
					const MinFeeMultiplierSupplier& minFeeMultiplierSupplier,
					const ShortHashesSupplier& shortHashesSupplier,
					const handlers::TransactionRangeHandler& transactionRangeConsumer,
					size_t batchSize)
					: m_minFeeMultiplierSupplier(minFeeMultiplierSupplier)
					, m_shortHashesSupplier(shortHashesSupplier)
					, m_transactionRangeConsumer(transactionRangeConsumer)
					, m_batchSize(batchSize)
			{}

		public:
			thread::future<std::vector<model::TransactionRange>> apiCall(const RemoteApiType& api) const {
				return api.unconfirmedTransactions(m_minFeeMultiplierSupplier(), m_shortHashesSupplier(), m_batchSize);
			}

			void consume(std::vector<model::TransactionRange>&& ranges, const Key& sourcePublicKey) const {
				for (auto& range : ranges)
					m_transactionRangeConsumer(model::AnnotatedTransactionRange(std::move(range), sourcePublicKey));
			}

			static size_t size(const std::vector<model::TransactionRange>& ranges) {
				auto entityCount = 0u;
				for (const auto& range : ranges)
					entityCount += range.size();

				return entityCount;
			}

		private:
			MinFeeMultiplierSupplier m_minFeeMultiplierSupplier;
			ShortHashesSupplier m_shortHashesSupplier;
			handlers::TransactionRangeHandler m_transactionRangeConsumer;
			size_t m_batchSize;
		};
	}

	RemoteNodeSynchronizer<api::RemoteTransactionApi> CreateUtSynchronizer(
			const MinFeeMultiplierSupplier& minFeeMultiplierSupplier,
			const ShortHashesSupplier& shortHashesSupplier,
			const handlers::TransactionRangeHandler& transactionRangeConsumer,
			size_t batchSize) {
		auto traits = UtTraits(minFeeMultiplierSupplier, shortHashesSupplier, transactionRangeConsumer, batchSize);
		auto pSynchronizer = std::make_shared<EntitiesSynchronizer<UtTraits>>(std::move(traits));
		return CreateRemoteNodeSynchronizer(pSynchronizer);
	}
}}
