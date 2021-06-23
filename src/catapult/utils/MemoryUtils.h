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
#include "catapult/exceptions.h"
#include "catapult/model/EntityPtr.h"
#include <memory>
#include <forward_list>

namespace catapult { namespace utils {

	/// Creates a unique pointer of the specified type with custom \a size.
	template<typename T>
	model::UniqueEntityPtr<T> MakeUniqueWithSize(size_t size) {
		if (size < sizeof(T))
			CATAPULT_THROW_INVALID_ARGUMENT("size is insufficient");

		return model::UniqueEntityPtr<T>(static_cast<T*>(::operator new(size)));
	}

	/// Creates a shared pointer of the specified type with custom \a size.
	template<typename T>
	std::shared_ptr<T> MakeSharedWithSize(size_t size) {
		if (size < sizeof(T))
			CATAPULT_THROW_INVALID_ARGUMENT("size is insufficient");

		return std::shared_ptr<T>(reinterpret_cast<T*>(::operator new(size)), model::EntityPtrDeleter<T>{});
	}

	/// Converts a unique \a pointer to a shared pointer of the same type.
	template<typename T, typename Deleter>
	std::shared_ptr<T> UniqueToShared(std::unique_ptr<T, Deleter>&& pointer) {
		return std::move(pointer);
	}
	/// Copies \a count bytes from \a pSrc to \a pDest.
	/// \note This wrapper only requires valid pointers when \a count is nonzero.
	inline void memcpy_cond(void* pDest, const void* pSrc, size_t count) {
		if (0 < count)
			std::memcpy(pDest, pSrc, count);
	}
	/// A simple memory pool for allocating memory chunks for objects or arrays of objects.
	/// Note: constructors/destructors are not called on object memory allocation/deallocation.
	struct Mempool {
	public:
		/// Allocates memory sufficient to store \a count of uninitialized objects of the given type.
		/// Returns pointer the first allocated object.
		template<typename T>
		T* malloc(size_t count) {
			m_allocatedMemory.emplace_front(std::vector<uint8_t>(count * sizeof(T)));
			return reinterpret_cast<T*>(m_allocatedMemory.front().data());
		}

		/// Allocates memory sufficient to store an object of the given type and
		/// copy content of \a obj to the allocated memory.
		template<typename T>
		T* malloc(const T& obj) {
			auto begin = reinterpret_cast<const uint8_t*>(&obj);
			m_allocatedMemory.emplace_front(std::vector<uint8_t>(begin, begin + sizeof(T)));
			return reinterpret_cast<T*>(m_allocatedMemory.front().data());
		}
	private:
		std::forward_list<std::vector<uint8_t>> m_allocatedMemory;
	};
}}
