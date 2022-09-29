/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include <algorithm>

namespace catapult { namespace utils {

	/// Abstraction for a simplified manual mutation tracking mechanism.
	template<typename TMaxedClassEnum>
	class SimpleMutationTracker {
	public:
		SimpleMutationTracker() : m_data({})
		{}

	public:
		/// Returns \c true if there are any tracked changes, \c false otherwise.
		bool hasChanges() const {
			return std::any_of(m_data.begin(), m_data.end(), [](bool x) {return x;});
		}

		/// Returns \c true if there are any tracked changes, \c false otherwise.
		template<TMaxedClassEnum Value>
		bool hasChange() const {
			return m_data[static_cast<size_t>(Value)];
		}

		/// Marks the given value as changed
		template<TMaxedClassEnum Value>
		void mark() {
			m_data[static_cast<size_t>(Value)] = true;
		}

		/// Marks the given value as unchanged
		template<TMaxedClassEnum Value>
		void clear() {
			m_data[static_cast<size_t>(Value)] = false;
		}

		/// Marks all values as unchanged
		void clear() {
			m_data = {};
		}

	private:
		std::array<bool, static_cast<size_t>(TMaxedClassEnum::Max)> m_data;
	};
}}
