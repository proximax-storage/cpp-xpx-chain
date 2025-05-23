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
#include "catapult/io/BufferInputStreamAdapter.h"
#include "catapult/io/PodIoUtils.h"
#include "catapult/io/StringOutputStream.h"

namespace catapult { namespace cache {

	/// Serializer for identifier group elements.
	template<typename TDescriptor>
	class IdentifierGroupSerializer {
	private:
		using KeyType = typename TDescriptor::KeyType;
		using ValueType = typename TDescriptor::ValueType; // this is expected to be IdentifierGroup

	public:
		/// Serializes \a value to string.
		static std::string SerializeValue(const ValueType& value) {
			io::StringOutputStream output(sizeof(VersionType) + Size(value));

			// write version
			io::Write32(output, 1);

			io::Write(output, value.key());
			io::Write64(output, static_cast<uint64_t>(value.size()));
			for (const auto& identifier : value.identifiers())
				Write(output, identifier);

			return output.str();
		}

		/// Deserializes value from \a buffer.
		static ValueType DeserializeValue(const RawBuffer& buffer) {
			io::BufferInputStreamAdapter<RawBuffer> input(buffer);

			// read version
			VersionType version = io::Read32(input);
			if (version > 1)
				CATAPULT_THROW_RUNTIME_ERROR_1("invalid version of identifier group", version);

			auto key = io::Read<typename ValueType::GroupingKeyType>(input);
			ValueType value(key);

			auto size = io::Read64(input);
			for (auto i = 0u; i < size; ++i) {
				typename ValueType::Identifiers::key_type identifier;
				Read(input, identifier);
				value.add(identifier);
			}

			return value;
		}

	private:
		template<typename T>
		static void Read(io::InputStream& input, T& value) {
			io::Read(input, value);
		}

		template<size_t N, typename TTag>
		static void Read(io::InputStream& input, utils::ByteArray<N, TTag>& value) {
			input.read(value);
		}

		template<typename T>
		static void Write(io::OutputStream& input, const T& value) {
			io::Write(input, value);
		}

		template<size_t N, typename TTag>
		static void Write(io::OutputStream& input, const utils::ByteArray<N, TTag>& value) {
			input.write(value);
		}

	private:
		static size_t Size(const ValueType& value) {
			return sizeof(KeyType) + value.identifiers().size() * sizeof(ValueType);
		}
	};
}}
