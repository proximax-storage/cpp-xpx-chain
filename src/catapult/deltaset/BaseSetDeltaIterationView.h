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
#include "BaseSetDelta.h"
#include "BaseSetIterationView.h"

namespace catapult { namespace deltaset {

	/// A view that provides iteration support to a base set delta.
	/// \note This is only supported for set types where SetType and MemorySetType are the same.
	template<typename TSetTraits>
	class BaseSetDeltaIterationView {
	private:
		using SetType = typename TSetTraits::MemorySetType;
		using KeyType = typename TSetTraits::KeyType;

	public:
		/// Creates a view around \a originalElements, \a deltas and \a size.
		BaseSetDeltaIterationView(const SetType& originalElements, const DeltaElements<SetType>& deltas, size_t size)
				: m_originalElements(originalElements)
				, m_deltas(deltas)
				, m_size(size)
		{}

	public:
		/// Iterator used for iterating over the view.
		class iterator {
		public:
			using difference_type = std::ptrdiff_t;
			using value_type = const typename TSetTraits::StorageType;
			using pointer = value_type*;
			using reference = value_type&;
			using iterator_category = std::forward_iterator_tag;

		public:
			/// Creates an iterator around the original \a elements and \a deltas at \a position given a total of \a size elements.
			iterator(const SetType& elements, const DeltaElements<SetType>& deltas, size_t position, size_t size)
					: m_elements(elements)
					, m_deltas(deltas)
					, m_position(position)
					, m_size(size)
					, m_stage(IterationStage::Copied)
					, m_iter(m_deltas.Copied.cbegin()) {
				if (m_position == m_size)
					m_iter = m_deltas.Added.cend();
				else
					moveToValidElement();
			}

		public:
			/// Returns \c true if this iterator and \a rhs are equal.
			bool operator==(const iterator& rhs) const {
				// don't need to check m_deltas because m_elements is a private member of BaseSetDelta and only one BaseSetDelta
				// is allowed outstanding at once, so it will never be different when m_elements is the same
				return &m_elements == &rhs.m_elements && m_position == rhs.m_position;
			}

			/// Returns \c true if this iterator and \a rhs are not equal.
			bool operator!=(const iterator& rhs) const {
				return !(*this == rhs);
			}

		public:
			/// Advances the iterator to the next position.
			iterator& operator++() {
				if (m_position == m_size)
					CATAPULT_THROW_OUT_OF_RANGE("cannot advance iterator beyond end");

				++m_position;
				++m_iter;
				moveToValidElement();
				return *this;
			}

			/// Advances the iterator to the next position.
			iterator operator++(int) {
				auto copy = *this;
				++*this;
				return copy;
			}

		private:
			void moveToValidElement() {
				if (IterationStage::Copied == m_stage) {
					if (handleCopiedStage())
						return;

					// all copied elements have been iterated, so advance to the next stage
					m_stage = IterationStage::Original;
					m_iter = m_elements.cbegin();
				}

				if (IterationStage::Original == m_stage) {
					if (handleOriginalStage())
						return;

					// all original elements have been iterated, so advance to the next stage
					m_stage = IterationStage::Added;
					m_iter = m_deltas.Added.cbegin();
				}

				// last stage
				handleAddedStage();
			}

			bool handleCopiedStage() {
				return m_deltas.Copied.cend() != m_iter;
			}

			bool handleOriginalStage() {
				// advance to the next original element that has neither been removed nor copied
				for (; m_elements.cend() != m_iter; ++m_iter) {
					const auto& key = TSetTraits::ToKey(*m_iter);
					if (!Contains(m_deltas.Removed, key) && !Contains(m_deltas.Copied, key))
						return true;
				}

				return false;
			}

			bool handleAddedStage() {
				// advance to the next added element that has not been removed
				for (; m_deltas.Added.cend() != m_iter; ++m_iter) {
					const auto& key = TSetTraits::ToKey(*m_iter);
					if (!Contains(m_deltas.Removed, key))
						return true;
				}

				return false;
			}

		private:
			static constexpr bool Contains(const SetType& set, const KeyType& key) {
				return set.cend() != set.find(key);
			}

		public:
			/// Returns a pointer to the current element.
			value_type* operator->() const {
				if (m_position == m_size)
					CATAPULT_THROW_OUT_OF_RANGE("cannot dereference at end");

				return &*m_iter;
			}

			/// Returns a reference to the current element.
			value_type& operator*() const {
				return *(this->operator->());
			}

		private:
			enum class IterationStage { Copied, Original, Added };

		private:
			const SetType& m_elements;
			DeltaElements<SetType> m_deltas;
			size_t m_position;
			size_t m_size;
			IterationStage m_stage;
			typename SetType::const_iterator m_iter;
		};

		/// Returns a const iterator to the first element of the underlying set.
		auto begin() const {
			return iterator(m_originalElements, m_deltas, 0, m_size);
		}

		/// Returns a const iterator to the element following the last element of the underlying set.
		auto end() const {
			return iterator(m_originalElements, m_deltas, m_size, m_size);
		}

	private:
		const SetType& m_originalElements;
		DeltaElements<SetType> m_deltas; // by value because deltas holds all sets by reference
		size_t m_size;
	};

	/// Makes a base set \a delta iterable.
	/// \note This should only be supported for in memory views.
	template<typename TElementTraits, typename TSetTraits>
	BaseSetDeltaIterationView<TSetTraits> MakeIterableView(const BaseSetDelta<TElementTraits, TSetTraits>& delta) {
		return BaseSetDeltaIterationView<TSetTraits>(SelectIterableSet(delta.m_originalElements), delta.deltas(), delta.size());
	}
}}
