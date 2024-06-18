/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include <cstdint>
#include <iosfwd>

namespace catapult { namespace fastfinality {

#define COMMITTEE_PHASE_LIST \
	/* Committee phase is unknown. */ \
	ENUM_VALUE(None) \
	\
	/* Propose a block. */ \
	ENUM_VALUE(Propose) \
	\
	/* Prevote phase. */ \
	ENUM_VALUE(Prevote) \
	\
	/* Precommit phase. */ \
	ENUM_VALUE(Precommit) \
	\
	/* Commit phase. */ \
	ENUM_VALUE(Commit)

#define ENUM_VALUE(LABEL) LABEL,
	/// Enumeration of possible committee phases.
	enum class CommitteePhase : uint8_t {
		COMMITTEE_PHASE_LIST
	};
#undef ENUM_VALUE

	/// Insertion operator for outputting \a value to \a out.
	std::ostream& operator<<(std::ostream& out, CommitteePhase value);
}}