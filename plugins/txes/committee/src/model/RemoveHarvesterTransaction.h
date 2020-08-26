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

	/// Binary layout for a remove harvester transaction body.
	template<typename THeader>
	struct RemoveHarvesterTransactionBody : public THeader {
	private:
		using TransactionType = RemoveHarvesterTransactionBody<THeader>;

	public:
		DEFINE_TRANSACTION_CONSTANTS(Entity_Type_RemoveHarvester, 1)

	public:
		// Calculates the real size of a remove harvester transaction.
		static constexpr uint64_t CalculateRealSize(const TransactionType&) noexcept {
			return sizeof(TransactionType);
		}
	};

	DEFINE_EMBEDDABLE_TRANSACTION(RemoveHarvester)

#pragma pack(pop)
}}
