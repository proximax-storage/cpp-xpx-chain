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

	/// Binary layout for a download approval transaction body.
	template<typename THeader>
	struct DownloadApprovalTransactionBody : public THeader {
	private:
		using TransactionType = DownloadApprovalTransactionBody<THeader>;

	public:
		explicit DownloadApprovalTransactionBody<THeader>()
		{}

	public:
		DEFINE_TRANSACTION_CONSTANTS(Entity_Type_DownloadApproval, 1)

	public:
		/// The identifier of the download channel.
		Hash256 DownloadChannelId;

		/// Reason of the transaction release.
		bool ResponseToFinishDownloadTransaction;

		/// Opinion about the replicator's upload volume to the consumer.
		uint64_t ReplicatorUploadOpinion;

	public:
		// Calculates the real size of a storage \a transaction.
		static constexpr uint64_t CalculateRealSize(const TransactionType&) noexcept {
			return sizeof(TransactionType);
		}
	};

	DEFINE_EMBEDDABLE_TRANSACTION(DownloadApproval)

#pragma pack(pop)
}}
