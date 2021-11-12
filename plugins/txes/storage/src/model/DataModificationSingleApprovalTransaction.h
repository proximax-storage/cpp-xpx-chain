/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "StorageEntityType.h"
#include "catapult/model/Transaction.h"
#include "catapult/utils/ArraySet.h"

namespace catapult { namespace model {

#pragma pack(push, 1)

	/// Binary layout for a data modification single approval transaction body.
	template<typename THeader>
	struct DataModificationSingleApprovalTransactionBody : public THeader {
	private:
		using TransactionType = DataModificationSingleApprovalTransactionBody<THeader>;

	public:
		explicit DataModificationSingleApprovalTransactionBody<THeader>()
		{}

	public:
		DEFINE_TRANSACTION_CONSTANTS(Entity_Type_DataModificationSingleApproval, 1)

	public:
		/// Key of drive.
		Key DriveKey;

		/// Identifier of the transaction that initiated the modification.
		Hash256 DataModificationId;

		/// Total used disk space of the drive.
		uint64_t UsedDriveSize;

		/// Number of replicators' public keys.
		uint8_t PublicKeysCount;

		/// List of the Uploader keys (current Replicators of the Drive or the Drive Owner).
		DEFINE_TRANSACTION_VARIABLE_DATA_ACCESSORS(PublicKeys, Key)

		/// One-dimensional array of opinion elements (how much each Uploader has uploaded to the signer).
		DEFINE_TRANSACTION_VARIABLE_DATA_ACCESSORS(Opinions, uint64_t)

	private:
		template<typename T>
		static auto* PublicKeysPtrT(T& transaction) {
			auto* pPayloadStart = THeader::PayloadStart(transaction);
			return transaction.PublicKeysCount ? pPayloadStart : nullptr;
		}

		template<typename T>
		static auto* OpinionsPtrT(T& transaction) {
			auto* pPayloadStart = THeader::PayloadStart(transaction);
			return transaction.PublicKeysCount ? pPayloadStart + transaction.PublicKeysCount * sizeof(Key) : nullptr;
		}

	public:
		// Calculates the real size of a data modification single approval \a transaction.
		static constexpr uint64_t CalculateRealSize(const TransactionType& transaction) noexcept {
			return sizeof(TransactionType) + transaction.PublicKeysCount * (sizeof(Key) + sizeof(uint64_t));
		}
	};

	DEFINE_EMBEDDABLE_TRANSACTION(DataModificationSingleApproval)

#pragma pack(pop)
}}
