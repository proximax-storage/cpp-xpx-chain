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

	/// Binary layout for a download payment transaction body.
	template<typename THeader>
	struct DownloadPaymentTransactionBody : public THeader {
	private:
		using TransactionType = DownloadPaymentTransactionBody<THeader>;

	public:
		explicit DownloadPaymentTransactionBody<THeader>()
		{}

	public:
		DEFINE_TRANSACTION_CONSTANTS(Entity_Type_DownloadPayment, 1)

	public:
		/// The identifier of the download channel.
		Hash256 DownloadChannelId;

		/// Download size to add to the prepaid size of the download channel.
		uint64_t DownloadSizeMegabytes;

		/// Amount of XPXs to transfer to the download channel.
		Amount FeedbackFeeAmount;

	public:
		// Calculates the real size of a storage \a transaction.
		static constexpr uint64_t CalculateRealSize(const TransactionType& transaction) noexcept {
			return sizeof(TransactionType);
		}
	};

	DEFINE_EMBEDDABLE_TRANSACTION(DownloadPayment)

#pragma pack(pop)
}}
