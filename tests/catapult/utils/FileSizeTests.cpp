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

#include "catapult/utils/FileSize.h"
#include "tests/test/nodeps/Comparison.h"
#include "tests/test/nodeps/Equality.h"

namespace catapult { namespace utils {

#define TEST_CLASS FileSizeTests

	// region creation

	TEST(TEST_CLASS, CanCreateDefaultFileSize) {
		// Assert:
		EXPECT_EQ(0u, FileSize().bytes());
	}

	TEST(TEST_CLASS, CanCreateFileSizeFromTerabytes) {
		// Assert:
		EXPECT_EQ(1024ull * 1024ull * 1024ull * 1024ull, FileSize::FromTerabytes(1).bytes());
		EXPECT_EQ(2 * 1024ull * 1024ull * 1024ull * 1024ull, FileSize::FromTerabytes(2).bytes());
		EXPECT_EQ(10 * 1024ull * 1024ull * 1024ull * 1024ull, FileSize::FromTerabytes(10).bytes());
		EXPECT_EQ(123 * 1024ull * 1024ull * 1024ull * 1024ull, FileSize::FromTerabytes(123).bytes());
	}

	TEST(TEST_CLASS, CanCreateFileSizeFromGigabytes) {
		// Assert:
		EXPECT_EQ(1024ull * 1024ull * 1024ull, FileSize::FromGigabytes(1).bytes());
		EXPECT_EQ(2 * 1024ull * 1024ull * 1024ull, FileSize::FromGigabytes(2).bytes());
		EXPECT_EQ(10 * 1024ull * 1024ull * 1024ull, FileSize::FromGigabytes(10).bytes());
		EXPECT_EQ(123 * 1024ull * 1024ull * 1024ull, FileSize::FromGigabytes(123).bytes());
	}

	TEST(TEST_CLASS, CanCreateFileSizeFromMegabytes) {
		// Assert:
		EXPECT_EQ(1024 * 1024u, FileSize::FromMegabytes(1).bytes());
		EXPECT_EQ(2 * 1024 * 1024u, FileSize::FromMegabytes(2).bytes());
		EXPECT_EQ(10 * 1024 * 1024u, FileSize::FromMegabytes(10).bytes());
		EXPECT_EQ(123 * 1024 * 1024u, FileSize::FromMegabytes(123).bytes());
	}

	TEST(TEST_CLASS, CanCreateFileSizeFromKilobytes) {
		// Assert:
		EXPECT_EQ(1024u, FileSize::FromKilobytes(1).bytes());
		EXPECT_EQ(2 * 1024u, FileSize::FromKilobytes(2).bytes());
		EXPECT_EQ(10 * 1024u, FileSize::FromKilobytes(10).bytes());
		EXPECT_EQ(123 * 1024u, FileSize::FromKilobytes(123).bytes());
	}

	TEST(TEST_CLASS, CanCreateFileSizeFromBytes) {
		// Assert:
		EXPECT_EQ(1u, FileSize::FromBytes(1).bytes());
		EXPECT_EQ(2u, FileSize::FromBytes(2).bytes());
		EXPECT_EQ(10u, FileSize::FromBytes(10).bytes());
		EXPECT_EQ(123u, FileSize::FromBytes(123).bytes());
	}

	// endregion

	// region accessor conversions

	TEST(TEST_CLASS, TerabytesAreTruncatedWhenConverted) {
		// Assert:
		constexpr uint64_t Base_Bytes = 10 * 1024ull * 1024ull * 1024ull * 1024ull;
		EXPECT_EQ(9u, FileSize::FromBytes(Base_Bytes - 1).terabytes());
		EXPECT_EQ(10u, FileSize::FromBytes(Base_Bytes).terabytes());
		EXPECT_EQ(10u, FileSize::FromBytes(Base_Bytes + 1).terabytes());
	}

	TEST(TEST_CLASS, GigabytesAreTruncatedWhenConverted) {
		// Assert:
		constexpr uint64_t Base_Bytes = 10 * 1024ull * 1024ull * 1024ull;
		EXPECT_EQ(9u, FileSize::FromBytes(Base_Bytes - 1).gigabytes());
		EXPECT_EQ(10u, FileSize::FromBytes(Base_Bytes).gigabytes());
		EXPECT_EQ(10u, FileSize::FromBytes(Base_Bytes + 1).gigabytes());
	}

	TEST(TEST_CLASS, MegabytesAreTruncatedWhenConverted) {
		// Assert:
		constexpr uint64_t Base_Bytes = 10 * 1024 * 1024u;
		EXPECT_EQ(9u, FileSize::FromBytes(Base_Bytes - 1).megabytes());
		EXPECT_EQ(10u, FileSize::FromBytes(Base_Bytes).megabytes());
		EXPECT_EQ(10u, FileSize::FromBytes(Base_Bytes + 1).megabytes());
	}

	TEST(TEST_CLASS, KilobytesAreTruncatedWhenConverted) {
		// Assert:
		constexpr uint64_t Base_Bytes = 10 * 1024u;
		EXPECT_EQ(9u, FileSize::FromBytes(Base_Bytes - 1).kilobytes());
		EXPECT_EQ(10u, FileSize::FromBytes(Base_Bytes).kilobytes());
		EXPECT_EQ(10u, FileSize::FromBytes(Base_Bytes + 1).kilobytes());
	}

	namespace {
		void Assert32BitFileSize(uint32_t value) {
			// Arrange:
			auto fileSize = FileSize::FromBytes(value);

			// Act + Assert: the value is accessible via bytes32 and bytes
			EXPECT_EQ(value, fileSize.bytes32());
			EXPECT_EQ(value, fileSize.bytes());
		}

		void Assert64BitFileSize(uint64_t value) {
			// Arrange:
			auto fileSize = FileSize::FromBytes(value);

			// Act + Assert: the value is accessible via bytes but not bytes32
			EXPECT_THROW(fileSize.bytes32(), catapult_runtime_error);
			EXPECT_EQ(value, fileSize.bytes());
		}
	}

	TEST(TEST_CLASS, Bytes32ReturnsBytesWhenBytes64FitsInto32Bit) {
		// Assert:
		using NumericLimits = std::numeric_limits<uint32_t>;
		Assert32BitFileSize(NumericLimits::min()); // min
		Assert32BitFileSize(1); // other values
		Assert32BitFileSize(1234);
		Assert32BitFileSize(8692);
		Assert32BitFileSize(NumericLimits::max()); // max
	}

	TEST(TEST_CLASS, Bytes32ThrowsWhenBytes64DoesNotFitInto32Bit) {
		// Assert:
		uint64_t max32 = std::numeric_limits<uint32_t>::max();
		Assert64BitFileSize(max32 + 1);
		Assert64BitFileSize(max32 + 1234);
		Assert64BitFileSize(max32 + 8692);
		Assert64BitFileSize(std::numeric_limits<uint64_t>::max());
	}

	// endregion

	// region equality operators

	namespace {
		std::unordered_set<std::string> GetEqualTags() {
			return { "10240 B", "10 KB", "10240 B (2)" };
		}

		std::unordered_map<std::string, FileSize> GenerateEqualityInstanceMap() {
			return {
				{ "10240 B", FileSize::FromBytes(10240) },
				{ "10 KB", FileSize::FromKilobytes(10) },
				{ "10240 B (2)", FileSize::FromBytes(10240) },

				{ "10239 B", FileSize::FromBytes(10239) },
				{ "10241 B", FileSize::FromBytes(10241) },
				{ "10240 KB", FileSize::FromKilobytes(10240) },
				{ "10 MB", FileSize::FromMegabytes(10) },
			};
		}
	}

	TEST(TEST_CLASS, OperatorEqualReturnsTrueOnlyForEqualValues) {
		// Assert:
		test::AssertOperatorEqualReturnsTrueForEqualObjects("10240 B", GenerateEqualityInstanceMap(), GetEqualTags());
	}

	TEST(TEST_CLASS, OperatorNotEqualReturnsTrueOnlyForUnequalValues) {
		// Assert:
		test::AssertOperatorNotEqualReturnsTrueForUnequalObjects("10240 B", GenerateEqualityInstanceMap(), GetEqualTags());
	}

	// endregion

	// region comparison operators

	namespace {
		std::vector<FileSize> GenerateIncreasingValues() {
			return {
				FileSize::FromBytes(10239),
				FileSize::FromKilobytes(10),
				FileSize::FromBytes(10241),
				FileSize::FromMegabytes(10),
				FileSize::FromKilobytes(10241)
			};
		}
	}

	DEFINE_COMPARISON_TESTS(TEST_CLASS, GenerateIncreasingValues())

	// endregion

	// region to string

	namespace {
		void AssertStringRepresentation(const std::string& expected, uint64_t numTerabytes, uint64_t numGigabytes, uint64_t numMegabytes, uint64_t numKilobytes, uint64_t numBytes) {
			// Arrange:
			auto timeSpan = FileSize::FromBytes(((((numTerabytes * 1024ull + numGigabytes) * 1024ull + numMegabytes) * 1024ull) + numKilobytes) * 1024ull + numBytes);

			// Act:
			auto str = test::ToString(timeSpan);

			// Assert:
			EXPECT_EQ(expected, str) << numTerabytes << "TB " << numGigabytes << "GB " << numMegabytes << "MB " << numKilobytes << "KB " << numBytes << "B";
		}
	}

	TEST(TEST_CLASS, CanOutputFileSize) {
		// Assert:
		// - zero
		AssertStringRepresentation("0B", 0, 0, 0, 0, 0);

		// - ones
		AssertStringRepresentation("1MB", 0, 0, 1, 0, 0);
		AssertStringRepresentation("1KB", 0, 0, 0, 1, 0);
		AssertStringRepresentation("1B", 0, 0, 0, 0, 1);

		// - overflows
		AssertStringRepresentation("20GB 20MB", 0, 0, 20500, 0, 0);
		AssertStringRepresentation("1GB 4MB", 0, 0, 1028, 0, 0);
		AssertStringRepresentation("1MB 4KB", 0, 0, 0, 1028, 0);
		AssertStringRepresentation("1KB 4B", 0, 0, 0, 0, 1028);

		// - all values
		AssertStringRepresentation("1TB 1GB 1MB 1KB 1B", 1, 1, 1, 1, 1);
		AssertStringRepresentation("3GB 1023MB 1023KB 1023B", 0, 3, 1023, 1023, 1023);
		AssertStringRepresentation("2TB 2GB 12MB 52KB 46B", 2, 2, 12, 52, 46);
		AssertStringRepresentation("4TB 12MB 46B", 4, 0, 12, 0, 46);
	}

	// endregion
}}
