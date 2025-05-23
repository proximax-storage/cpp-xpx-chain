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

#include "Random.h"
#include <random>
#include <mutex>

namespace catapult { namespace test {

	namespace {
		template<typename T>
		void RandomFill(T& container) {
			std::generate_n(container.begin(), container.size(), []() { return static_cast<typename T::value_type>(Random()); });
		}

		template<typename T>
		T GenerateRandomContainer(size_t size) {
			T container;
			container.resize(size);
			RandomFill(container);
			return container;
		}

		class RandomGenerator {
		public:
			RandomGenerator() {
				std::random_device rd;
				auto seed = (static_cast<uint64_t>(rd()) << 32) | rd();
				m_gen.seed(seed);
			}

		public:
			static RandomGenerator& instance() {
				static RandomGenerator generator;
				return generator;
			}

			uint64_t operator()() {
				std::lock_guard<std::mutex> lock(m_mutex);
				// this call mutates generator, so
				// we need to have serialized access to operator()
				return m_gen();
			}

		private:
			std::mutex m_mutex;
			std::mt19937_64 m_gen;
		};
	}

	uint64_t Random() {
		return RandomGenerator::instance()();
	}

	uint8_t RandomByte() {
		return static_cast<uint8_t>(Random());
	}

	uint16_t Random16() {
		return static_cast<uint16_t>(Random());
	}

	uint32_t Random32() {
		return static_cast<uint32_t>(Random());
	}

	std::string GenerateRandomString(size_t size) {
		return GenerateRandomContainer<std::string>(size);
	}

	std::string GenerateRandomHexString(size_t size) {
		std::string str;
		str.resize(size);
		std::generate_n(str.begin(), str.size(), []() {
			auto value = Random() % 16;
			return static_cast<char>(value < 10 ? (value + '0') : (value - 10 + 'a'));
		});
		return str;
	}

	std::vector<uint8_t> GenerateRandomVector(size_t size) {
		return GenerateRandomContainer<std::vector<uint8_t>>(size);
	}

	void FillWithRandomData(std::vector<uint8_t>& vec) {
		RandomFill(vec);
	}

	void FillWithRandomData(const MutableRawBuffer& dataBuffer) {
		std::generate_n(dataBuffer.pData, dataBuffer.Size, RandomByte);
	}

	void FillWithRandomData(UnresolvedAddress& unresolvedAddress) {
		FillWithRandomData({ reinterpret_cast<uint8_t*>(unresolvedAddress.data()), unresolvedAddress.size() });
	}
}}
