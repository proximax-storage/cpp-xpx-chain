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
#include "ByteArray.h"
#include <cstring>
#include <tuple>
#include <string.h>
#include <stdint.h>
#include <stdio.h>

#define H1(s,i,x)   (x*65599u+(uint8_t)s[(i)<strlen(s)?strlen(s)-1-(i):strlen(s)])
#define H4(s,i,x)   H1(s,i,H1(s,i+1,H1(s,i+2,H1(s,i+3,x))))
#define H16(s,i,x)  H4(s,i,H4(s,i+4,H4(s,i+8,H4(s,i+12,x))))
#define H64(s,i,x)  H16(s,i,H16(s,i+16,H16(s,i+32,H16(s,i+48,x))))
#define H256(s,i,x) H64(s,i,H64(s,i+64,H64(s,i+128,H64(s,i+192,x))))

#define HASH(s)    ((uint32_t)(H256(s,0,0)^(H256(s,0,0)>>16)))

namespace catapult { namespace utils {

	/// Hasher object for a ByteArray with a variable offset.
	/// \note Offset defaults to 4 because because some arrays (e.g. Address) don't have a lot of entropy at the beginning.
	/// \note Hash is composed of only sizeof(size_t) bytes starting at offset.
	template<typename TArray, size_t Offset = 4>
	struct ArrayHasher {
		/// Hashes \a arrayData.
		size_t operator()(const TArray& array) const {
			size_t hash;
			std::memcpy(static_cast<void*>(&hash), &array[Offset], sizeof(size_t));
			return hash;
		}
	};

	/// Hasher object for a base value.
	template<typename TValue>
	struct BaseValueHasher {
		/// Hashes \a value.
		size_t operator()(TValue value) const {
			return static_cast<size_t>(value.unwrap());
		}
	};
}}
