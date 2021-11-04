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
namespace catapult { namespace model {

#pragma pack(push, 1)

		/// Binary layout for a account upgrade transaction meant to upgrade a V1 account to a V2 account, resulting in all asset transfer from one to the other.
		template<typename THeader>
		struct AccountV2UpgradeTransaction : public THeader {
		private:
			using TransactionType = AccountV2UpgradeTransaction<THeader>;

		public:
			DEFINE_TRANSACTION_CONSTANTS(Entity_Type_AccountV2_Upgrade, 1)

		public:
			/// New Account public key.
			Key NewAccountPublicKey;

		public:
			/// Calculates the real size of key link \a transaction.
			static constexpr uint64_t CalculateRealSize(const TransactionType&) noexcept {
				return sizeof(TransactionType);
			}
		};

#pragma pack(pop)
	}}
