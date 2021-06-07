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

	/// Binary layout for a data modification transaction body.
	template<typename THeader>
	struct DataModificationTransactionBody : public THeader {
	private:
		using TransactionType = DataModificationTransactionBody<THeader>;

	public:
		explicit DataModificationTransactionBody<THeader>()
		{}

	public:
		DEFINE_TRANSACTION_CONSTANTS(Entity_Type_DataModification, 1)

	public:
		/// Key of drive.
		Key DriveKey;

		/// Download data CDI of modification.
		Hash256 DownloadDataCdi;

		/// Size of upload.
		uint64_t UploadSize;

	public:
		// Calculates the real size of a data modification \a transaction.
		static constexpr uint64_t CalculateRealSize(const TransactionType&) noexcept {
			return sizeof(TransactionType);
		}
	};

	DEFINE_EMBEDDABLE_TRANSACTION(DataModification)

#pragma pack(pop)
}}
