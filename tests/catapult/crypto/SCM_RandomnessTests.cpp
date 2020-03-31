/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "tests/TestHarness.h"
#include <boost/random/random_device.hpp>
#include <bitset>
#include <fstream>
#include <random>

namespace catapult { namespace crypto {

#define TEST_CLASS SCM_RandomnessTests

	namespace {
		void WriteBits(std::ofstream& stream, uint64_t value) {
			std::bitset<64> bits(value);
			stream << bits;
		}

		uint64_t Random() {
			boost::random_device rd;
			std::mt19937_64 gen;
			auto seed = (static_cast<uint64_t>(rd()) << 32) | rd();
			gen.seed(seed);
			return gen();
		}
	}

	TEST(TEST_CLASS, Randomness) {
		// Arrange:
		std::ofstream outputStream("../resources/result/ProximaX RNG.txt");
		for (auto i = 0u; i < 1000u; ++i) {
			for (auto j = 0u; j < 15625u; ++j) {
				WriteBits(outputStream, Random());
			}
		}
	}
}}