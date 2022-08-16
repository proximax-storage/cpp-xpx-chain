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
#include "BaseSet.h"

namespace catapult { namespace deltaset {

	/// A view that provides iteration support to a base set.
	template<typename TSetTraits, typename TSetType>
	class BaseSetIterationView {
	private:
		using KeyType = typename TSetTraits::KeyType;

	public:
		/// Creates a view around \a set.
		explicit BaseSetIterationView(const TSetType& set) : m_set(set)
		{}

	public:
		/// Returns an iterator that points to the element with \a key if it is contained in this set, or end() otherwise.
		auto findIterator(const KeyType& key) const {
			return m_set.find(key);
		}

	public:
		/// Returns a const iterator to the first element of the underlying set.
		auto begin() const {
			return m_set.cbegin();
		}

		/// Returns a const iterator to the element following the last element of the underlying set.
		auto end() const {
			return m_set.cend();
		}

	private:
		const TSetType& m_set;
	};

	/// Returns \c true if \a set is iterable.
	template<typename TSet>
	bool IsSetIterable(const TSet&) {
		return true;
	}
	/// Returns \c true if \a set is iterable.
	template<typename TSet>
	bool IsSetBroadIterable(const TSet&) {
		return false;
	}

	/// Selects the iterable set from \a set.
	template<typename TSet>
	const TSet& SelectIterableSet(const TSet& set) {
		return set;
	}

	/// Selects the broad iterable set from \a set.
	/// Linker error if attempting to select a non supported set type.
	template<typename TSet>
	const TSet& SelectBroadIterableSet(const TSet& set);


	/// Returns \c true if \a set is iterable.
	template<typename TElementTraits, typename TSetTraits, typename TCommitPolicy>
	bool IsBaseSetIterable(const BaseSet<TElementTraits, TSetTraits, TCommitPolicy>& set) {
		return IsSetIterable(set.m_elements);
	}

	/// Returns \c true if \a set is iterable.
	template<typename TElementTraits, typename TSetTraits, typename TCommitPolicy>
	bool IsBaseSetBroadIterable(const BaseSet<TElementTraits, TSetTraits, TCommitPolicy>& set) {
		return IsSetBroadIterable(set.m_elements);
	}

	/// Makes a base memory based \a set iterable.
	/// \note This should only be supported for in memory views.
	template<typename TElementTraits, typename TSetTraits, typename TCommitPolicy>
	BaseSetIterationView<TSetTraits> MakeIterableView(const BaseSet<TElementTraits, TSetTraits, TCommitPolicy>& set) {
		return BaseSetIterationView<TSetTraits>(SelectIterableSet(set.m_elements));
	}


	/// Makes a base conditional container \a set iterable.
	/// \note This only currently supports conditional containers.
	template<typename TElementTraits, typename TSetTraits, typename TCommitPolicy>
	auto MakeBroadIterableView(const BaseSet<TElementTraits, TSetTraits, TCommitPolicy>& set) -> BaseSetIterationView<TSetTraits, decltype(SelectBroadIterableSet(set.m_elements))>{
		return BaseSetIterationView<TSetTraits, decltype(SelectBroadIterableSet(set.m_elements))>(SelectBroadIterableSet(set.m_elements));
	}

}}
