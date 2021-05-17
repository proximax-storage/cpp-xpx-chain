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
		/// Key of drive.
		Key DriveKey;

		/// Prepaid Download Size.
		uint64_t DownloadSize;

		/// XPXs to lock for future payment for.
		Amount TransactionFee;

	public:
		// Calculates the real size of a storage \a transaction.
		static constexpr uint64_t CalculateRealSize(const TransactionType&) noexcept {
			return sizeof(TransactionType);
		}
	};

	DEFINE_EMBEDDABLE_TRANSACTION(Download)

#pragma pack(pop)
}}
