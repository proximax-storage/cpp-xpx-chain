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

#include "BlockSpan.h"
#include "IntegerMath.h"

namespace catapult { namespace utils {

	std::ostream& operator<<(std::ostream& out, const BlockSpan& blockSpan) {
		// calculate components
		auto totalHours = blockSpan.hours();
		auto hours = DivideAndGetRemainder<uint64_t>(totalHours, 24);
		auto days = totalHours;

		// output as 0d 0h
		out << days << "d " << hours << "h";
		return out;
	}
}}
