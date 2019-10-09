/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "ServiceEntityType.h"
#include "ServiceTypes.h"
#include "catapult/model/Transaction.h"

namespace catapult { namespace model {

#pragma pack(push, 1)

	/// Binary layout for a drive file system transaction body.
	template<typename THeader>
	struct DriveFileSystemTransactionBody : public THeader {
	private:
		using TransactionType = DriveFileSystemTransactionBody<THeader>;

	public:
		DEFINE_TRANSACTION_CONSTANTS(Entity_Type_DriveFileSystem, 1)

	public:
		/// A new RootHash of drive.
		Hash256 RootHash;

		/// Xor of a new RootHash of drive with previous RootHash.
		Hash256 XorRootHash;

		/// Count of add actions.
		uint16_t AddActionsCount;

		/// Actions to add files to drive.
		DEFINE_TRANSACTION_VARIABLE_DATA_ACCESSORS(AddActions, AddAction)

		/// Count of remove actions.
		uint16_t RemoveActionsCount;

		/// Actions to remove files from drive.
		DEFINE_TRANSACTION_VARIABLE_DATA_ACCESSORS(RemoveActions, RemoveAction)

	private:
		template<typename T>
		static auto* AddActionsPtrT(T& transaction) {
			auto* pPayloadStart = THeader::PayloadStart(transaction);
			return pPayloadStart ? pPayloadStart : nullptr;
		}

		template<typename T>
		static auto* RemoveActionsPtrT(T& transaction) {
			auto* pAddActionsStart = AddActionsPtrT(transaction);
			return pAddActionsStart ? pAddActionsStart + transaction.AddActionsCount * sizeof(AddAction) : nullptr;
		}

	public:
		// Calculates the real size of a service \a transaction.
		static constexpr uint64_t CalculateRealSize(const TransactionType& transaction) noexcept {
			return 	sizeof(TransactionType)
					+ transaction.AddActionsCount * sizeof(AddAction)
					+ transaction.RemoveActionsCount * sizeof(RemoveAction);
		}
	};

	DEFINE_EMBEDDABLE_TRANSACTION(DriveFileSystem)

#pragma pack(pop)
}}
