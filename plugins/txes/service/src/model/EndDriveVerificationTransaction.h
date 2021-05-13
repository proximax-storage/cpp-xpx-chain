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

	/// Binary layout for an aggregate transaction header.
	template<typename THeader>
	struct EndDriveVerificationTransactionHeader : public THeader {
	public:
		DEFINE_TRANSACTION_CONSTANTS(Entity_Type_End_Drive_Verification, 1)

	public:
		size_t GetHeaderSize() const {
			return sizeof(EndDriveVerificationTransactionHeader<THeader>);
		}
	};

	/// Binary layout for an end drive verification transaction body.
	template<typename THeader>
	struct EndDriveVerificationTransactionBody : public TransactionContainer<EndDriveVerificationTransactionHeader<THeader>, VerificationFailure> {
	private:
		using TransactionType = EndDriveVerificationTransactionBody<THeader>;

	public:
		// Calculates the real size of end drive verification \a transaction.
		static constexpr uint64_t CalculateRealSize(const TransactionType& transaction) noexcept {
			return IsSizeValid(transaction) ? transaction.Size : std::numeric_limits<uint64_t>::max();
		}

	private:
		static bool IsSizeValid(const TransactionType& transaction) {
			if (transaction.Size < sizeof(TransactionType)) {
				CATAPULT_LOG(warning) << "end drive verification transaction failed size validation (transaction size " << transaction.Size
									  << " less than header size " << sizeof(TransactionType) << ")";
				return false;
			}

			auto failures = transaction.Transactions(EntityContainerErrorPolicy::Suppress);
			auto areAllSizesValid = std::all_of(failures.cbegin(), failures.cend(), [](const auto& failure) {
				return failure.IsSizeValid();
			});

			if (areAllSizesValid && !failures.hasError())
				return true;

			CATAPULT_LOG(warning) << "end drive verification transaction failed size validation (valid sizes? " << areAllSizesValid
								  << ", errors? " << failures.hasError() << ")";
			return false;
		}
	};

	// Calculates the PayloadSize of end drive verification \a transaction.
	template<typename TransactionType>
	static constexpr size_t GetTransactionPayloadSize(const TransactionType& transaction) noexcept {
		return transaction.Size - sizeof(TransactionType);
	}

	DEFINE_EMBEDDABLE_TRANSACTION(EndDriveVerification)

#pragma pack(pop)
}}
