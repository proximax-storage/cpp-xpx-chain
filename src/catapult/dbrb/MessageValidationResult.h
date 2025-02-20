/**
*** Copyright 2024 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include <iosfwd>
#include <cstdint>

namespace catapult { namespace dbrb {

#define MESSAGE_VALIDATION_RESULT_LIST \
	ENUM_VALUE(Message_Valid, 0) \
	\
	ENUM_VALUE(Message_Invalid, 1) \
	\
	ENUM_VALUE(Message_Broadcast_Stopped, 2)

#define ENUM_VALUE(LABEL, VALUE) LABEL = VALUE,
	enum class MessageValidationResult : uint32_t {
		MESSAGE_VALIDATION_RESULT_LIST
	};
#undef ENUM_VALUE

	/// Insertion operator for outputting \a value to \a out.
	std::ostream& operator<<(std::ostream& out, MessageValidationResult value);
}}
