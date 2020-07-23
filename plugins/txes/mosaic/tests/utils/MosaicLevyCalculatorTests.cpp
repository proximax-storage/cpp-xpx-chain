/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/utils/MosaicLevyCalculator.h"
#include "tests/TestHarness.h"
#include "src/model/MosaicLevy.h"
#include "tests/test/MosaicTestUtils.h"
#include "tests/test/LevyTestUtils.h"

namespace catapult { namespace test {

#define TEST_CLASS MosaicCalculatorTest
	
	TEST(TEST_CLASS, CalculatorTestNone) {
		catapult::Amount amount(100);
		catapult::Amount levyFee(50);

		utils::MosaicLevyCalculatorFactory factory;
		auto  result = factory.getCalculator(model::LevyType::None)(amount, levyFee );
		
		EXPECT_EQ(Amount(0), result);
	}

	TEST(TEST_CLASS, CalculatorTestAbsolute) {
		catapult::Amount amount(100);
		catapult::Amount levyFee(50);

		utils::MosaicLevyCalculatorFactory factory;
		auto  result = factory.getCalculator(model::LevyType::Absolute)(amount, levyFee );
		
		EXPECT_EQ(Amount(50), result);
	}

	TEST(TEST_CLASS, CalculatorTestPercentile) {
		catapult::Amount amount(1000);
		catapult::Amount levyFee = test::CreateMosaicLevyFeePercentile(1.5);      // 1.5% percent levy fee

		utils::MosaicLevyCalculatorFactory factory;
		auto  result = factory.getCalculator(model::LevyType::Percentile)(amount, levyFee );
		
		EXPECT_EQ(Amount(15), result);
		
		levyFee = test::CreateMosaicLevyFeePercentile(50);                          // 50%  levy fee
		result = factory.getCalculator(model::LevyType::Percentile)(amount, levyFee );
		
		EXPECT_EQ(Amount(500), result);
			
		levyFee = test::CreateMosaicLevyFeePercentile(0.125);                         // 0.125%  levy fee
		result = factory.getCalculator(model::LevyType::Percentile)(amount, levyFee );
		
		EXPECT_EQ(Amount(1), result);        // should be 1.25 but decimal place cut off
	}
}}
