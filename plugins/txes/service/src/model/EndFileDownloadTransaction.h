/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "ServiceEntityType.h"
#include "ServiceTypes.h"
#include "catapult/model/Transaction.h"

namespace catapult { namespace model {

#pragma pack(push, 1)

	/// Binary layout for an end file download transaction body.
	template<typename THeader>
	struct EndFileDownloadTransactionBody : public THeader {
	private:
		using TransactionType = EndFileDownloadTransactionBody<THeader>;

	public:
		DEFINE_TRANSACTION_CONSTANTS(Entity_Type_EndFileDownload, 1)

	public:
		/// Key of the file recipient.
		Key Recipient;

		/// Unique operation token.
		Hash256 OperationToken;

		/// Count of downloaded files.
		uint16_t FileCount;

		/// Hashes of the files downloaded from drive.
		DEFINE_TRANSACTION_VARIABLE_DATA_ACCESSORS(Files, File)

	private:
		template<typename T>
		static auto* FilesPtrT(T& transaction) {
			auto* pPayloadStart = THeader::PayloadStart(transaction);
			return transaction.FileCount ? pPayloadStart : nullptr;
		}

	public:
		// Calculates the real size of an end file download \a transaction.
		static constexpr uint64_t CalculateRealSize(const TransactionType& transaction) noexcept {
			return 	sizeof(TransactionType) + transaction.FileCount * sizeof(File);
		}
	};

	DEFINE_EMBEDDABLE_TRANSACTION(EndFileDownload)

#pragma pack(pop)
}}
