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

#include "catapult/model/InflationCalculator.h"
#include "tests/TestHarness.h"

namespace catapult { namespace model {

#define TEST_CLASS InflationCalculatorTests

	namespace {
		struct TestInflationCalculator : public InflationCalculator {
			TestInflationCalculator() : InflationCalculator(Amount(10000), Amount(0)) {

			}
			IntervalMetadata getIntervalMetadata(Height height) {
				return m_intervalMetadata.at(height);
			}
			bool containsIntervalMetadata(Height height) {
				return m_intervalMetadata.find(height) != m_intervalMetadata.cend();
			}
			Amount getConfiguredMinimum() {
				return m_initialCurrencyAtomicUnits;
			}
		};
		struct InflationEntry {
			catapult::Height Height;
			catapult::Amount Amount;
			catapult::Amount MaxMosaics;
		};

		TestInflationCalculator CreateInflationCalculator(const std::vector<InflationEntry>& inflationEntries) {
			TestInflationCalculator calculator;
			for (const auto& inflationEntry : inflationEntries)
				calculator.add(inflationEntry.Height, inflationEntry.Amount, inflationEntry.MaxMosaics+calculator.getConfiguredMinimum());

			// Sanity:
			EXPECT_EQ(inflationEntries.size(), calculator.size());

			return calculator;
		}
	}

	// region ctor

	TEST(TEST_CLASS, InflationCalculatorIsInitiallyEmpty) {
		// Act:
		TestInflationCalculator calculator;

		// Assert:
		EXPECT_EQ(0u, calculator.size());
	}

	// endregion

	// region add

	TEST(TEST_CLASS, CanAddSingleInflationEntry) {
		// Arrange:
		TestInflationCalculator calculator;

		// Act:
		calculator.add(Height(1), Amount(50), Amount(200)+calculator.getConfiguredMinimum());

		// Assert:
		EXPECT_EQ(1u, calculator.size());
		EXPECT_TRUE(calculator.contains(Height(1), Amount(50)));
		EXPECT_TRUE(calculator.containsIntervalMetadata(Height(1)));

		// Continue:

		auto metadata = calculator.getIntervalMetadata(Height(1));

		EXPECT_EQ(metadata.LastRewardHeight, Height(4));
		EXPECT_EQ(metadata.TotalInflationWithinInterval, Amount(200));
		EXPECT_EQ(metadata.InflationRemainder, Amount(0));
		EXPECT_EQ(metadata.StartTotalInflation, Amount(0));
	}

	TEST(TEST_CLASS, CannotAddSingleInflationEntryAboveNemesisAsFirstRecord) {
		// Arrange:
		TestInflationCalculator calculator;

		// Act/Assert:

		EXPECT_THROW(calculator.add(Height(4), Amount(50), Amount(500)), catapult_invalid_argument);
	}

	TEST(TEST_CLASS, CanAddMultipleInflationEntries) {
		// Arrange:
		TestInflationCalculator calculator;

		// Act:
		calculator.add(Height(1), Amount(345), Amount(5000)+calculator.getConfiguredMinimum());
		calculator.add(Height(15), Amount(234), Amount(5000)+calculator.getConfiguredMinimum());
		calculator.add(Height(25), Amount(123), Amount(55000)+calculator.getConfiguredMinimum());

		// Assert:
		EXPECT_EQ(3u, calculator.size());
		EXPECT_TRUE(calculator.contains(Height(1), Amount(345)));
		EXPECT_TRUE(calculator.contains(Height(15), Amount(234)));
		EXPECT_TRUE(calculator.contains(Height(25), Amount(123)));

		EXPECT_TRUE(calculator.containsIntervalMetadata(Height(1)));
		EXPECT_TRUE(calculator.containsIntervalMetadata(Height(15)));
		EXPECT_TRUE(calculator.containsIntervalMetadata(Height(25)));

		auto metadata = calculator.getIntervalMetadata(Height(1));

		EXPECT_EQ(metadata.LastRewardHeight, Height(15));
		EXPECT_EQ(metadata.TotalInflationWithinInterval, Amount(4830));
		EXPECT_EQ(metadata.InflationRemainder, Amount(0));
		EXPECT_EQ(metadata.StartTotalInflation, Amount(0));

		metadata = calculator.getIntervalMetadata(Height(15));

		EXPECT_EQ(metadata.LastRewardHeight, Height(15));
		EXPECT_EQ(metadata.TotalInflationWithinInterval, Amount(170));
		EXPECT_EQ(metadata.InflationRemainder, Amount(170));
		EXPECT_EQ(metadata.StartTotalInflation, Amount(4830));

		metadata = calculator.getIntervalMetadata(Height(25));

		EXPECT_EQ(metadata.LastRewardHeight, Height(431));
		EXPECT_EQ(metadata.TotalInflationWithinInterval, Amount(50000));
		EXPECT_EQ(metadata.InflationRemainder, Amount(62));
		EXPECT_EQ(metadata.StartTotalInflation, Amount(5000));
	}

	TEST(TEST_CLASS, CannotAddInflationEntryAtHeightZero) {
		// Arrange:
		TestInflationCalculator calculator;

		// Act + Assert:
		EXPECT_THROW(calculator.add(Height(0), Amount(123), Amount(10000)), catapult_invalid_argument);
	}

	TEST(TEST_CLASS, CannotAddInflationEntryWhenHeightIsEqualToLastEntryHeight) {
		// Arrange:
		auto calculator = CreateInflationCalculator({ { Height(1), Amount(345), Amount(10000) }, { Height(15), Amount(123), Amount(10000) } });

		// Assert:
		EXPECT_THROW(calculator.add(Height(15), Amount(234), Amount(100)), catapult_invalid_argument);
	}

	TEST(TEST_CLASS, CannotAddInflationEntryWhenHeightIsLessThanLastEntryHeight) {
		// Arrange:
		auto calculator = CreateInflationCalculator({ { Height(1), Amount(345), Amount(10000) }, { Height(15), Amount(123), Amount(10000) } });

		// Assert:
		EXPECT_THROW(calculator.add(Height(5), Amount(456), Amount(10000)), catapult_invalid_argument);
		EXPECT_THROW(calculator.add(Height(10), Amount(567), Amount(10000)), catapult_invalid_argument);
		EXPECT_THROW(calculator.add(Height(14), Amount(678), Amount(10000)), catapult_invalid_argument);
	}

	// endregion

	// region getSpotAmount

	namespace {
		constexpr const char* Height_Message = "at height ";
	}

	TEST(TEST_CLASS, GetSpotAmountThrowsWhenMapIsEmptyAndNotNemesisBlock) {
		// Arrange:
		TestInflationCalculator calculator;

		// Act + Assert:
		for (auto rawHeight : { 0u,  10u, 123456u })
			EXPECT_THROW(calculator.getSpotAmount(Height(rawHeight)), catapult_runtime_error) << Height_Message << rawHeight;
	}

	TEST(TEST_CLASS, GetSpotAmountReturnsExpectedAmount_HeightExistsInMap) {
		// Arrange:
		std::vector<InflationEntry> entries{ { Height(1), Amount(345), Amount(5000) }, { Height(15), Amount(234), Amount(5000) }, { Height(25), Amount(123), Amount(55000) } };
		auto calculator = CreateInflationCalculator(entries);

		// Act + Assert:
		EXPECT_EQ(Amount(345), calculator.getSpotAmount(Height(1)));
		EXPECT_EQ(Amount(345), calculator.getSpotAmount(Height(5)));
		EXPECT_EQ(Amount(170), calculator.getSpotAmount(Height(15)));
		EXPECT_EQ(Amount(0), calculator.getSpotAmount(Height(16)));
		EXPECT_EQ(Amount(123), calculator.getSpotAmount(Height(25)));
		EXPECT_EQ(Amount(123), calculator.getSpotAmount(Height(26)));
		EXPECT_EQ(Amount(0), calculator.getSpotAmount(Height(999999)));
	}
	// endregion

	// region getCumulativeAmount

	TEST(TEST_CLASS, GetCumulativeAmounThrowsWhenMapIsEmpty) {
		// Arrange:
		TestInflationCalculator calculator;

		// Act + Assert:
		for (auto rawHeight : { 5u, 10u, 123456u })
			EXPECT_THROW(calculator.getCumulativeAmount(Height(rawHeight)), catapult_runtime_error) << Height_Message << rawHeight;
	}

	TEST(TEST_CLASS, GetCumulativeAmountReturnsZeroAtHeightZeroOrOne) {
		// Arrange:

		std::vector<InflationEntry> entries{ { Height(1), Amount(345), Amount(5000) }, { Height(15), Amount(234), Amount(5000) }, { Height(25), Amount(123), Amount(55000) } };
		auto calculator = CreateInflationCalculator(entries);

		// Act + Assert:
		EXPECT_EQ(Amount(), calculator.getCumulativeAmount(Height(0)));
		EXPECT_EQ(Amount(), calculator.getCumulativeAmount(Height(1)));
	}

	TEST(TEST_CLASS, GetCumulativeAmountReturnsExpectedAmount_HeightExistsInMap) {
		// Arrange:
		std::vector<InflationEntry> entries{  { Height(1), Amount(345), Amount(5000) }, { Height(15), Amount(234), Amount(5000) }, { Height(25), Amount(123), Amount(55000) } };
		auto calculator = CreateInflationCalculator(entries);

		// Act + Assert: total inflation does not include the height provided in the call to getTotalAmount
		EXPECT_EQ(Amount(), calculator.getCumulativeAmount(Height(1))); // always zero up to first entry;
		EXPECT_EQ(Amount(4830), calculator.getCumulativeAmount(Height(15)));
		EXPECT_EQ(Amount(5000), calculator.getCumulativeAmount(Height(25)));
		EXPECT_EQ(Amount(5000+123*3), calculator.getCumulativeAmount(Height(28)));
	}

	// endregion

	// region sumAll

	TEST(TEST_CLASS, SumAllReturnsTotalInflationWhenNotEncounteringOverflow) {
		// Arrange:
		auto calculator = CreateInflationCalculator({  { Height(1), Amount(345), Amount(5000) }, { Height(15), Amount(234), Amount(5000) }, { Height(25), Amount(123), Amount(55000) } });

		// Act:
		auto totalInflation = calculator.sumAll();

		// Assert:
		EXPECT_EQ(Amount(55000), totalInflation);
	}


	TEST(TEST_CLASS, SumAllReturnsDifferentValueWhenMaxAmountIsIncreased) {
		// Arrange:
		// Arrange:
		auto calculator = CreateInflationCalculator({  { Height(1), Amount(345), Amount(5000) }, { Height(15), Amount(234), Amount(5000) }, { Height(25), Amount(123), Amount(55000) } });

		// Act:
		auto totalInflation = calculator.sumAll();

		// Sanity:
		EXPECT_EQ(Amount(55000), totalInflation);

		// Proceed:
		calculator.add(Height(100), Amount(3), Amount(999999));

		totalInflation = calculator.sumAll();

		// Assert:
		EXPECT_EQ(Amount(999999)-calculator.getConfiguredMinimum(), totalInflation);

	}
	// endregion
}}
