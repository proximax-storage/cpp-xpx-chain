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
#include "BaseSetDefaultTraits.h"
#include "catapult/utils/traits/StlTraits.h"
#include <unordered_set>

namespace catapult { namespace deltaset {

	/// Mixin that wraps BaseSetDelta and provides a facade on top of BaseSetDelta::deltas().
	template<typename TSetDelta>
	class DeltaElementsMixin {
	private:
		// region value accessors

		template<typename TSet, bool IsMap = utils::traits::is_map_v<TSet>>
		struct ValueAccessorT {
			using ValueType = typename TSet::value_type;

			static const ValueType* GetPointer(const typename TSet::value_type& value) {
				return &value;
			}
		};

		template<typename TSet>
		struct ValueAccessorT<TSet, true> { // map specialization
			using ValueType = typename TSet::value_type::second_type;

			static const ValueType* GetPointer(const typename TSet::value_type& pair) {
				return &pair.second;
			}
		};

		// endregion

	private:
		// use MemorySetType for detection because it is always stl (memory) container
		using ValueAccessor = ValueAccessorT<typename TSetDelta::MemorySetType>;
		using ValueType = typename ValueAccessor::ValueType;
		using PointerContainer = std::unordered_set<const ValueType*>;

	public:
		/// Creates a mixin around \a setDelta.
		explicit DeltaElementsMixin(TSetDelta& setDelta) : m_setDelta(setDelta)
		{}

	public:
		/// Gets pointers to all added elements.
		PointerContainer addedElements() const {
			return CollectAllPointers(m_setDelta.deltas().Added);
		}

		/// Gets pointers to all modified elements.
		PointerContainer modifiedElements() const {
			return CollectAllPointers(m_setDelta.deltas().Copied);
		}

		/// Gets pointers to all removed elements.
		PointerContainer removedElements() const {
			return CollectAllPointers(m_setDelta.deltas().Removed);
		}

	public:
		/// Backs up the changes made in the cache delta. Set \c true to \a replace previous backup.
		void backupChanges(bool replace) {
			m_setDelta.backupChanges(replace);
		}

		/// Restores the last backed up changes in the cache delta.
		void restoreChanges() {
			m_setDelta.restoreChanges();
		}

	private:
		template<typename TSource>
		static PointerContainer CollectAllPointers(const TSource& source) {
			PointerContainer dest;
			for (const auto& value : source)
				dest.insert(ValueAccessor::GetPointer(value));

			return dest;
		}

	private:
		TSetDelta& m_setDelta;
	};
}}
