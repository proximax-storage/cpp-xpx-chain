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

	/// Binary layout for a data modification approval transaction body.
	template<typename THeader>
	struct DataModificationApprovalTransactionBody : public THeader {
	private:
		using TransactionType = DataModificationApprovalTransactionBody<THeader>;
		
	public:
		explicit DataModificationApprovalTransactionBody<THeader>()
		{}

	public:
		DEFINE_TRANSACTION_CONSTANTS(Entity_Type_DataModificationApproval, 1)

	public:
		/// Key of drive.
		Key DriveKey;

		/// Key of replicator
		Key ReplicatorKey

		/// Identifier of the transaction that initiated the modification.
		Hash256 DataModificationId;

		/// Content Download Information for the File Structure.
		Hash256 FileStructureCdi;

		/// Size of the File Structure.
		uint64_t FileStructureSize;

		/// Total used disk space of the drive.
		uint64_t UsedDriveSize;

	public:
		// Calculates the real size of a data modification approval \a transaction.
		static constexpr uint64_t CalculateRealSize(const TransactionType&) noexcept {
			return sizeof(TransactionType);
		}
	};

	DEFINE_EMBEDDABLE_TRANSACTION(DataModificationApproval)

#pragma pack(pop)
}}
