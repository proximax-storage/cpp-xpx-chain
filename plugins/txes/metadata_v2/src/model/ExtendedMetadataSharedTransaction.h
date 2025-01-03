/**
*** Copyright 2024 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/model/Transaction.h"

namespace catapult { namespace model {

#pragma pack(push, 1)

	/// Metadata transaction header.
	template<typename THeader>
	struct ExtendedMetadataTransactionHeader : public THeader {
		/// Metadata target address.
		Key TargetKey;

		/// Metadata key scoped to source, target and type.
		uint64_t ScopedMetadataKey;
	};

	/// Binary layout for a basic extended metadata transaction body.
	template<typename THeader, EntityType Extended_Metadata_Entity_Type>
	struct BasicExtendedMetadataTransactionBody : public THeader {
	private:
		using TransactionType = BasicExtendedMetadataTransactionBody<THeader, Extended_Metadata_Entity_Type>;

	public:
		DEFINE_TRANSACTION_CONSTANTS(Extended_Metadata_Entity_Type, 1)

	public:
		/// State of immutability.
		bool IsValueImmutable;

		/// Change in value size in bytes.
		int16_t ValueSizeDelta;

		/// Value size in bytes.
		uint16_t ValueSize;

		// followed by value data if ValueSize != 0
		DEFINE_TRANSACTION_VARIABLE_DATA_ACCESSORS(Value, uint8_t)

	private:
		template<typename T>
		static auto* ValuePtrT(T& transaction) {
			return transaction.ValueSize ? THeader::PayloadStart(transaction) : nullptr;
		}

	public:
		/// Calculates the real size of metadata \a transaction.
		static constexpr uint64_t CalculateRealSize(const TransactionType& transaction) noexcept {
			return sizeof(TransactionType) + transaction.ValueSize;
		}
	};

#pragma pack(pop)
}}
