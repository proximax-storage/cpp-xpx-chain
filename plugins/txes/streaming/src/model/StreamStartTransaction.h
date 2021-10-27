/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "StreamingEntityType.h"
#include "catapult/model/Transaction.h"
#include "catapult/utils/ArraySet.h"

namespace catapult { namespace model {

#pragma pack(push, 1)

	/// Binary layout for a data modification transaction body.
	template<typename THeader>
	struct StreamStartTransactionBody : public THeader {
	private:
		using TransactionType = StreamStartTransactionBody<THeader>;

	public:
		explicit StreamStartTransactionBody<THeader>()
		{}

	public:
		DEFINE_TRANSACTION_CONSTANTS(Entity_Type_StreamStart, 1)

	public:
		/// Key of drive.
		Key DriveKey;

		/// Expected size of stream.
		uint64_t ExpectedUploadSize;

		/// Message of folderName string in bytes
		uint16_t FolderNameSize;

		/// Amount of XPXs to transfer to the drive.
		Amount FeedbackFeeAmount;

		/// FolderName to save stream in.
		// Followed by folderName if FolderNameSize != 0
		DEFINE_TRANSACTION_VARIABLE_DATA_ACCESSORS(FolderName, uint8_t)

	private:
		template<typename T>
		static auto* FolderNamePtrT(T& transaction) {
			return transaction.FolderNameSize ? THeader::PayloadStart(transaction) : nullptr;
		}

	public:
		// Calculates the real size of a data modification \a transaction.
		static uint64_t CalculateRealSize(const TransactionType& transaction) noexcept {
			return sizeof(TransactionType) + transaction.FolderNameSize;
		}
	};

	DEFINE_EMBEDDABLE_TRANSACTION(StreamStart)

#pragma pack(pop)
}}
