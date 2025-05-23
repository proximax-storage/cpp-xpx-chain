/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "src/catapult/types.h"

namespace catapult { namespace state {

	struct AVLTreeNode {
		Key Left;
		Key Right;
		uint16_t Height = 1;
		uint32_t Size = 1;
	};

}}