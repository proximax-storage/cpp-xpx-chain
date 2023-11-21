/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "CommitteeEntityType.h"
#include "catapult/model/Transaction.h"

namespace catapult { namespace model {

#pragma pack(push, 1)

	/// Binary layout for an add harvester transaction body.
	template<typename THeader>
	struct AddHarvesterTransactionBody : public THeader {
	private:
		using TransactionType = AddHarvesterTransactionBody<THeader>;

	public:
		DEFINE_TRANSACTION_CONSTANTS(Entity_Type_AddHarvester, 1)

	public:
		/// Harvester public key.
		Key HarvesterKey;

	public:
		// Calculates the real size of an add harvester transaction.
		static constexpr uint64_t CalculateRealSize(const TransactionType&) noexcept {
			return sizeof(TransactionType);
		}
	};

	DEFINE_EMBEDDABLE_TRANSACTION(AddHarvester)

#pragma pack(pop)
}}
