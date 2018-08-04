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

#include "ConfigurationValueParsers.h"
#include "StreamFormatGuard.h"
#include "XpxUnit.h"

namespace catapult { namespace utils {

	std::ostream& operator<<(std::ostream& out, XpxUnit unit) {
		StreamFormatGuard guard(out, std::ios::dec, '0');
		out << unit.xpx();
		if (unit.isFractional())
			out << '.' << std::setw(6) << (unit.m_value % XpxUnit::Microxpx_Per_Xpx);

		return out;
	}

	bool TryParseValue(const std::string& str, XpxUnit& parsedValue) {
		Amount microxpx;
		if (!TryParseValue(str, microxpx))
			return false;

		XpxUnit xpxUnit(microxpx);
		if (xpxUnit.isFractional())
			return false;

		parsedValue = xpxUnit;
		return true;
	}
}}
