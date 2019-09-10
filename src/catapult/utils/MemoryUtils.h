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

		return std::shared_ptr<T>(reinterpret_cast<T*>(::operator new(size)));
	}

	/// Converts a unique \a pointer to a shared pointer of the same type.
	template<typename T, typename Deleter>
	std::shared_ptr<T> UniqueToShared(std::unique_ptr<T, Deleter>&& pointer) {
		return std::move(pointer);
	}
}}
