//
// Created by kyrylo on 29.03.2022.
//
#include <algorithm>
#include "MathUtils.h"

namespace catapult::utils {

	uint64_t sqrt(uint64_t value) {
		const uint64_t maxRoot = 4294967295UL;
		uint64_t left = 0;
		// Right bound is unreachable
		uint64_t right = std::min(value, maxRoot) + 1;

		while (left + 1 < right) {
			uint64_t m = (left + right) / 2;
			if (m * m <= value) {
				left = m;
			} else {
				right = m;
			}
		}
		return left;
	}

	uint64_t pow(uint64_t a, uint64_t b) {
		if (b == 0) {
			return 1;
		}
		if (b % 2 == 0) {
			return pow(a * a, b / 2);
		}
		return a * pow(a, b - 1);
	}
} // namespace catapult::utils