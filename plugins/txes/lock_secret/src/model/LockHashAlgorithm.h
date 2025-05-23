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
#include <stdint.h>

namespace catapult { namespace model {

	/// Lock secret hash algorithm.
	enum class LockHashAlgorithm : uint8_t {
		/// Input is hashed using Sha-3-256.
		Op_Sha3_256,

		/// Input is hashed using Keccak-256.
		Op_Keccak_256,

		/// Input is hashed twice: first with SHA-256 and then with RIPEMD-160.
		Op_Hash_160,

		/// Input is hashed twice with SHA-256.
		Op_Hash_256,

		/// For internal use only.
		Op_Internal
	};
}}
