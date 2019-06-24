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
#include "BasicTransactionsCache.h"
#include "catapult/functions.h"
#include <vector>

namespace catapult { namespace cache {

	/// An interface for modifying a partial transactions cache.
	/// \note Cache assumes that added transactions are stripped of all cosignatures.
	class PtCacheModifier : public BasicTransactionsCacheModifier<model::DetachedTransactionInfo> {
	public:
		using BasicTransactionsCacheModifier<model::DetachedTransactionInfo>::add;

	public:
		/// Adds a cosignature (composed of \a signer and \a signature) for a partial transaction with hash \a parentHash to the cache.
		virtual model::DetachedTransactionInfo add(const Hash256& parentHash, const Key& signer, const Signature& signature) = 0;

		/// Removes all partial transactions that have deadlines at or before the given \a timestamp.
		virtual std::vector<model::DetachedTransactionInfo> prune(Timestamp timestamp) = 0;

		/// Removes all partial transactions for which \a hashPredicate returns \c true.
		virtual std::vector<model::DetachedTransactionInfo> prune(const predicate<const Hash256&>& hashPredicate) = 0;
	};

	/// A delegating proxy around a PtCacheModifier.
	/// \note This is returned by value by PtCache::modifier in order to allow it to be consistent with other modifier functions.
	class PtCacheModifierProxy final : public BasicTransactionsCacheModifierProxy<model::DetachedTransactionInfo, PtCacheModifier> {
	public:
		using BaseType = BasicTransactionsCacheModifierProxy<model::DetachedTransactionInfo, PtCacheModifier>;
		using BaseType::BasicTransactionsCacheModifierProxy;
		using BaseType::add;

	public:
		/// Adds a cosignature (composed of \a signer and \a signature) for a partial transaction with hash \a parentHash to the cache.
		model::DetachedTransactionInfo add(const Hash256& parentHash, const Key& signer, const Signature& signature) {
			return modifier().add(parentHash, signer, signature);
		}

		/// Removes all partial transactions that have deadlines at or before the given \a timestamp.
		std::vector<model::DetachedTransactionInfo> prune(Timestamp timestamp) {
			return modifier().prune(timestamp);
		}

		/// Removes all partial transactions for which \a hashPredicate returns \c true.
		std::vector<model::DetachedTransactionInfo> prune(const predicate<const Hash256&>& hashPredicate) {
			return modifier().prune(hashPredicate);
		}
	};

	/// An interface for caching partial transactions.
	class PtCache : public BasicTransactionsCache<PtCacheModifierProxy> {};
}}
