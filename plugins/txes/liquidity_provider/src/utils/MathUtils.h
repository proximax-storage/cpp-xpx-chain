#pragma once

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
			}
			else {
				right = m;
			}
		}
		return left;
	}
}
