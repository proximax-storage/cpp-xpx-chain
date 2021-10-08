/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include <cstdint>
#include <iosfwd>

namespace catapult { namespace utils {

#define SWAP_OPERATION_LIST \
	/* Buy service mosaics. */ \
	ENUM_VALUE(Buy) \
	\
	/* Sell service mosaics. */ \
	ENUM_VALUE(Sell) \

#define ENUM_VALUE(LABEL) LABEL,
	/// Enumeration of possible swap operations.
	enum class SwapOperation : uint8_t {
		SWAP_OPERATION_LIST
	};
#undef ENUM_VALUE

	/// Insertion operator for outputting \a value to \a out.
	std::ostream& operator<<(std::ostream& out, SwapOperation value);
}}