/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "MetadataEntityType.h"
#include "MetadataTypes.h"
#include "catapult/model/Transaction.h"
#include "catapult/model/TransactionContainer.h"

namespace catapult { namespace model {

#pragma pack(push, 1)

	template<typename THeader, typename TMetadataId>
	/// Binary layout for an metadata transaction header.
	struct MetadataTransactionHeader : public THeader {
	public:
		/// Metadata type.
		model::MetadataType MetadataType;

		/// Id of metadata(MosaicId, NamespaceId and etc).
		TMetadataId MetadataId;

	public:
		size_t GetHeaderSize() const {
			return sizeof(MetadataTransactionHeader);
		}
	};

	/// Binary layout for a basic metadata transaction body.
	template<typename THeader, typename TMetadataId>
	struct BasicMetadataTransactionBody : public TransactionContainer<MetadataTransactionHeader<THeader, TMetadataId>, MetadataModification> {
	private:
		using TransactionType = BasicMetadataTransactionBody<THeader, TMetadataId>;

	public:
		// Calculates the real size of metadata \a transaction.
		static constexpr uint64_t CalculateRealSize(const TransactionType& transaction) noexcept {
			return IsSizeValid(transaction) ? transaction.Size : std::numeric_limits<uint64_t>::max();
		}

	private:
		static bool IsSizeValid(const TransactionType& metadataTransaction) {
			auto transactions = metadataTransaction.Transactions(EntityContainerErrorPolicy::Suppress);
			auto areAllTransactionsValid = std::all_of(transactions.cbegin(), transactions.cend(), [](const auto& modification) {
				return modification.IsSizeValid();
			});

			if (areAllTransactionsValid && !transactions.hasError())
				return true;

			CATAPULT_LOG(warning) << "metadataT transactions failed size validation (valid sizes? " << areAllTransactionsValid
								  << ", errors? " << transactions.hasError() << ")";
			return false;
		}
	};

	template<typename TransactionType>
	// Calculates the PayloadSize of metadata \a transaction.
	static constexpr size_t GetTransactionPayloadSize(const TransactionType& transaction) noexcept {
		return transaction.Size - sizeof(TransactionType);
	}

#define DEFINE_METADATA_TRANSACTION(VALUE_NAME, ENTITY_TYPE_NAME, VALUE_TYPE) \
	template<typename THeader> \
	struct VALUE_NAME##MetadataTransactionBody : public BasicMetadataTransactionBody<THeader, VALUE_TYPE> { \
	public: \
		DEFINE_TRANSACTION_CONSTANTS(Entity_Type_##ENTITY_TYPE_NAME##_Metadata, 1) \
	}; \
	\
	DEFINE_EMBEDDABLE_TRANSACTION(VALUE_NAME##Metadata)

#pragma pack(pop)
}}
