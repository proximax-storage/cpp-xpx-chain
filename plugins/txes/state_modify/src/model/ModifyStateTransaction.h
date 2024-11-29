/**
*** Copyright (c) 2016-present,
*** Jaguar0625, gimre, BloodyRookie, Tech Bureau, Corp. All rights reserved.
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
#include "ModifyStateEntityType.h"
#include "catapult/model/Transaction.h"
#include "catapult/model/TransactionContainer.h"

namespace catapult { namespace model {

#pragma pack(push, 1)


	template<typename THeader>
	struct ModifyStateTransactionBody : public THeader {
	private:
		using TransactionType = ModifyStateTransactionBody<THeader>;

	public:
		explicit ModifyStateTransactionBody<THeader>()
		{}

	public:
		DEFINE_TRANSACTION_CONSTANTS(Entity_Type_ModifyState, 1)

	public:
		/// Size of the cache name.
		uint16_t CacheNameSize;
		///Id of the subcache
		uint8_t SubCacheId;
		/// Size of the key.
		uint32_t KeySize;
		/// Size of the entry.
		uint32_t ContentSize;

		// followed by cache name
		DEFINE_TRANSACTION_VARIABLE_DATA_ACCESSORS(CacheName, uint8_t)

		// followed by key
		DEFINE_TRANSACTION_VARIABLE_DATA_ACCESSORS(Key, uint8_t)

		// followed by data
		DEFINE_TRANSACTION_VARIABLE_DATA_ACCESSORS(Content, uint8_t)

	private:
		template<typename T>
		static auto* CacheNamePtrT(T& transaction) {
			return transaction.CacheNameSize ? THeader::PayloadStart(transaction) : nullptr;
		}

		template<typename T>
		static auto* KeyPtrT(T& transaction) {
			auto* pPayloadStart = THeader::PayloadStart(transaction);
			return transaction.KeySize && pPayloadStart ?
				pPayloadStart + transaction.CacheNameSize : nullptr;
		}

		template<typename T>
		static auto* ContentPtrT(T& transaction) {
			auto* pPayloadStart = THeader::PayloadStart(transaction);
			return transaction.ContentSize && pPayloadStart ?
														pPayloadStart + transaction.CacheNameSize + transaction.KeySize : nullptr;
		}

	public:
		// Calculates the real size of a data modification \a transaction.
		static constexpr uint64_t CalculateRealSize(const TransactionType& transaction) noexcept {
			return sizeof(TransactionType) + transaction.CacheNameSize + transaction.KeySize + transaction.ContentSize;
		}
	};

	DEFINE_EMBEDDABLE_TRANSACTION(ModifyState)


#pragma pack(pop)
}}
