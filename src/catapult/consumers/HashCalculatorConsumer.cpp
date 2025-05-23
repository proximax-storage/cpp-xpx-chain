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

#include "BlockConsumers.h"
#include "ConsumerResultFactory.h"
#include "TransactionConsumers.h"
#include "catapult/crypto/MerkleHashBuilder.h"
#include "catapult/model/EntityHasher.h"

namespace catapult { namespace consumers {

	namespace {
		class BlockHashCalculatorConsumer {
		public:
			BlockHashCalculatorConsumer(const GenerationHash& generationHash, const model::TransactionRegistry& transactionRegistry)
					: m_generationHash(generationHash)
					, m_transactionRegistry(transactionRegistry)
			{}

		public:
			ConsumerResult operator()(BlockElements& elements) const {
				if (elements.empty())
					return Abort(Failure_Consumer_Empty_Input);

				for (auto& element : elements) {
					// note that disruptor input elements have been extracted from a packet (or created within this
					// process), so their sizes have already been validated
					crypto::MerkleHashBuilder transactionsHashBuilder;
					for (const auto& transaction : element.Block.Transactions()) {
						model::TransactionElement transactionElement(transaction);
						model::UpdateHashes(m_transactionRegistry, m_generationHash, transactionElement);
						element.Transactions.push_back(transactionElement);

						transactionsHashBuilder.update(transactionElement.MerkleComponentHash);
					}

					Hash256 transactionsHash;
					transactionsHashBuilder.final(transactionsHash);
					if (element.Block.BlockTransactionsHash != transactionsHash)
						return Abort(Failure_Consumer_Block_Transactions_Hash_Mismatch);

					element.EntityHash = model::CalculateHash(element.Block);
				}

				return Continue();
			}

		private:
			GenerationHash m_generationHash;
			const model::TransactionRegistry& m_transactionRegistry;
		};
	}

	disruptor::BlockConsumer CreateBlockHashCalculatorConsumer(
			const GenerationHash& generationHash,
			const model::TransactionRegistry& transactionRegistry) {
		return BlockHashCalculatorConsumer(generationHash, transactionRegistry);
	}

	namespace {
		class TransactionHashCalculatorConsumer {
		public:
			TransactionHashCalculatorConsumer(const GenerationHash& generationHash, const model::TransactionRegistry& transactionRegistry)
					: m_generationHash(generationHash)
					, m_transactionRegistry(transactionRegistry)
			{}

		public:
			ConsumerResult operator()(TransactionElements& elements) const {
				if (elements.empty())
					return Abort(Failure_Consumer_Empty_Input);

				for (auto& element : elements)
					model::UpdateHashes(m_transactionRegistry, m_generationHash, element);

				return Continue();
			}

		private:
			GenerationHash m_generationHash;
			const model::TransactionRegistry& m_transactionRegistry;
		};
	}

	disruptor::TransactionConsumer CreateTransactionHashCalculatorConsumer(
			const GenerationHash& generationHash,
			const model::TransactionRegistry& transactionRegistry) {
		return TransactionHashCalculatorConsumer(generationHash, transactionRegistry);
	}
}}
