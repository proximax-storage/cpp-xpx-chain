/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "LockFundCacheTypes.h"
#include "catapult/deltaset/BaseSetDelta.h"
#include <numeric>

namespace catapult { namespace cache {
	/// A mixin for looking up records.
	/// \note Due to double lookup, this cannot be replaced with one ConstAccessorMixin per set.
	template<typename TPrimarySet, typename TSecondarySet>
	class LockFundMutableLookupMixin {
	public:
		/// An iterator that is returned by namespace cache find functions.
		using PrimaryKeyType = typename LockFundCacheDescriptor::KeyType;
		using PrimaryValueAdapter = detail::NoOpAdapter<typename LockFundCacheDescriptor::ValueType>;
		using PrimaryValueType = typename PrimaryValueAdapter::AdaptedValueType;
		using PrimarySetIteratorType = typename TPrimarySet::FindConstIterator;

		using InverseKeyType = typename LockFundCacheTypes::KeyedLockFundTypesDescriptor::KeyType;
		using InverseValueAdapter = detail::NoOpAdapter<typename LockFundCacheTypes::KeyedLockFundTypesDescriptor::ValueType>;
		using InverseValueType = typename InverseValueAdapter::AdaptedValueType;
		using InverseSetIteratorType = typename TSecondarySet::FindConstIterator;

		using primary_iterator = detail::CacheFindIterator<LockFundCacheDescriptor, PrimaryValueAdapter, typename TPrimarySet::FindIterator, PrimaryValueType>;
		using inverse_iterator = detail::CacheFindIterator<LockFundCacheTypes::KeyedLockFundTypesDescriptor, InverseValueAdapter, typename TSecondarySet::FindIterator, InverseValueType>;

	public:
		/// Creates a mixin around (history by id) \a set and \a flatMap.
		explicit LockFundMutableLookupMixin(TPrimarySet& set, TSecondarySet& inverseSet)
				: m_set(set)
				, m_inverseSet(inverseSet)
		{}

	public:

		/// Finds the cache value identified by \a height.
		primary_iterator find(Height height) {
			auto setIter = m_set.find(height);
			return primary_iterator(std::move(setIter), height);
		}

		/// Finds the cache value identified by \a key.
		inverse_iterator find(Key key) {
			auto setIter = m_inverseSet.find(key);
			return inverse_iterator(std::move(setIter), key);
		}

	private:
		TPrimarySet& m_set;
		TSecondarySet& m_inverseSet;
	};

	/// A mixin for looking up records.
	/// \note Due to double lookup, this cannot be replaced with one ConstAccessorMixin per set.
	template<typename TPrimarySet, typename TSecondarySet>
	class LockFundConstLookupMixin {
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
		explicit LockFundConstLookupMixin(const TPrimarySet& set, const TSecondarySet& inverseSet)
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

	/// A mixin for getting the set size compatible with both sets.
	template<typename TPrimarySet, typename TSecondarySet>
	class LockFundSizeMixin {
		public:
			/// Creates a mixin around \a sets.
			explicit LockFundSizeMixin(const TPrimarySet& set, const TSecondarySet& inverseSet) : m_set(set), m_secondarySet(inverseSet)
			{}

		public:
			/// Gets the number of elements in the primary set.
			size_t size() const {
				return m_set.size();
			}

			/// Gets the number of elements in the secondary set.
			size_t secondarySize() const {
				return m_secondarySet.size();
			}

		private:
			const TPrimarySet& m_set;
			const TSecondarySet& m_secondarySet;
	};
}}
