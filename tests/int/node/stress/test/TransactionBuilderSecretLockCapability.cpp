/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "TransactionBuilderSecretLockCapability.h"
#include "sdk/src/builders/SecretLockBuilder.h"
#include "sdk/src/builders/SecretProofBuilder.h"
#include "sdk/src/extensions/ConversionExtensions.h"
#include "catapult/crypto/Hashes.h"
#include "tests/test/nodeps/Random.h"
#include "tests/test/nodeps/TestConstants.h"

namespace catapult { namespace test {

	namespace {
		constexpr auto Network_Identifier = model::NetworkIdentifier::Mijin_Test;
	}


	// endregion

	// region add / create

	void TransactionBuilderSecretLockCapability::registerHooks() {
        auto self = ptr<TransactionBuilderSecretLockCapability>();
        m_builder.registerDescriptor(DescriptorTypes::Secret_Lock, [self](auto& pDescriptor, auto deadline){
            return self->createSecretLock(CastToDescriptor<SecretLockDescriptor>(pDescriptor), deadline);
        });

        m_builder.registerDescriptor(DescriptorTypes::Secret_Proof, [self](auto& pDescriptor, auto deadline){
            return self->createSecretProof(CastToDescriptor<SecretProofDescriptor>(pDescriptor), deadline);
        });
	}
	std::vector<uint8_t> TransactionBuilderSecretLockCapability::addSecretLock(
			size_t senderId,
			size_t recipientId,
			Amount transferAmount,
			BlockDuration duration,
			const std::vector<uint8_t>& proof) {
		Hash256 secret;
		crypto::Sha3_256(proof, secret);

		auto descriptor = SecretLockDescriptor{ senderId, recipientId, transferAmount, duration, secret };
		add(DescriptorTypes::Secret_Lock, descriptor);
		return proof;
	}

	std::vector<uint8_t> TransactionBuilderSecretLockCapability::addSecretLock(
			size_t senderId,
			size_t recipientId,
			Amount transferAmount,
			BlockDuration duration) {
		return addSecretLock(senderId, recipientId, transferAmount, duration, test::GenerateRandomVector(25));
	}

	void TransactionBuilderSecretLockCapability::addSecretProof(size_t senderId, size_t recipientId, const std::vector<uint8_t>& proof) {
		auto descriptor = SecretProofDescriptor{ senderId, recipientId, proof };
		add(DescriptorTypes::Secret_Proof, descriptor);
	}

	model::UniqueEntityPtr<model::Transaction> TransactionBuilderSecretLockCapability::createSecretLock(
			const SecretLockDescriptor& descriptor,
			Timestamp deadline) {
		const auto& senderKeyPair = accounts().getKeyPair(descriptor.SenderId);
		auto recipientAddress = extensions::CopyToUnresolvedAddress(accounts().getAddress(descriptor.RecipientId));

		builders::SecretLockBuilder builder(Network_Identifier, senderKeyPair.publicKey());
		builder.setMosaic({ extensions::CastToUnresolvedMosaicId(Default_Currency_Mosaic_Id), descriptor.Amount });
		builder.setDuration(descriptor.Duration);
		builder.setHashAlgorithm(model::LockHashAlgorithm::Op_Sha3_256);
		builder.setSecret(descriptor.Secret);
		builder.setRecipient(recipientAddress);
		auto pTransaction = builder.build();

		return SignWithDeadline(std::move(pTransaction), senderKeyPair, deadline);
	}


	model::UniqueEntityPtr<model::Transaction> TransactionBuilderSecretLockCapability::createSecretProof(
			const SecretProofDescriptor& descriptor,
			Timestamp deadline) {
		const auto& senderKeyPair = accounts().getKeyPair(descriptor.SenderId);
		auto recipientAddress = extensions::CopyToUnresolvedAddress(accounts().getAddress(descriptor.RecipientId));

		Hash256 secret;
		crypto::Sha3_256(descriptor.Proof, secret);

		builders::SecretProofBuilder builder(Network_Identifier, senderKeyPair.publicKey());
		builder.setHashAlgorithm(model::LockHashAlgorithm::Op_Sha3_256);
		builder.setSecret(secret);
		builder.setRecipient(recipientAddress);
		builder.setProof(descriptor.Proof);
		auto pTransaction = builder.build();

		return SignWithDeadline(std::move(pTransaction), senderKeyPair, deadline);
	}

	// endregion
}}
