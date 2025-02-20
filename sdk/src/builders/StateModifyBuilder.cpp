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

#include "StateModifyBuilder.h"

namespace catapult { namespace builders {

	ModifyStateBuilder::ModifyStateBuilder(model::NetworkIdentifier networkIdentifier, const Key& signer)
			: TransactionBuilder(networkIdentifier, signer)
			, m_key()
			, m_data()
	{}

	model::UniqueEntityPtr<ModifyStateBuilder::Transaction> ModifyStateBuilder::build() const {
		return buildImpl<Transaction>();
	}

	model::UniqueEntityPtr<ModifyStateBuilder::EmbeddedTransaction> ModifyStateBuilder::buildEmbedded() const {
		return buildImpl<EmbeddedTransaction>();
	}

	template<typename TransactionType>
	model::UniqueEntityPtr<TransactionType> ModifyStateBuilder::buildImpl() const {
		// 1. allocate, zero (header), set model::Transaction fields
		auto cacheName = cache::GetCacheNameFromCacheId(m_cacheId);
		auto size = sizeof(TransactionType);
		size += cacheName.size();
		size += m_data.size();
		size += m_key.size();
		auto pTransaction = createTransaction<TransactionType>(size);

		// 2. set fixed transaction fields
		pTransaction->CacheNameSize = utils::checked_cast<size_t, uint8_t>(cacheName.size());
		pTransaction->SubCacheId = m_subCacheId;
		pTransaction->KeySize = utils::checked_cast<size_t, uint16_t>(m_key.size());
		pTransaction->ContentSize = utils::checked_cast<size_t, uint32_t>(m_data.size());

		// 3. set transaction attachments
		std::copy(cacheName.cbegin(), cacheName.cend(), pTransaction->CacheNamePtr());
		std::copy(m_key.cbegin(), m_key.cend(), pTransaction->KeyPtr());
		std::copy(m_data.cbegin(), m_data.cend(), pTransaction->ContentPtr());

		return pTransaction;
	}
	void ModifyStateBuilder::setData(const RawBuffer& data) {
		if (0 == data.Size)
			CATAPULT_THROW_INVALID_ARGUMENT("argument `message` cannot be empty");

		if (!m_data.empty())
			CATAPULT_THROW_RUNTIME_ERROR("`message` field already set");

		m_data.resize(data.Size);
		m_data.assign(data.pData, data.pData + data.Size);
	}
	void ModifyStateBuilder::setKey(const std::string& key) {

		if (0 == key.size())
			CATAPULT_THROW_INVALID_ARGUMENT("argument `message` cannot be empty");

		if (!m_key.empty())
			CATAPULT_THROW_RUNTIME_ERROR("`message` field already set");

		m_key.resize(key.size());
		m_key.assign(key.data(), key.data() + key.size());
	}
	void ModifyStateBuilder::setCacheId(cache::CacheId cacheId, cache::SubCacheId subCacheId) {
		m_subCacheId = subCacheId;
		m_cacheId = cacheId;
	}
}}
