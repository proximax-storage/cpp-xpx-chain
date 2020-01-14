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

	/// Binary layout for a start file download transaction body.
	template<typename THeader>
	struct StartFileDownloadTransactionBody : public THeader {
	private:
		using TransactionType = StartFileDownloadTransactionBody<THeader>;

	public:
		DEFINE_TRANSACTION_CONSTANTS(Entity_Type_StartFileDownload, 1)

	public:
		/// Drive key.
		Key DriveKey;

		/// Unique operation token.
		Hash256 OperationToken;

		/// Count of files to download.
		uint16_t FileCount;

		/// Actions to download files from drive.
		DEFINE_TRANSACTION_VARIABLE_DATA_ACCESSORS(Files, DownloadAction)

	private:
		template<typename T>
		static auto* FilesPtrT(T& transaction) {
			auto* pPayloadStart = THeader::PayloadStart(transaction);
			return transaction.FileCount ? pPayloadStart : nullptr;
		}

	public:
		// Calculates the real size of a start file download \a transaction.
		static constexpr uint64_t CalculateRealSize(const TransactionType& transaction) noexcept {
			return 	sizeof(TransactionType) + transaction.FileCount * sizeof(DownloadAction);
		}
	};

	DEFINE_EMBEDDABLE_TRANSACTION(StartFileDownload)

#pragma pack(pop)
}}
