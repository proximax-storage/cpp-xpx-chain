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

	/// Binary layout for a finish download transaction body.
	template<typename THeader>
	struct FinishDownloadTransactionBody : public THeader {
	private:
		using TransactionType = FinishDownloadTransactionBody<THeader>;

	public:
		explicit FinishDownloadTransactionBody<THeader>()
		{}

	public:
		DEFINE_TRANSACTION_CONSTANTS(Entity_Type_FinishDownload, 1)

	public:
		/// The identifier of the download channel.
		Hash256 DownloadChannelId;

		/// Amount of XPXs to transfer to the download channel.
		Amount FeedbackFeeAmount;

	public:
		// Calculates the real size of a storage \a transaction.
		static constexpr uint64_t CalculateRealSize(const TransactionType& transaction) noexcept {
			return sizeof(TransactionType);
		}
	};

	DEFINE_EMBEDDABLE_TRANSACTION(FinishDownload)

#pragma pack(pop)
}}
