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

#include "NamespaceCacheSerializers.h"

namespace catapult { namespace cache {

	std::string NamespaceFlatMapTypesSerializer::SerializeValue(const ValueType& value) {
		io::StringOutputStream output(sizeof(VersionType) + sizeof(ValueType));

		// write version
		io::Write32(output, 1);

		io::Write64(output, value.path().size());
		for (auto id : value.path())
			io::Write(output, id);

		return output.str();
	}

	state::Namespace NamespaceFlatMapTypesSerializer::DeserializeValue(const RawBuffer& buffer) {
		io::BufferInputStreamAdapter<RawBuffer> input(buffer);

		// read version
		VersionType version = io::Read32(input);
		if (version > 1)
			CATAPULT_THROW_RUNTIME_ERROR_1("invalid version of Namespace", version);

		state::Namespace::Path path;
		auto size = io::Read64(input);
		for (auto i = 0u; i < size; ++i)
			path.push_back(io::Read<NamespaceId>(input));

		return state::Namespace(path);
	}
}}
