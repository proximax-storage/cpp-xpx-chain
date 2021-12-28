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

		/// The hash block that initiated the rewards approval.
		Hash256 ApprovalTrigger;

		/// Sequence number of current download approval transaction in the download channel.
		uint16_t SequenceNumber;

		/// Reason of the transaction release.
		bool ResponseToFinishDownloadTransaction;

		/// Number of replicators that provided their opinions, but on which no opinions were provided.
		uint8_t JudgingKeysCount;

		/// Number of replicators that both provided their opinions, and on which at least one opinion was provided.
		uint8_t OverlappingKeysCount;

		/// Number of replicators that didn't provide their opinions, but on which at least one opinion was provided.
		uint8_t JudgedKeysCount;

		/// Total number of opinion elements.
		// TODO: Remove; instead process PresentOpinions to count up existing opinion elements
		uint8_t OpinionElementCount;

		/// Replicators' public keys.
		DEFINE_TRANSACTION_VARIABLE_DATA_ACCESSORS(PublicKeys, Key)

		/// Signatures of replicators' opinions.
		DEFINE_TRANSACTION_VARIABLE_DATA_ACCESSORS(Signatures, Signature)

		/// Two-dimensional array of opinion element presence.
		/// Must be interpreted bitwise (1 if corresponding element exists, 0 otherwise).
		DEFINE_TRANSACTION_VARIABLE_DATA_ACCESSORS(PresentOpinions, uint8_t)

		/// Jagged array of opinion elements.
		DEFINE_TRANSACTION_VARIABLE_DATA_ACCESSORS(Opinions, uint64_t)

	private:
		template<typename T>
		static auto* PublicKeysPtrT(T& transaction) {
			const auto totalKeysCount = transaction.JudgingKeysCount + transaction.OverlappingKeysCount + transaction.JudgedKeysCount;
			return totalKeysCount ? THeader::PayloadStart(transaction) : nullptr;
		}

		template<typename T>
		static auto* SignaturesPtrT(T& transaction) {
			auto* pPayloadStart = THeader::PayloadStart(transaction);
			const auto totalKeysCount = transaction.JudgingKeysCount + transaction.OverlappingKeysCount + transaction.JudgedKeysCount;
			const auto totalJudgingKeysCount = transaction.JudgingKeysCount + transaction.OverlappingKeysCount;
			return totalJudgingKeysCount && pPayloadStart ? pPayloadStart
					+ totalKeysCount * sizeof(Key) : nullptr;
		}

		template<typename T>
		static auto* PresentOpinionsPtrT(T& transaction) {
			auto* pPayloadStart = THeader::PayloadStart(transaction);
			const auto totalKeysCount = transaction.JudgingKeysCount + transaction.OverlappingKeysCount + transaction.JudgedKeysCount;
			const auto totalJudgingKeysCount = transaction.JudgingKeysCount + transaction.OverlappingKeysCount;
			const auto totalJudgedKeysCount = transaction.OverlappingKeysCount + transaction.JudgedKeysCount;
			return totalJudgingKeysCount && totalJudgedKeysCount && pPayloadStart ? pPayloadStart
					+ totalKeysCount * sizeof(Key)
					+ totalJudgingKeysCount * sizeof(Signature) : nullptr;
		}

		template<typename T>
		static auto* OpinionsPtrT(T& transaction) {
			auto* pPayloadStart = THeader::PayloadStart(transaction);
			const auto totalKeysCount = transaction.JudgingKeysCount + transaction.OverlappingKeysCount + transaction.JudgedKeysCount;
			const auto totalJudgingKeysCount = transaction.JudgingKeysCount + transaction.OverlappingKeysCount;
			const auto presentOpinionByteCount = (totalJudgingKeysCount * (transaction.OverlappingKeysCount + transaction.JudgedKeysCount) + 7) / 8;
			return transaction.OpinionElementCount && pPayloadStart ? pPayloadStart
					+ totalKeysCount * sizeof(Key)
				  	+ totalJudgingKeysCount * sizeof(Signature)
					+ presentOpinionByteCount * sizeof(uint8_t) : nullptr;
		}

	public:
		// Calculates the real size of a download approval \a transaction.
		static constexpr uint64_t CalculateRealSize(const TransactionType& transaction) noexcept {
			const auto totalKeysCount = transaction.JudgingKeysCount + transaction.OverlappingKeysCount + transaction.JudgedKeysCount;
			const auto totalJudgingKeysCount = transaction.JudgingKeysCount + transaction.OverlappingKeysCount;
			const auto presentOpinionByteCount = (totalJudgingKeysCount * (transaction.OverlappingKeysCount + transaction.JudgedKeysCount) + 7) / 8;
			return sizeof(TransactionType)
				   + totalKeysCount * sizeof(Key)
				   + totalJudgingKeysCount * sizeof(Signature)
				   + presentOpinionByteCount * sizeof(uint8_t)
				   + transaction.OpinionElementCount * sizeof(uint64_t);
		}
	};

	DEFINE_EMBEDDABLE_TRANSACTION(DownloadApproval)

#pragma pack(pop)
}}
