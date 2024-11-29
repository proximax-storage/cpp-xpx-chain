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
#include "TransactionBuilder.h"
#include "plugins/txes/state_modify/src/model/ModifyStateTransaction.h"
#include "catapult/cache/CacheConstants.h"
#include <vector>

namespace catapult { namespace builders {

	/// Builder for a transfer transaction.
	class ModifyStateBuilder : public TransactionBuilder {
	public:
		using Transaction = model::ModifyStateTransaction;
		using EmbeddedTransaction = model::EmbeddedModifyStateTransaction;

	public:
		/// Creates a transfer builder for building a transfer transaction from \a signer
		/// for the network specified by \a networkIdentifier.
		ModifyStateBuilder(model::NetworkIdentifier networkIdentifier, const Key& signer);

	public:
		/// Sets data.
		void setData(const RawBuffer& data);

		/// Sets data.
		void setKey(const std::string& key);

		/// Sets the transaction message to \a message.
		void setCacheId(cache::CacheId cacheId, cache::SubCacheId subCacheId);

	public:
		/// Builds a new transfer transaction.
		model::UniqueEntityPtr<Transaction> build() const;

		/// Builds a new embedded transfer transaction.
		model::UniqueEntityPtr<EmbeddedTransaction> buildEmbedded() const;

	private:
		template<typename TTransaction>
		model::UniqueEntityPtr<TTransaction> buildImpl() const;

	private:
		std::vector<uint8_t> m_data;
		std::vector<uint8_t> m_key;
		cache::CacheId m_cacheId;
		cache::SubCacheId m_subCacheId;
	};
}}
