/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/io/BufferInputStreamAdapter.h"
#include "catapult/io/PodIoUtils.h"
#include "catapult/io/StringOutputStream.h"

namespace catapult { namespace cache {

	template<typename TDescriptor>
	class IdentifierSerializer {
	private:
		using ValueType = typename TDescriptor::ValueType;

	public:
		/// Serializes \a value to string.
		static std::string SerializeValue(const ValueType& value) {
			io::StringOutputStream output(sizeof(VersionType) + sizeof(ValueType));

			// write version
			io::Write32(output, 1);

			io::Write(output, value);

			return output.str();
		}

		/// Deserializes value from \a buffer.
		static ValueType DeserializeValue(const RawBuffer& buffer) {
			io::BufferInputStreamAdapter<RawBuffer> input(buffer);

			// read version
			VersionType version = io::Read32(input);
			if (version > 1)
			CATAPULT_THROW_RUNTIME_ERROR_1("invalid version of identifier", version);

			auto value = io::Read<ValueType>(input);

			return value;
		}

		/// Converts \a key to pruning boundary.
		static uint64_t KeyToBoundary(const ValueType& key) {
			return TDescriptor::KeyToBoundary(key);
		}
	};
}}
