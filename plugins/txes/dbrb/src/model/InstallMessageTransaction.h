/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "DbrbEntityType.h"
#include "extensions/fastfinality/src/dbrb/DbrbUtils.h"
#include "catapult/model/Transaction.h"

namespace catapult { namespace model {

#pragma pack(push, 1)

	/// Binary layout for an Install message transaction body.
	template<typename THeader>
	struct InstallMessageTransactionBody : public THeader {
	private:
		using TransactionType = InstallMessageTransactionBody<THeader>;
		
	public:
		explicit InstallMessageTransactionBody<THeader>()
		{}

	public:
		DEFINE_TRANSACTION_CONSTANTS(Entity_Type_InstallMessage, 1)

	public:
		/// Hash of the Install message.
		Hash256 MessageHash;

		/// Number of views that compose a sequence.
		uint32_t ViewsCount;

		/// Number of membership change pairs in the most recent (longest) view.
		uint32_t MostRecentViewSize;

		/// Number of pairs of process IDs and their signatures.
		uint32_t SignaturesCount;

		/// Numbers of membership change pairs in corresponding views.
		/// Has the length of ViewsCount.
		// TODO: Can be reduced to (ViewsCount - 1), since the last size must always be equal to MostRecentViewSize
		DEFINE_TRANSACTION_VARIABLE_DATA_ACCESSORS(ViewSizes, uint16_t)

		/// Keys of the processes mentioned in the most recent (longest) view.
		/// Has the length of MostRecentViewSize.
		DEFINE_TRANSACTION_VARIABLE_DATA_ACCESSORS(ViewProcessIds, Key)

		/// Changes of process membership in the system. True = joined, false = left.
		/// Has the length of MostRecentViewSize.
		DEFINE_TRANSACTION_VARIABLE_DATA_ACCESSORS(MembershipChanges, bool)

		/// Keys of the processes mentioned in the signatures map.
		/// Has the length of SignaturesCount.
		DEFINE_TRANSACTION_VARIABLE_DATA_ACCESSORS(SignaturesProcessIds, Key)

		/// Signatures of processes.
		/// Has the length of SignaturesCount.
		DEFINE_TRANSACTION_VARIABLE_DATA_ACCESSORS(Signatures, Signature)

	private:
		template<typename T>
		static auto* ViewSizesPtrT(T& transaction) {
			auto* pPayloadStart = THeader::PayloadStart(transaction);
			return transaction.ViewsCount ? pPayloadStart : nullptr;
		}

		template<typename T>
		static auto* ViewProcessIdsPtrT(T& transaction) {
			auto* pPayloadStart = THeader::PayloadStart(transaction);
			return transaction.MostRecentViewSize && pPayloadStart ? pPayloadStart
					+ transaction.ViewsCount * sizeof(uint16_t) : nullptr;
		}

		template<typename T>
		static auto* MembershipChangesPtrT(T& transaction) {
			auto* pPayloadStart = THeader::PayloadStart(transaction);
			return transaction.MostRecentViewSize && pPayloadStart ? pPayloadStart
					+ transaction.ViewsCount * sizeof(uint16_t)
					+ transaction.MostRecentViewSize * sizeof(Key) : nullptr;
		}

		template<typename T>
		static auto* SignaturesProcessIdsPtrT(T& transaction) {
			auto* pPayloadStart = THeader::PayloadStart(transaction);
			return transaction.SignaturesCount && pPayloadStart ? pPayloadStart
					+ transaction.ViewsCount * sizeof(uint16_t)
					+ transaction.MostRecentViewSize * sizeof(Key)
				  	+ transaction.MostRecentViewSize * sizeof(bool) : nullptr;
		}

		template<typename T>
		static auto* SignaturesPtrT(T& transaction) {
			auto* pPayloadStart = THeader::PayloadStart(transaction);
			return transaction.SignaturesCount && pPayloadStart ? pPayloadStart
				  	+ transaction.ViewsCount * sizeof(uint16_t)
				  	+ transaction.MostRecentViewSize * sizeof(Key)
				  	+ transaction.MostRecentViewSize * sizeof(bool)
					+ transaction.SignaturesCount * sizeof(Key) : nullptr;
		}

	public:
		// Calculates the real size of an install \a transaction.
		static constexpr uint64_t CalculateRealSize(const TransactionType& transaction) noexcept {
			return sizeof(TransactionType)
				   + transaction.ViewsCount * sizeof(uint16_t)
				   + transaction.MostRecentViewSize * sizeof(Key)
				   + transaction.MostRecentViewSize * sizeof(bool)
				   + transaction.SignaturesCount * sizeof(Key)
				   + transaction.SignaturesCount * sizeof(Signature);
		}
	};

	DEFINE_EMBEDDABLE_TRANSACTION(InstallMessage)

#pragma pack(pop)
}}
