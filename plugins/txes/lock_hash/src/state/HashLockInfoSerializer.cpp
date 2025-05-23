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

#include "HashLockInfoSerializer.h"

namespace catapult { namespace state {

	void HashLockInfoExtendedDataSerializer::Save(const HashLockInfo& lockInfo, io::OutputStream& output) {
		// write version
		io::Write32(output, 1);

		output.write(lockInfo.Hash);
	}

	void HashLockInfoExtendedDataSerializer::Load(io::InputStream& input, HashLockInfo& lockInfo) {
		// read version
		VersionType version = io::Read32(input);
		if (version > 1)
			CATAPULT_THROW_RUNTIME_ERROR_1("invalid version of HashLockInfo", version);

		input.read(lockInfo.Hash);
	}
}}
