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

#include "RealTransactionFactory.h"
#include "sdk/src/builders/AddressAliasBuilder.h"
#include "sdk/src/builders/RegisterNamespaceBuilder.h"
#include "sdk/src/builders/TransferBuilder.h"
#include "sdk/src/extensions/ConversionExtensions.h"
#include "sdk/src/extensions/TransactionExtensions.h"
#include "plugins/txes/namespace/src/model/NamespaceIdGenerator.h"
#include "catapult/crypto/KeyPair.h"
#include "catapult/model/Address.h"
#include "tests/test/nodeps/Nemesis.h"
#include "tests/test/nodeps/Random.h"
#include "tests/test/nodeps/TestConstants.h"

namespace catapult { namespace test {

	namespace {
		constexpr auto Network_Identifier = model::NetworkIdentifier::Mijin_Test;
	}

	// region transfer transaction

	namespace {
		model::UniqueEntityPtr<model::Transaction> CreateUnsignedTransferTransaction(
				const Key& signerPublicKey,
				const UnresolvedAddress& recipient,
				const std::vector<uint8_t>& message,
				const std::vector<model::UnresolvedMosaic>& mosaics) {
			builders::TransferBuilder builder(Network_Identifier, signerPublicKey);
			builder.setRecipient(recipient);

			if (!message.empty())
				builder.setMessage(message);

			for (const auto& mosaic : mosaics)
				builder.addMosaic(mosaic);

			auto pTransfer = builder.build();
			pTransfer->MaxFee = GenerateRandomValue<Amount>();
			pTransfer->Deadline = GenerateRandomValue<Timestamp>();
			return std::move(pTransfer);
		}
	}

	model::UniqueEntityPtr<model::Transaction> CreateUnsignedTransferTransaction(
			const Key& signerPublicKey,
			const UnresolvedAddress& recipient,
			Amount amount) {
		auto mosaicId = extensions::CastToUnresolvedMosaicId(Default_Currency_Mosaic_Id);
		return CreateUnsignedTransferTransaction(signerPublicKey, recipient, {}, { { mosaicId, amount } });
	}

	model::UniqueEntityPtr<model::Transaction> CreateTransferTransaction(
			const crypto::KeyPair& signer,
			const UnresolvedAddress& recipient,
			Amount amount) {
		auto pTransaction = CreateUnsignedTransferTransaction(signer.publicKey(), recipient, amount);
		extensions::TransactionExtensions(GetNemesisGenerationHash()).sign(signer, *pTransaction);
		return pTransaction;
	}

	model::UniqueEntityPtr<model::Transaction> CreateTransferTransaction(
			const crypto::KeyPair& signer,
			const Key& recipientPublicKey,
			Amount amount) {
		auto recipient = model::PublicKeyToAddress(recipientPublicKey, Network_Identifier);
		return CreateTransferTransaction(signer, extensions::CopyToUnresolvedAddress(recipient), amount);
	}

	// endregion

	// region namespace transactions

	model::UniqueEntityPtr<model::Transaction> CreateRegisterRootNamespaceTransaction(
			const crypto::KeyPair& signer,
			const std::string& name,
			BlockDuration duration) {
		builders::RegisterNamespaceBuilder builder(Network_Identifier, signer.publicKey());
		builder.setName({ reinterpret_cast<const uint8_t*>(name.data()), name.size() });
		builder.setDuration(duration);
		auto pTransaction = builder.build();

		extensions::TransactionExtensions(GetNemesisGenerationHash()).sign(signer, *pTransaction);
		return std::move(pTransaction);
	}

	model::UniqueEntityPtr<model::Transaction> CreateRootAddressAliasTransaction(
			const crypto::KeyPair& signer,
			const std::string& name,
			const Address& address) {
		auto namespaceId = model::GenerateNamespaceId(NamespaceId(), name);
		builders::AddressAliasBuilder builder(Network_Identifier, signer.publicKey());
		builder.setNamespaceId(namespaceId);
		builder.setAddress(address);
		auto pTransaction = builder.build();

		extensions::TransactionExtensions(GetNemesisGenerationHash()).sign(signer, *pTransaction);
		return std::move(pTransaction);
	}

	// endregion
}}
