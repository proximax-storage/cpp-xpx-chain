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
#include "tests/test/other/MutableBlockchainConfiguration.h"
#include "tests/TestHarness.h"

namespace catapult { namespace config {

#define TEST_CLASS ValidateConfigurationTests

	namespace {
		// the key is invalid because it contains a non hex char ('G')
		const char* Invalid_Private_Key = "3485D98EFD7EB07ABAFCFD1A157D89DE2G96A95E780813C0258AF3F5F84ED8CB";
		const char* Valid_Private_Key = "3485D98EFD7EB07ABAFCFD1A157D89DE2796A95E780813C0258AF3F5F84ED8CB";

		auto CreateMutableBlockchainConfiguration() {
			test::MutableBlockchainConfiguration config;
			config.Immutable.NetworkIdentifier = model::NetworkIdentifier::Mijin_Test;

			auto& networkConfig = config.Network;
			networkConfig.ImportanceGrouping = 1;
			networkConfig.MaxMosaicAtomicUnits = Amount(1000);
			networkConfig.MaxRollbackBlocks = 5u;
			networkConfig.ImportanceGrouping = 10u;
			networkConfig.HarvestBeneficiaryPercentage = 10;

			auto& nodeConfig = config.Node;
			nodeConfig.FeeInterest = 1;
			nodeConfig.FeeInterestDenominator = 1;

			auto& userConfig = config.User;
			userConfig.BootKey = Valid_Private_Key;

			return config;
		}
	}

	// region boot key validation

	namespace {
		void AssertInvalidBootKey(const std::string& bootKey) {
			// Arrange:
			auto mutableConfig = CreateMutableBlockchainConfiguration();
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
		auto CreateBlockchainConfiguration(uint32_t importanceGrouping, uint32_t maxRollbackBlocks) {
			auto mutableConfig = CreateMutableBlockchainConfiguration();
			mutableConfig.Network.ImportanceGrouping = importanceGrouping;
			mutableConfig.Network.MaxRollbackBlocks = maxRollbackBlocks;
			return mutableConfig.ToConst();
		}
	}

	TEST(TEST_CLASS, ImportanceGroupingIsValidatedAgainstMaxRollbackBlocks) {
		// Arrange:
		auto assertNoThrow = [](uint32_t importanceGrouping, uint32_t maxRollbackBlocks) {
			auto config = CreateBlockchainConfiguration(importanceGrouping, maxRollbackBlocks);
			EXPECT_NO_THROW(ValidateConfiguration(config)) << "IG " << importanceGrouping << ", MRB " << maxRollbackBlocks;
		};

		auto assertThrow = [](uint32_t importanceGrouping, uint32_t maxRollbackBlocks) {
			auto config = CreateBlockchainConfiguration(importanceGrouping, maxRollbackBlocks);
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
		auto CreateBlockchainConfiguration(uint8_t harvestBeneficiaryPercentage) {
			auto mutableConfig = CreateMutableBlockchainConfiguration();
			mutableConfig.Network.HarvestBeneficiaryPercentage = harvestBeneficiaryPercentage;
			return mutableConfig.ToConst();
		}
	}

	TEST(TEST_CLASS, HarvestBeneficiaryPercentageIsValidated) {
		// Arrange:
		auto assertNoThrow = [](uint8_t harvestBeneficiaryPercentage) {
			auto config = CreateBlockchainConfiguration(harvestBeneficiaryPercentage);
			EXPECT_NO_THROW(ValidateConfiguration(config)) << "HBP " << harvestBeneficiaryPercentage;
		};

		auto assertThrow = [](uint8_t harvestBeneficiaryPercentage) {
			auto config = CreateBlockchainConfiguration(harvestBeneficiaryPercentage);
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


}}
