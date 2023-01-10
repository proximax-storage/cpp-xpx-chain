/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma  once
#include "catapult/types.h"
#include <set>

namespace catapult { namespace ionet { class Node; }}

namespace catapult { namespace dbrb {

	/// Changes of processes' membership in a view.
	enum class MembershipChange : uint8_t {
		Join,
		Leave,
	};

	using ProcessId = Key;
	using ViewData = std::set<std::pair<ProcessId, MembershipChange>>;

	constexpr size_t ProcessId_Size = Key_Size;
}}