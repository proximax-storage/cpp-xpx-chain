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

#include "AccountStateCacheSerializers.h"
#include "catapult/io/PodIoUtils.h"

namespace catapult { namespace cache {

	std::string KeyAddressPairSerializer::SerializeValue(const ValueType& value) {
		io::StringOutputStream output(sizeof(VersionType) + sizeof(Key) + sizeof(Address));

		// write version
		io::Write32(output, 1);

		output.write(value.first);
		output.write(value.second);
		return output.str();
	}

	KeyAddressPairSerializer::ValueType KeyAddressPairSerializer::DeserializeValue(const RawBuffer& buffer) {
		io::BufferInputStreamAdapter<RawBuffer> input(buffer);

		// read version
		VersionType version = io::Read32(input);
		if (version > 1)
			CATAPULT_THROW_RUNTIME_ERROR_1("invalid version of KeyAddress pair", version);

		ValueType value;
		input.read(value.first);
		input.read(value.second);
		return value;
	}
}}
