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
#include <vector>
#include <mutex>

namespace catapult { namespace test {

	/// Base class for mocks that capture parameters.
	template<typename TParams>
	class ParamsCapture {
	public:
		virtual ~ParamsCapture() = default;

	public:
		/// Gets the captured parameters.
		const std::vector<TParams>& params() const {
			return m_params;
		}

	public:
		/// Clears the captured parameters.
		void clear() {
			std::lock_guard<std::mutex> lock(m_mutex);
			m_params.clear();
		}

	public:
		/// Captures \a args.
		template<typename... TArgs>
		void push(TArgs&&... args) {
			std::lock_guard<std::mutex> lock(m_mutex);
			m_params.emplace_back(std::forward<TArgs>(args)...);
		}

	private:
		std::mutex m_mutex;
		std::vector<TParams> m_params;
	};
}}
