/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include <stdint.h>

namespace catapult { namespace model {

	/// Account link transaction action.
	enum class LockFundAction : uint8_t {
		/// Lock mosaics.
		Lock,

		/// Unlock mosaics.
		Unlock
	};
}}
