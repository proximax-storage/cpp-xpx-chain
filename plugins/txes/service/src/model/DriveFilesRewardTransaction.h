/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "ServiceEntityType.h"
#include "ServiceTypes.h"
#include "catapult/model/Transaction.h"
#include "catapult/model/TransactionContainer.h"

namespace catapult { namespace model {

#pragma pack(push, 1)

	/// Binary layout for an drive files reward transaction header.
	template<typename THeader>
	struct DriveFilesRewardTransactionBody : public THeader {
	private:
		using TransactionType = DriveFilesRewardTransactionBody<THeader>;

	public:
		DEFINE_TRANSACTION_CONSTANTS(Entity_Type_DriveFilesReward, 1)

		/// Count of Upload Info.
		uint16_t UploadInfosCount;

		/// Upload infos for drive.
		DEFINE_TRANSACTION_VARIABLE_DATA_ACCESSORS(UploadInfos, UploadInfo)

	private:
		template<typename T>
		static auto* UploadInfosPtrT(T& transaction) {
			auto* pPayloadStart = THeader::PayloadStart(transaction);
			return transaction.UploadInfosCount ? pPayloadStart : nullptr;
		}

	public:
		// Calculates the real size of a service \a transaction.
		static constexpr uint64_t CalculateRealSize(const TransactionType& transaction) noexcept {
			return sizeof(TransactionType) + transaction.UploadInfosCount * sizeof(UploadInfo);
		}
	};

	DEFINE_EMBEDDABLE_TRANSACTION(DriveFilesReward)

#pragma pack(pop)
}}
