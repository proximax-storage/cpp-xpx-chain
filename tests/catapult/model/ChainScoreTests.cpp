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

#include "catapult/model/ChainScore.h"
#include "tests/test/nodeps/Comparison.h"

namespace catapult { namespace model {

#define TEST_CLASS ChainScoreTests

	// region constructor / assign

	TEST(TEST_CLASS, CanCreateDefaultChainScore) {
		// Act:
		ChainScore score;
		auto scoreArray = score.toArray();

		// Assert:
		EXPECT_EQ(0u, scoreArray[0]);
		EXPECT_EQ(0u, scoreArray[1]);
	}

	TEST(TEST_CLASS, CanCreateChainScoreFrom64BitValue) {
		// Act:
		ChainScore score(0x7A6B3481023543B6);
		auto scoreArray = score.toArray();

		// Assert:
		EXPECT_EQ(0u, scoreArray[0]);
		EXPECT_EQ(0x7A6B3481023543B6u, scoreArray[1]);
	}

	TEST(TEST_CLASS, CanCreateChainScoreFrom128BitValue) {
		// Act:
		ChainScore score(0x7FDE42679C23D678, 0x7A6B3481023543B6);
		auto scoreArray = score.toArray();

		// Assert:
		EXPECT_EQ(0x7FDE42679C23D678u, scoreArray[0]);
		EXPECT_EQ(0x7A6B3481023543B6u, scoreArray[1]);
	}

	TEST(TEST_CLASS, CanCopyConstructChainScore) {
		// Act:
		ChainScore score(0x7FDE42679C23D678, 0x7A6B3481023543B6);
		ChainScore scoreCopy(score);
		auto scoreArray = scoreCopy.toArray();

		// Assert:
		EXPECT_EQ(0x7FDE42679C23D678u, scoreArray[0]);
		EXPECT_EQ(0x7A6B3481023543B6u, scoreArray[1]);
	}

	TEST(TEST_CLASS, CanAssignChainScore) {
		// Act:
		ChainScore score(0x7FDE42679C23D678, 0x7A6B3481023543B6);
		ChainScore scoreCopy;
		const auto& result = scoreCopy = score;
		auto scoreArray = scoreCopy.toArray();

		// Assert:
		EXPECT_EQ(&scoreCopy, &result);
		EXPECT_EQ(0x7FDE42679C23D678u, scoreArray[0]);
		EXPECT_EQ(0x7A6B3481023543B6u, scoreArray[1]);
	}

	// endregion

	// region comparison operators

	namespace {
		std::vector<ChainScore> GenerateIncreasingValues() {
			return {
				ChainScore(0x6FDE'4267'9C23'D678, 0x6A6B'3481'0235'43B6),
				ChainScore(0x6FDE'4267'9C23'D678, 0x6A6B'3481'0235'43B7),
				ChainScore(0x6FDE'4267'9C23'D678, 0x7A6B'3481'0235'43B7),
				ChainScore(0x6FDE'4267'9C23'D679, 0x7A6B'3481'0235'43B7),
				ChainScore(0x7FDE'4267'9C23'D679, 0x7A6B'3481'0235'43B7)
			};
		}
	}

	DEFINE_EQUALITY_AND_COMPARISON_TESTS(TEST_CLASS, GenerateIncreasingValues())

	// endregion

	// region add / subtract

	TEST(TEST_CLASS, CanAddToChainScore) {
		// Arrange:
		ChainScore score(0x6FDE'4267'9C23'D678, 0x7A6B'3481'0235'43B6);

		// Act:
		const auto& result = score += ChainScore(0x1000'0002'F000'0010, 0x0200'0500'0030'0005);
		auto scoreArray = score.toArray();

		// Assert:
		EXPECT_EQ(&score, &result);
		EXPECT_EQ(0x7FDE'426A'8C23'D688u, scoreArray[0]);
		EXPECT_EQ(0x7C6B'3981'0265'43BBu, scoreArray[1]);
	}

	TEST(TEST_CLASS, CanSubtractFromChainScore_Positive_Result) {
		// Arrange:
		ChainScore score(0x7FDE'4267'9C23'D678, 0x7A6B'3481'0235'43B6);

		// Act:
		const auto& result = score -= ChainScore(0x1000'0002'F000'0010, 0x0200'0500'0030'0005);
		auto scoreArray = score.toArray();

		// Assert:
		EXPECT_EQ(&score, &result);
		EXPECT_EQ(0x6FDE'4264'AC23'D668u, scoreArray[0]);
		EXPECT_EQ(0x786B'2F81'0205'43B1u, scoreArray[1]);
	}

	TEST(TEST_CLASS, CanSubtractFromChainScore_Negative_Result) {
		// Arrange:
		ChainScore score(0x7FDE'4267'9C23'D678, 0x7A6B'3481'0235'43B6);

		// Act:
		const auto& result = score -= ChainScore(0x7FDE'4267'9C23'D678, 0x7FDE'4267'9C23'D678);
		auto scoreArray = score.toArray();

		// Assert:
		EXPECT_EQ(&score, &result);
		EXPECT_EQ(0xFFFF'FFFF'FFFF'FFFFu, scoreArray[0]);
		EXPECT_EQ(0xFA8C'F219'6611'6D3Eu, scoreArray[1]);
	}

	// endregion

	// region insertion operator

	TEST(TEST_CLASS, CanOutput64BitChainScoreToStream) {
		// Arrange:
		ChainScore score(1235498201);

		// Act:
		auto str = test::ToString(score);

		// Assert:
		EXPECT_EQ("1235498201", str);
	}

	TEST(TEST_CLASS, CanOutput128BitChainScoreToStream) {
		// Arrange:
		ChainScore score(0x7FDE'4267'9C23'D678, 0x006B'3481'0235'43B6);

		// Act:
		std::stringstream out;
		out << std::uppercase << std::hex << score;

		// Assert:
		EXPECT_EQ("7FDE42679C23D678006B3481023543B6", out.str());
	}

	// endregion
}}
