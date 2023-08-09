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
#include "TestElement.h"
#include "catapult/deltaset/DeltaElements.h"
#include "catapult/cache_db/RdbTypedColumnContainer.h"
#include "catapult/utils/Hashers.h"
#include "catapult/cache/CacheDescriptorAdapters.h"
#include "catapult/io/StringOutputStream.h"
#include "catapult/io/BufferInputStreamAdapter.h"
#include <map>
#include <unordered_map>
#include "catapult/io/PodIoUtils.h"
#include "tests/test/cache/StringKey.h"
#include "tests/test/cache/BasicMapDescriptor.h"
#include "catapult/io/SizeCalculatingOutputStream.h"
#include "catapult/utils/RawBuffer.h"
namespace catapult { namespace test {


	/// Test helpers for interacting with delta elements representing mutable element values with storage virtualization.
	struct RealTestValue {
	public:

		std::string KeyCopy; //10
		int Integer; //4
		std::string RandomData; //20
	};
	struct RealColumnDescriptor : public test::BasicMapDescriptor<test::StringKey, RealTestValue> {
	public:
		struct Serializer {
		public:

			static void Save(const ValueType& value, io::OutputStream& output) {
				io::Write32(output, value.KeyCopy.size());
				output.write(utils::RawBuffer(
						reinterpret_cast<const unsigned char*>(value.KeyCopy.c_str()), value.KeyCopy.size()));
				io::Write32(output, value.RandomData.size());
				output.write(utils::RawBuffer(
						reinterpret_cast<const unsigned char*>(value.RandomData.c_str()), value.RandomData.size()));
				io::Write32(output, value.Integer);
			}
			static std::string SerializeValue(const ValueType& value) {
				io::SizeCalculatingOutputStream calculator;
				Save(value, calculator);
				io::StringOutputStream output(calculator.size());
				Save(value, output);
				return output.str();
			}

			static ValueType DeserializeValue(const RawBuffer& buffer) {
				io::BufferInputStreamAdapter<RawBuffer> input(buffer);
				RealTestValue returnValue;
				auto size = io::Read32(input);
				std::vector<uint8_t> key(size);
				io::Read(input, key);
				size = io::Read32(input);
				std::vector<uint8_t> additional(size);
				io::Read(input, additional);
				returnValue.Integer = io::Read32(input);
				returnValue.KeyCopy = std::string(key.begin(), key.end());
				returnValue.RandomData = std::string(additional.begin(), additional.end());
				return returnValue;
			}

			static uint64_t KeyToBoundary(const KeyType& key) {
				return key.size();
			}
		};
	};

	class DeltaElementsTestUtils {
	public:

		/// Type definitions.
		struct Types {
		private:
			using ElementType = SetElementType<MutableElementValueTraits>;

		public:
			using StorageMapType = std::map<std::pair<std::string, unsigned int>, ElementType>;
			using MemoryMapType = std::unordered_map<std::pair<std::string, unsigned int>, ElementType, MapKeyHasher>;
			using RealStorageMapType = cache::RdbTypedColumnContainer<RealColumnDescriptor>;
			using RealMemoryMapType = std::unordered_map<std::string, RealColumnDescriptor::ValueType, MapKeyHasher>;
			// to emulate storage virtualization, use two separate sets (ordered and unordered)
			using StorageTraits = deltaset::MapStorageTraits<StorageMapType, TestElementToKeyConverter<ElementType>, MemoryMapType>;
			// to emulate true storage, use two separate sets (ordered and unordered)
			using RealStorageTraits = deltaset::MapStorageTraits<
					deltaset::ConditionalContainer<
							RealColumnDescriptor,
							RealStorageMapType,
							RealMemoryMapType
							>,
					RealColumnDescriptor,
					RealMemoryMapType
					>;
		};

	public:
		/// Storage for delta elements.
		template<typename TSet>
		struct Wrapper {
		public:
			using SetType = TSet;
			using MemorySetType = SetType;

		public:
			/// Added elements.
			SetType Added;

			/// Removed elements.
			SetType Removed;

			/// Copied elements.
			SetType Copied;

		public:
			/// Returns a delta elements around the sub sets.
			auto deltas() const {
				return deltaset::DeltaElements<SetType>(Added, Removed, Copied);
			}
		};

		/// Mixin that provides generational change emulation.
		template<typename TSet>
		class GenerationalChangeMixin {
		public:
			using KeyType = typename TSet::key_type;

		public:
			/// Creates mixin.
			GenerationalChangeMixin() : m_generationId(1)
			{}

		public:
			/// Gets the current generation id.
			uint32_t generationId() const {
				return m_generationId;
			}

			/// Gets the generation id associated with \a key.
			uint32_t generationId(const KeyType& key) const {
				// unlike BaseSetDelta, default generation is initial generation (1) instead of unset generation (0)
				auto iter = m_keyGenerationIdMap.find(key);
				return m_keyGenerationIdMap.cend() == iter ? 1 : iter->second;
			}

			/// Sets the generation id (\a generationId) for \a key.
			void setGenerationId(const KeyType& key, uint32_t generationId) {
				m_keyGenerationIdMap[key] = generationId;
			}

			/// Increments the generation id.
			void incrementGenerationId() {
				++m_generationId;
			}

		private:
			uint32_t m_generationId;
			std::unordered_map<KeyType, uint32_t> m_keyGenerationIdMap;
		};

		/// Storage for delta elements with generational support.
		template<typename TSet>
		struct WrapperWithGenerationalSupport
				: public Wrapper<TSet>
				, public GenerationalChangeMixin<TSet>
		{};
	};

	template<typename TContainer>
	auto ToSlice(const TContainer& container) {
		return rocksdb::Slice(reinterpret_cast<const char*>(container.data()), container.size());
	}
}}
