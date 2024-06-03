/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "StorageEntityType.h"
#include "catapult/model/Transaction.h"

namespace catapult { namespace model {

#pragma pack(push, 1)

	/// Binary layout for a prepare drive transaction body.
	template<typename THeader>
	struct ReplicatorOnboardingTransactionBody : public THeader {
	private:
		using TransactionType = ReplicatorOnboardingTransactionBody<THeader>;

	public:
		explicit ReplicatorOnboardingTransactionBody<THeader>() = default;

	public:
		DEFINE_TRANSACTION_CONSTANTS(Entity_Type_ReplicatorOnboarding, 2)

	public:
		/// The storage size that the replicator provides to the system.
		Amount Capacity;

		/// The boot public key of the node where this replicator will be running on.
		Key NodeBootKey;

		/// The message signed by the boot private key of the node.
		Hash256 Message;

		/// The signature of the message.
		Signature MessageSignature;

	public:
		// Calculates the real size of a storage \a transaction.
		static constexpr uint64_t CalculateRealSize(const TransactionType&) noexcept {
			return sizeof(TransactionType);
		}
	};

	DEFINE_EMBEDDABLE_TRANSACTION(ReplicatorOnboarding)

#pragma pack(pop)
}}
