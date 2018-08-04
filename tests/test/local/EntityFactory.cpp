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

#include "EntityFactory.h"
#include "sdk/src/builders/TransferBuilder.h"
#include "sdk/src/extensions/TransactionExtensions.h"
#include "catapult/crypto/KeyPair.h"
#include "catapult/crypto/Signer.h"
#include "catapult/model/Address.h"
#include "catapult/model/NetworkInfo.h"
#include "catapult/constants.h"
#include "tests/test/core/AddressTestUtils.h"
#include "tests/test/core/BlockTestUtils.h"
#include "tests/test/nodeps/Random.h"
#include <string.h>

namespace catapult { namespace test {

	namespace {
		constexpr auto Network_Identifier = model::NetworkIdentifier::Mijin_Test;
	}

	std::unique_ptr<model::Transaction> CreateUnsignedTransferTransaction(
			const Key& signerPublicKey,
			const Address& recipient,
			const std::vector<uint8_t>& message,
			const std::vector<model::Mosaic>& mosaics) {
		builders::TransferBuilder builder(Network_Identifier, signerPublicKey, recipient);
		if (!message.empty())
			builder.setMessage(message);

		for (const auto& mosaic : mosaics)
			builder.addMosaic(mosaic.MosaicId, mosaic.Amount);

		auto pTransfer = builder.build();
		pTransfer->Fee = GenerateRandomValue<Amount>();
		pTransfer->Deadline = GenerateRandomValue<Timestamp>();
		return std::move(pTransfer);
	}

	std::unique_ptr<model::Transaction> CreateUnsignedTransferTransaction(
			const Key& signerPublicKey,
			const Address& recipient,
			Amount amount) {
		return CreateUnsignedTransferTransaction(signerPublicKey, recipient, {}, { { Xpx_Id, amount } });
	}

	std::unique_ptr<model::Transaction> CreateTransferTransaction(const crypto::KeyPair& signer, const Address& recipient, Amount amount) {
		auto pTransaction = CreateUnsignedTransferTransaction(signer.publicKey(), recipient, amount);
		extensions::SignTransaction(signer, *pTransaction);
		return pTransaction;
	}

	std::unique_ptr<model::Transaction> CreateTransferTransaction(
			const crypto::KeyPair& signer,
			const Key& recipientPublicKey,
			Amount amount) {
		auto recipient = model::PublicKeyToAddress(recipientPublicKey, Network_Identifier);
		return CreateTransferTransaction(signer, recipient, amount);
	}

	std::unique_ptr<model::Transaction> GenerateRandomTransferTransaction() {
		auto signer = GenerateKeyPair();
		auto recipient = GenerateRandomAddress();
		return CreateTransferTransaction(signer, recipient, GenerateRandomValue<Amount>());
	}

	namespace {
		ConstTransactions GenerateRandomTransferTransactions(size_t count) {
			ConstTransactions transactions;
			for (auto i = 0u; i < count; ++i)
				transactions.push_back(GenerateRandomTransferTransaction());

			return transactions;
		}
	}

	std::unique_ptr<model::Block> GenerateBlockWithTransferTransactionsAtHeight(size_t numTransactions, Height height) {
		auto transactions = GenerateRandomTransferTransactions(numTransactions);
		auto pBlock = GenerateRandomBlockWithTransactions(transactions);
		pBlock->Height = height;
		return pBlock;
	}
}}
