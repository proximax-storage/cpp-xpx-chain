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
#include "LockFundCacheTypes.h"
#include "catapult/deltaset/BaseSetDelta.h"
#include <numeric>

namespace catapult { namespace cache {
	/// A mixin for looking up namespaces.
	/// \note Due to double lookup, this cannot be replaced with one ConstAccessorMixin per set.
	template<typename TPrimarySet, typename TSecondarySet>
	class LockFundLookupMixin {
	public:
		/// An iterator that is returned by namespace cache find functions.
		using PrimaryKeyType = typename LockFundCacheDescriptor::KeyType;
		using PrimaryValueAdapter = detail::NoOpAdapter<const typename LockFundCacheDescriptor::ValueType>;
		using PrimaryValueType = typename PrimaryValueAdapter::AdaptedValueType;
		using PrimarySetIteratorType = typename TPrimarySet::FindConstIterator;

		using InverseKeyType = typename LockFundCacheTypes::KeyedLockFundTypesDescriptor::KeyType;
		using InverseValueAdapter = detail::NoOpAdapter<const typename LockFundCacheTypes::KeyedLockFundTypesDescriptor::ValueType>;
		using InverseValueType = typename InverseValueAdapter::AdaptedValueType;
		using InverseSetIteratorType = typename TSecondarySet::FindConstIterator;

		using const_primary_iterator = detail::CacheFindIterator<LockFundCacheDescriptor, PrimaryValueAdapter, typename TPrimarySet::FindConstIterator, const PrimaryValueType>;
		using const_inverse_iterator = detail::CacheFindIterator<LockFundCacheTypes::KeyedLockFundTypesDescriptor, InverseValueAdapter, typename TSecondarySet::FindConstIterator, const InverseValueType>;

	public:
		/// Creates a mixin around (history by id) \a set and \a flatMap.
		explicit LockFundLookupMixin(const TPrimarySet& set, const TSecondarySet& inverseSet)
				: m_set(set)
				, m_inverseSet(inverseSet)
		{}

	public:

		/// Finds the cache value identified by \a height.
		const_primary_iterator find(Height height) const {
			auto setIter = m_set.find(height);
			return const_primary_iterator(std::move(setIter), height);
		}

		/// Finds the cache value identified by \a key.
		const_inverse_iterator find(Key key) const {
			auto setIter = m_inverseSet.find(key);
			return const_inverse_iterator(std::move(setIter), key);
		}

	private:
		const TPrimarySet& m_set;
		const TSecondarySet& m_inverseSet;
	};
}}
