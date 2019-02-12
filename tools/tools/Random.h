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
#include <random>
#include <stdint.h>

namespace catapult { namespace tools {

	/// Random generator.
	class RandomGenerator {
	public:
		/// Creates random generator using random system seed.
		RandomGenerator();

		/// Creates random generator using \a seed.
		explicit RandomGenerator(uint64_t seed);

	public:
		/// Generates next random number.
		uint64_t operator()();

	private:
		std::mt19937_64 m_gen;
	};

	/// Generates a uint64_t random number.
	uint64_t Random();

	/// Generates a uint8_t random number.
	uint8_t RandomByte();
}}
