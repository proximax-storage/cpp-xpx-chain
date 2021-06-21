/**
*** Copyright (c) 2016-2019, Jaguar0625, gimre, BloodyRookie, Tech Bureau, Corp.
*** Copyright (c) 2020-present, Jaguar0625, gimre, BloodyRookie.
*** All rights reserved.
***
*** This file is part of Catapult.
***
*** Catapult is free software: you can redistribute it and/or modify
*** it under the terms of the GNU Lesser General Public License as published by
*** the Free Software Foundation, either version 3 of the License, or
*** (at your option) any later version.
***
*** Catapult is distributed in the hope that it will be useful,
*** but WITHOUT ANY WARRANTY; without even the implied warranty of
*** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*** GNU Lesser General Public License for more details.
***
*** You should have received a copy of the GNU Lesser General Public License
*** along with Catapult. If not, see <http://www.gnu.org/licenses/>.
**/

#pragma once
#include <catapult/model/Transaction.h>
#include "AccountLinkEntityType.h"
#include "AccountLinkBaseTransaction.h"
namespace catapult { namespace model {

#pragma pack(push, 1)

		/// Binary layout for a key link transaction body.
		template<typename THeader, typename TAccountPublicKey, EntityType Key_Link_Entity_Type, VersionType VERSION>
		struct AccountLinkBaseTransaction : public THeader {
		private:
			using TransactionType = AccountLinkBaseTransaction<THeader, TAccountPublicKey, Key_Link_Entity_Type, VERSION>;

		public:
			DEFINE_TRANSACTION_CONSTANTS(Key_Link_Entity_Type, VERSION)

		public:
			/// Linked public key.
			TAccountPublicKey RemoteAccountKey;

			/// Link action.
			AccountLinkAction LinkAction;

		public:
			/// Calculates the real size of key link \a transaction.
			static constexpr uint64_t CalculateRealSize(const TransactionType&) noexcept {
				return sizeof(TransactionType);
			}
		};

#pragma pack(pop)
	}}
