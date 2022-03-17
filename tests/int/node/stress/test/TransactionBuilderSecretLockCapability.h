/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "TransactionsBuilder.h"
#include "TransactionBuilderCapability.h"

namespace catapult { namespace test {


	/// Transactions builder and generator for transfer and secret lock transactions.
	class TransactionBuilderSecretLockCapability : public TransactionBuilderCapability
	{

	private:
		struct SecretLockDescriptor {
			size_t SenderId;
			size_t RecipientId;
			catapult::Amount Amount;
			BlockDuration Duration;
			Hash256 Secret;
		};

		struct SecretProofDescriptor {
			size_t SenderId;
			size_t RecipientId;
			std::vector<uint8_t> Proof;
		};
	public:
		/// Adds a secret lock from \a senderId to \a recipientId for amount \a transferAmount, specified \a duration and \a proof.
		std::vector<uint8_t> addSecretLock(
				size_t senderId,
				size_t recipientId,
				Amount transferAmount,
				BlockDuration duration,
				const std::vector<uint8_t>& proof);

		/// Adds a secret lock from \a senderId to \a recipientId for amount \a transferAmount and specified \a duration.
		std::vector<uint8_t> addSecretLock(size_t senderId, size_t recipientId, Amount transferAmount, BlockDuration duration);

		/// Adds a secret proof from \a senderId to \a recipientId using \a proof data.
		void addSecretProof(size_t senderId, size_t recipientId, const std::vector<uint8_t>& proof);

	public:
		TransactionBuilderSecretLockCapability(TransactionsBuilder& builder) : TransactionBuilderCapability(builder)
		{

		}
		void registerHooks() override;
	private:
		model::UniqueEntityPtr<model::Transaction> createSecretLock(const SecretLockDescriptor& descriptor, Timestamp deadline);

		model::UniqueEntityPtr<model::Transaction> createSecretProof(const SecretProofDescriptor& descriptor, Timestamp deadline);



	};
}}
