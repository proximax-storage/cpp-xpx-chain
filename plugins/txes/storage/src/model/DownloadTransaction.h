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

	/// Binary layout for a download transaction body.
	template<typename THeader>
	struct DownloadTransactionBody : public THeader {
	private:
		using TransactionType = DownloadTransactionBody<THeader>;

	public:
		explicit DownloadTransactionBody<THeader>()
		{}

	public:
		DEFINE_TRANSACTION_CONSTANTS(Entity_Type_Download, 1)

	public:
		/// Public key of the drive.
		Key DriveKey;

		/// Prepaid Download Size.
		uint64_t DownloadSizeMegabytes;

		/// XPXs to lock for future payment for.
		Amount FeedbackFeeAmount;

		/// Size of the list of public keys
		uint16_t ListOfPublicKeysSize;

		/// List of public keys
		DEFINE_TRANSACTION_VARIABLE_DATA_ACCESSORS(ListOfPublicKeys, Key)

	private:
		template<typename T>
		static auto* ListOfPublicKeysPtrT(T& transaction) {
			auto* pPayloadStart = THeader::PayloadStart(transaction);
			return transaction.ListOfPublicKeysSize ? pPayloadStart : nullptr;
		}

	public:
		// Calculates the real size of a storage \a transaction.
		static constexpr uint64_t CalculateRealSize(const TransactionType& transaction) noexcept {
			return sizeof(TransactionType) + transaction.ListOfPublicKeysSize * sizeof(Key);
		}
	};

	DEFINE_EMBEDDABLE_TRANSACTION(Download)

#pragma pack(pop)
}}
