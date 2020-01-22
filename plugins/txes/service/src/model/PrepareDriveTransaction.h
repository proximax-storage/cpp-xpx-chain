/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "ServiceEntityType.h"
#include "catapult/model/Transaction.h"
#include "plugins/txes/multisig/src/model/ModifyMultisigAccountTransaction.h"

namespace catapult { namespace model {

#pragma pack(push, 1)

	/// Binary layout for a prepare drive transaction body.
	template<typename THeader>
	struct PrepareDriveTransactionBody : public THeader {
	private:
		using TransactionType = PrepareDriveTransactionBody<THeader>;

	public:
		DEFINE_TRANSACTION_CONSTANTS(Entity_Type_PrepareDrive, 1)

	public:
		/// Key of drive.
		Key DriveKey;

		/// Duration of drive.
		BlockDuration Duration;

		/// Billing period of drive.
		BlockDuration BillingPeriod;

		/// Price of drive for one billing period(storage units).
		Amount BillingPrice;

		/// Size of drive.
		uint64_t DriveSize;

		/// Count of replicas for drive.
		uint16_t Replicas;

		/// Minimal count of replicator to send transaction from name of drive.
        uint16_t MinReplicators;

		/// Percent of approves from replicators to apply transaction.
        uint8_t PercentApprovers;

	public:
		// Calculates the real size of a service \a transaction.
		static constexpr uint64_t CalculateRealSize(const TransactionType&) noexcept {
			return sizeof(TransactionType);
		}
	};

	DEFINE_EMBEDDABLE_TRANSACTION(PrepareDrive)

#pragma pack(pop)

	/// Extracts public keys of additional accounts that must approve \a transaction.
	inline utils::KeySet ExtractAdditionalRequiredCosigners(const EmbeddedPrepareDriveTransaction& transaction) {
		return { transaction.DriveKey };
	}
}}
