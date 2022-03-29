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

#include <cstdint>
namespace catapult::utils {

	uint64_t sqrt(uint64_t value);
	uint64_t pow(uint64_t a, uint64_t b);

	template<class T>
	T ceilDivision(T numerator, T denominator) {
		if (numerator == 0) {
			return 0;
		}
		return (numerator - 1) / denominator + 1;
	}

	template<class T>
	T floorDivision(T numerator, T denominator) {
		return numerator / denominator;
	}
} // namespace catapult::utils
