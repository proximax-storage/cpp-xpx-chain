/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "CatapultUpgradeEntityType.h"
#include "catapult/model/Transaction.h"

namespace catapult { namespace model {

#pragma pack(push, 1)

		/// Binary layout for a catapult upgrade transaction body.
		template<typename THeader>
		struct CatapultUpgradeTransactionBody : public THeader {
		private:
			using TransactionType = CatapultUpgradeTransactionBody<THeader>;

		public:
			DEFINE_TRANSACTION_CONSTANTS(Entity_Type_Catapult_Upgrade, 1)

		public:
			/// Relative or absolute height at which the new catapult version will apply.
			uint64_t Height;

			/// Whether applying height is absolute or relative to the current height.
			bool IsHeightRelative;

			/// New version of catapult.
			uint64_t UpgradeVersion;

		public:
			// Calculates the real size of a catapult upgrade \a transaction.
			static constexpr uint64_t CalculateRealSize(const TransactionType& transaction) noexcept {
				(void)transaction;
				return sizeof(TransactionType);
			}
		};

		DEFINE_EMBEDDABLE_TRANSACTION(CatapultUpgrade)

#pragma pack(pop)
	}}
