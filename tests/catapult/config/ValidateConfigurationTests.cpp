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

#include "catapult/config/ValidateConfiguration.h"
#include "catapult/config/CatapultConfiguration.h"
#include "catapult/utils/ConfigurationBag.h"
#include "tests/test/net/NodeTestUtils.h"
#include "tests/test/other/MutableCatapultConfiguration.h"
#include "tests/TestHarness.h"

namespace catapult { namespace config {

#define TEST_CLASS ValidateConfigurationTests

	namespace {
		// the key is invalid because it contains a non hex char ('G')
		const char* Invalid_Private_Key = "3485D98EFD7EB07ABAFCFD1A157D89DE2G96A95E780813C0258AF3F5F84ED8CB";
		const char* Valid_Private_Key = "3485D98EFD7EB07ABAFCFD1A157D89DE2796A95E780813C0258AF3F5F84ED8CB";

		auto CreateMutableCatapultConfiguration() {
			test::MutableCatapultConfiguration config;

			auto& blockChainConfig = config.BlockChain;
			blockChainConfig.ImportanceGrouping = 1;
			blockChainConfig.MaxMosaicAtomicUnits = Amount(1000);

			auto& inflationConfig = config.Inflation;
			inflationConfig.InflationCalculator.add(Height(1), Amount(1));
			inflationConfig.InflationCalculator.add(Height(100), Amount());

			auto& userConfig = config.User;
			userConfig.BootKey = Valid_Private_Key;

			return config;
		}
	}

	// region boot key validation

	namespace {
		void AssertInvalidBootKey(const std::string& bootKey) {
			// Arrange:
			auto mutableConfig = CreateMutableCatapultConfiguration();
			mutableConfig.User.BootKey = bootKey;

			// Act + Assert:
			EXPECT_THROW(ValidateConfiguration(mutableConfig.ToConst()), utils::property_malformed_error);
		}
	}

	TEST(TEST_CLASS, ValidationFailsWhenBootKeyIsInvalid) {
		// Assert:
		AssertInvalidBootKey(Invalid_Private_Key);
		AssertInvalidBootKey("");
	}

	// endregion

	// region importance grouping validation

	namespace {
		auto CreateCatapultConfiguration(uint32_t importanceGrouping, uint32_t maxRollbackBlocks) {
			auto mutableConfig = CreateMutableCatapultConfiguration();
			mutableConfig.BlockChain.ImportanceGrouping = importanceGrouping;
			mutableConfig.BlockChain.MaxRollbackBlocks = maxRollbackBlocks;
			return mutableConfig.ToConst();
		}
	}

	TEST(TEST_CLASS, ImportanceGroupingIsValidatedAgainstMaxRollbackBlocks) {
		// Arrange:
		auto assertNoThrow = [](uint32_t importanceGrouping, uint32_t maxRollbackBlocks) {
			auto config = CreateCatapultConfiguration(importanceGrouping, maxRollbackBlocks);
			EXPECT_NO_THROW(ValidateConfiguration(config)) << "IG " << importanceGrouping << ", MRB " << maxRollbackBlocks;
		};

		auto assertThrow = [](uint32_t importanceGrouping, uint32_t maxRollbackBlocks) {
			auto config = CreateCatapultConfiguration(importanceGrouping, maxRollbackBlocks);
			EXPECT_THROW(ValidateConfiguration(config), utils::property_malformed_error)
					<< "IG " << importanceGrouping << ", MRB " << maxRollbackBlocks;
		};

		// Act + Assert:
		// - no exceptions
		assertNoThrow(181, 360); // 2 * IG > MRB
		assertNoThrow(400, 360); // IG > MRB

		// - exceptions
		assertThrow(0, 360); // 0 IG
		assertThrow(180, 360); // 2 * IG == MRB
		assertThrow(179, 360); // 2 * IG < MRB
	}

	// endregion

	// region harvest beneficiary percentage validation

	namespace {
		auto CreateCatapultConfiguration(uint8_t harvestBeneficiaryPercentage) {
			auto mutableConfig = CreateMutableCatapultConfiguration();
			mutableConfig.BlockChain.HarvestBeneficiaryPercentage = harvestBeneficiaryPercentage;
			return mutableConfig.ToConst();
		}
	}

	TEST(TEST_CLASS, HarvestBeneficiaryPercentageIsValidated) {
		// Arrange:
		auto assertNoThrow = [](uint8_t harvestBeneficiaryPercentage) {
			auto config = CreateCatapultConfiguration(harvestBeneficiaryPercentage);
			EXPECT_NO_THROW(ValidateConfiguration(config)) << "HBP " << harvestBeneficiaryPercentage;
		};

		auto assertThrow = [](uint8_t harvestBeneficiaryPercentage) {
			auto config = CreateCatapultConfiguration(harvestBeneficiaryPercentage);
			EXPECT_THROW(ValidateConfiguration(config), utils::property_malformed_error) << "HBP " << harvestBeneficiaryPercentage;
		};

		// Act + Assert:
		// - no exceptions
		assertNoThrow(0);
		assertNoThrow(1);
		assertNoThrow(57);
		assertNoThrow(99);
		assertNoThrow(100);

		// - exceptions
		assertThrow(101);
		assertThrow(156);
		assertThrow(255);
	}

	// endregion

	// region total chain currency validation

	namespace {
		struct InflationEntry {
			catapult::Height Height;
			catapult::Amount Amount;
		};

		namespace {
			auto CreateCatapultConfiguration(uint64_t initialCurrencyAtomicUnits, const std::vector<InflationEntry>& inflationEntries) {
				auto mutableConfig = CreateMutableCatapultConfiguration();
				mutableConfig.BlockChain.InitialCurrencyAtomicUnits = Amount(initialCurrencyAtomicUnits);

				model::InflationCalculator calculator;
				for (const auto& inflationEntry : inflationEntries)
					calculator.add(inflationEntry.Height, inflationEntry.Amount);

				// Sanity:
				EXPECT_EQ(inflationEntries.size(), calculator.size());

				mutableConfig.Inflation.InflationCalculator = std::move(calculator);
				return mutableConfig.ToConst();
			}
		}
	}

	TEST(TEST_CLASS, SuccessfulValidationWhenConfigIsValid) {
		// Arrange: MaxMosaicAtomicUnits is 1000
		auto config = CreateCatapultConfiguration(123, { { Height(1), Amount(1) }, { Height(234), Amount(0) } });

		// Act:
		EXPECT_NO_THROW(ValidateConfiguration(config));
	}

	TEST(TEST_CLASS, InitialTotalChainAmountMustNotExceedMaxMosaicAtomicUnits) {
		// Act + Assert: MaxMosaicAtomicUnits is 1000
		auto inflationEntries = std::vector<InflationEntry>();
		EXPECT_THROW(ValidateConfiguration(CreateCatapultConfiguration(1001, inflationEntries)), utils::property_malformed_error);
		EXPECT_THROW(ValidateConfiguration(CreateCatapultConfiguration(5000, inflationEntries)), utils::property_malformed_error);
		EXPECT_THROW(ValidateConfiguration(CreateCatapultConfiguration(1234567890, inflationEntries)), utils::property_malformed_error);
	}

	TEST(TEST_CLASS, InitialTotalCurrenyPlusInflationMustNotExceedMaxMosaicAtomicUnits) {
		// Arrange: MaxMosaicAtomicUnits is 1000
		auto config = CreateCatapultConfiguration(600, { { Height(1), Amount(1) }, { Height(500), Amount(0) } });

		// Act:
		EXPECT_THROW(ValidateConfiguration(config), utils::property_malformed_error);
	}

	TEST(TEST_CLASS, InitialTotalCurrenyPlusInflationMustNotOverflow) {
		// Arrange:
		auto numBlocks = std::numeric_limits<uint64_t>::max();
		auto config = CreateCatapultConfiguration(2, { { Height(1), Amount(1) }, { Height(numBlocks), Amount(0) } });

		// Act:
		EXPECT_THROW(ValidateConfiguration(config), utils::property_malformed_error);
	}

	TEST(TEST_CLASS, TerminalInflationMustBeZero) {
		// Arrange:
		auto config = CreateCatapultConfiguration(0, { { Height(3), Amount(5) }, { Height(10), Amount(2) } });

		// Act:
		EXPECT_THROW(ValidateConfiguration(config), utils::property_malformed_error);
	}

	// endregion
}}
