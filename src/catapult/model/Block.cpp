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

#include "Block.h"

namespace catapult { namespace model {

	size_t BlockHeader::GetHeaderSize() const {
		switch (EntityVersion()) {
			case 3:
				return sizeof(BlockHeader);
			default:
				return sizeof(BlockHeaderV4);
		}
	}

	size_t GetTransactionPayloadSize(const BlockHeader& header) {
		switch (header.EntityVersion()) {
			case 3:
				return header.Size - sizeof(BlockHeader);
			default:
				return reinterpret_cast<const BlockHeaderV4&>(header).TransactionPayloadSize;
		}
	}

	namespace {
		bool IsPayloadSizeValid(const Block& block) {
			switch (block.EntityVersion()) {
				case 3: {
					return block.Size >= sizeof(BlockHeader);
				}
				default: {
					auto transactionPayloadSize = reinterpret_cast<const BlockHeaderV4&>(block).TransactionPayloadSize;
					return
						block.Size >= sizeof(BlockHeaderV4) &&
						block.Size - sizeof(BlockHeaderV4) >= transactionPayloadSize &&
						0 == (block.Size - sizeof(BlockHeaderV4) - transactionPayloadSize) % sizeof(Cosignature);
				}
			}
		}
	}

	bool IsSizeValid(const Block& block, const TransactionRegistry& registry) {
		if (!IsPayloadSizeValid(block)) {
			CATAPULT_LOG(warning) << block.Type << " block failed size validation with size " << block.Size;
			return false;
		}

		auto transactions = block.Transactions(EntityContainerErrorPolicy::Suppress);
		auto areAllTransactionsValid = std::all_of(transactions.cbegin(), transactions.cend(), [&registry](const auto& transaction) {
			return IsSizeValid(transaction, registry);
		});

		if (areAllTransactionsValid && !transactions.hasError())
			return true;

		CATAPULT_LOG(warning)
				<< "block transactions failed size validation (valid sizes? " << areAllTransactionsValid
				<< ", errors? " << transactions.hasError() << ")";
		return false;
	}
}}
