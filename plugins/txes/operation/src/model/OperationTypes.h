/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "OperationEntityType.h"
#include "catapult/model/Mosaic.h"
#include "catapult/model/Transaction.h"

namespace catapult { namespace model {

#pragma pack(push, 1)

	using OperationResult = uint16_t;

	constexpr OperationResult Operation_Result_None = 0;

	/// Binary layout for a basic operation transaction body.
	template<typename THeader, typename TDerived>
	struct BasicOperationTransactionBody : public THeader {
	private:
		using TransactionType = BasicOperationTransactionBody<THeader, TDerived>;

	public:
		/// Number of mosaics.
		uint8_t MosaicCount;

		// Mosaics.
		DEFINE_TRANSACTION_VARIABLE_DATA_ACCESSORS(Mosaics, UnresolvedMosaic)

	private:
		template<typename T>
		static auto* MosaicsPtrT(T& transaction) {
			return transaction.MosaicCount ? THeader::PayloadStart(static_cast<TDerived&>(transaction)) : nullptr;
		}

		template<typename T>
		static auto* MosaicsPtrT(const T& transaction) {
			return transaction.MosaicCount ? THeader::PayloadStart(static_cast<const TDerived&>(transaction)) : nullptr;
		}

	public:
		// Calculates the real size of a basic operation transaction.
		static constexpr uint64_t CalculateRealSize(const TransactionType& transaction) noexcept {
			return sizeof(TransactionType) + transaction.MosaicCount * sizeof(UnresolvedMosaic);
		}
	};

	/// Binary layout for a basic start operation transaction body.
	template<typename THeader, typename TDerived>
	struct BasicStartOperationTransactionBody : public BasicOperationTransactionBody<THeader, TDerived> {
	private:
		using TransactionType = BasicStartOperationTransactionBody<THeader, TDerived>;
		using BaseTransactionType = BasicOperationTransactionBody<THeader, TDerived>;

	public:
		/// Operation duration.
		BlockDuration Duration;

	public:
		// Calculates the real size of a basic start operation transaction.
		static constexpr uint64_t CalculateRealSize(const TransactionType& transaction) noexcept {
			return BaseTransactionType::CalculateRealSize(transaction) + sizeof(BlockDuration);
		}
	};

#pragma pack(pop)
}}
