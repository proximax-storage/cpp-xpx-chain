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

#include "src/catapult/utils/XpxUnit.h"
#include "tests/test/nodeps/Comparison.h"
#include "tests/test/nodeps/ConfigurationTestUtils.h"

namespace catapult { namespace utils {

#define TEST_CLASS XpxUnitTests

	// region constructor

	TEST(TEST_CLASS, CanCreateDefaultUnit) {
		// Arrange:
		XpxUnit unit;

		// Act + Assert:
		EXPECT_EQ(XpxAmount(0), unit.xpx());
		EXPECT_EQ(Amount(0), unit.microxpx());
		EXPECT_FALSE(unit.isFractional());
	}

	TEST(TEST_CLASS, CanCreateUnitFromAmount) {
		// Arrange:
		XpxUnit unit(Amount(123'000'000));

		// Act + Assert:
		EXPECT_EQ(XpxAmount(123), unit.xpx());
		EXPECT_EQ(Amount(123'000'000), unit.microxpx());
		EXPECT_FALSE(unit.isFractional());
	}

	TEST(TEST_CLASS, CanCreateUnitFromXpxAmount) {
		// Arrange:
		XpxUnit unit(XpxAmount(123'000'000));

		// Act + Assert:
		EXPECT_EQ(XpxAmount(123'000'000), unit.xpx());
		EXPECT_EQ(Amount(123'000'000'000'000), unit.microxpx());
		EXPECT_FALSE(unit.isFractional());
	}

	TEST(TEST_CLASS, CanCreateUnitFromFractionalAmount) {
		// Arrange:
		XpxUnit unit(Amount(123'789'432));

		// Act + Assert:
		EXPECT_EQ(XpxAmount(123), unit.xpx());
		EXPECT_EQ(Amount(123'789'432), unit.microxpx());
		EXPECT_TRUE(unit.isFractional());
	}

	// endregion

	// region copy + assign

	TEST(TEST_CLASS, CanCopyAssign) {
		// Arrange:
		XpxUnit unit(XpxAmount(123));
		XpxUnit copy(XpxAmount(456));

		// Act:
		const auto& assignResult = (copy = unit);

		// Assert:
		EXPECT_EQ(XpxAmount(123), unit.xpx());
		EXPECT_EQ(XpxAmount(123), copy.xpx());
		EXPECT_EQ(&copy, &assignResult);
	}

	TEST(TEST_CLASS, CanCopyConstruct) {
		// Act:
		XpxUnit unit(XpxAmount(123));
		XpxUnit copy(unit);

		// Assert:
		EXPECT_EQ(XpxAmount(123), unit.xpx());
		EXPECT_EQ(XpxAmount(123), copy.xpx());
	}

	TEST(TEST_CLASS, CanMoveAssign) {
		// Arrange:
		XpxUnit unit(XpxAmount(123));
		XpxUnit copy(XpxAmount(456));

		// Act:
		const auto& assignResult = (copy = std::move(unit));

		// Assert:
		EXPECT_EQ(XpxAmount(123), copy.xpx());
		EXPECT_EQ(&copy, &assignResult);
	}

	TEST(TEST_CLASS, CanMoveConstruct) {
		// Act:
		XpxUnit unit(XpxAmount(123));
		XpxUnit copy(std::move(unit));

		// Assert:
		EXPECT_EQ(XpxAmount(123), copy.xpx());
	}

	// endregion

	// region custom assign

	TEST(TEST_CLASS, CanAssignUnitFromAmount) {
		// Act:
		XpxUnit unit;
		const auto& assignResult = (unit = Amount(123'000'000));

		// Assert:
		EXPECT_EQ(XpxAmount(123), unit.xpx());
		EXPECT_EQ(Amount(123'000'000), unit.microxpx());
		EXPECT_FALSE(unit.isFractional());
		EXPECT_EQ(&unit, &assignResult);
	}

	TEST(TEST_CLASS, CanAssignUnitFromXpxAmount) {
		// Act:
		XpxUnit unit;
		const auto& assignResult = (unit = XpxAmount(123'000'000));

		// Assert:
		EXPECT_EQ(XpxAmount(123'000'000), unit.xpx());
		EXPECT_EQ(Amount(123'000'000'000'000), unit.microxpx());
		EXPECT_FALSE(unit.isFractional());
		EXPECT_EQ(&unit, &assignResult);
	}

	TEST(TEST_CLASS, CanAssignUnitFromFractionalAmount) {
		// Arrange:
		XpxUnit unit;
		const auto& assignResult = (unit = Amount(123'789'432));

		// Assert:
		EXPECT_EQ(XpxAmount(123), unit.xpx());
		EXPECT_EQ(Amount(123'789'432), unit.microxpx());
		EXPECT_TRUE(unit.isFractional());
		EXPECT_EQ(&unit, &assignResult);
	}

	// endregion

	// region comparison operators

	namespace {
		std::vector<XpxUnit> GenerateIncreasingValues() {
			return { XpxUnit(Amount(123)), XpxUnit(Amount(642)), XpxUnit(Amount(989)) };
		}
	}

	DEFINE_EQUALITY_TESTS(TEST_CLASS, GenerateIncreasingValues())

	// endregion

	// region to string

	TEST(TEST_CLASS, CanOutputWholeUnitAmount) {
		// Arrange:
		XpxUnit unit(XpxAmount(123));

		// Act:
		auto str = test::ToString(unit);

		// Assert:
		EXPECT_EQ("123", str);
	}

	TEST(TEST_CLASS, CanOutputFractionalUnitAmount) {
		// Arrange:
		XpxUnit unit(Amount(123'009'432));

		// Act:
		auto str = test::ToString(unit);

		// Assert:
		EXPECT_EQ("123.009432", str);
	}

	TEST(TEST_CLASS, OutputFormattingChangesDoNotLeak) {
		// Arrange:
		std::ostringstream out;
		out.flags(std::ios::hex | std::ios::uppercase);
		out.fill('~');

		// Act:
		out << std::setw(4) << 0xAB << " " << XpxUnit(Amount(123'009'432)) << " " << std::setw(4) << 0xCD;
		auto actual = out.str();

		// Assert:
		EXPECT_EQ("~~AB 123.009432 ~~CD", actual);
	}

	// endregion

	// region TryParseValue

	TEST(TEST_CLASS, TryParseValueFailsWhenParsingInvalidAmount) {
		test::AssertFailedParse("45Z'000'000", XpxUnit(), [](const auto& str, auto& parsedValue) {
			return TryParseValue(str, parsedValue);
		});
	}

	TEST(TEST_CLASS, TryParseValueFailsWhenParsingFractionalAmount) {
		test::AssertFailedParse("450'000'001", XpxUnit(), [](const auto& str, auto& parsedValue) {
			return TryParseValue(str, parsedValue);
		});
	}

	TEST(TEST_CLASS, TryParseValueSucceedsWhenParsingWholeUnitAmount) {
		test::AssertParse("450'000'000", XpxUnit(XpxAmount(450)), [](const auto& str, auto& parsedValue) {
			return TryParseValue(str, parsedValue);
		});
	}

	// endregion
}}
