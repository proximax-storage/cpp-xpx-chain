#pragma once

#include "src/catapult/types.h"

namespace catapult { namespace state {

	using Pointer = Key;

	struct AVLTreeNode {
		Pointer m_left;
		Pointer m_right;
		uint16_t m_height = 1;
	};

}} // namespace catapult::state