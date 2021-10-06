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

#include "AggregateTransaction.h"

namespace catapult { namespace model {

	template<uint32_t TCoSignatureVersion>
	size_t GetTransactionPayloadSize(const AggregateTransactionHeader<TCoSignatureVersion>& header) {
		return header.PayloadSize;
	}

	namespace {
		template<uint32_t TCoSignatureVersion>
		constexpr bool IsPayloadSizeValid(const AggregateTransaction<TCoSignatureVersion>& aggregate) {
			return
					aggregate.Size >= sizeof(AggregateTransaction<TCoSignatureVersion>) &&
					aggregate.Size - sizeof(AggregateTransaction<TCoSignatureVersion>) >= aggregate.PayloadSize &&
					0 == (aggregate.Size - sizeof(AggregateTransaction<TCoSignatureVersion>) - aggregate.PayloadSize) % sizeof(Cosignature<2>);
		}
	}

	template<uint32_t TCoSignatureVersion>
	bool IsSizeValid(const AggregateTransaction<TCoSignatureVersion>& aggregate, const TransactionRegistry& registry) {
		if (!IsPayloadSizeValid<TCoSignatureVersion>(aggregate)) {
			CATAPULT_LOG(warning)
					<< "aggregate transaction failed size validation with size "
					<< aggregate.Size << " and payload size " << aggregate.PayloadSize;
			return false;
		}

		auto transactions = aggregate.Transactions(EntityContainerErrorPolicy::Suppress);
		auto areAllTransactionsValid = std::all_of(transactions.cbegin(), transactions.cend(), [&registry](const auto& transaction) {
			return IsSizeValid(transaction, registry);
		});

		if (areAllTransactionsValid && !transactions.hasError())
			return true;

		CATAPULT_LOG(warning)
				<< "aggregate transactions failed size validation (valid sizes? " << areAllTransactionsValid
				<< ", errors? " << transactions.hasError() << ")";
		return false;
	}
	template size_t GetTransactionPayloadSize<1>(const AggregateTransactionHeader<1>& header);
	template bool IsSizeValid<1>(const AggregateTransaction<1>& aggregate, const TransactionRegistry& registry);
}}
