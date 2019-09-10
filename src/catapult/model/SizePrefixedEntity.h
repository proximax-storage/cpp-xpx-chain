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
#include "catapult/utils/NonCopyable.h"
#include <cstdint>

namespace catapult { namespace model {

#pragma pack(push, 1)

	/// Binary layout for a size prefixed entity.
	struct SizePrefixedEntity : public utils::NonCopyable {
	public:
		/// Entity size.
		uint32_t Size;

	protected:
		/// Returns byte-based const pointer to this entity.
		const uint8_t* ToBytePointer() const;

		/// Returns byte-based pointer to this entity.
		uint8_t* ToBytePointer();

		/// Gets the start of the variable data part of \a entity.
		template<typename T>
		static auto PayloadStart(T& entity) {
			return entity.Size != T::CalculateRealSize(entity) ? nullptr : entity.ToBytePointer() + sizeof(T);
		}

	public:
		/// Returns \c true if this entity is equal to \a rhs.
		bool operator==(const SizePrefixedEntity& rhs) const;

		/// Returns \c true if this entity is not equal to \a rhs.
		bool operator!=(const SizePrefixedEntity& rhs) const;
	};

#pragma pack(pop)
}}
