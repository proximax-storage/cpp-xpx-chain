/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "catapult/config/ImmutableConfiguration.h"
#include "catapult/crypto/KeyUtils.h"
#include "catapult/utils/ConfigurationUtils.h"
#include "catapult/utils/HexParser.h"
#include "tests/test/nodeps/ConfigurationTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace config {

#define TEST_CLASS ImmutableConfigurationTests

	// region loading

	namespace {
		constexpr auto Nemesis_Generation_Hash = "CE076EF4ABFBC65B046987429E274EC31506D173E91BF102F16BEB7FB8176230";

		struct ImmutableConfigurationTraits {
			using ConfigurationType = ImmutableConfiguration;

			static utils::ConfigurationBag::ValuesContainer CreateProperties() {
				return {
					{
						"immutable",
						{
							{ "networkIdentifier", "public-test" },
							{ "generationHash", Nemesis_Generation_Hash },

							{ "shouldEnableVerifiableState", "true" },
							{ "shouldEnableVerifiableReceipts", "true" },

							{ "currencyMosaicId", "0x1234'AAAA" },
							{ "harvestingMosaicId", "0x9876'BBBB" },
							{ "storageMosaicId", "0x4321'AAAA" },
							{ "streamingMosaicId", "0x6789'BBBB" },
							{ "reviewMosaicId", "0x4321'CCCC" },
							{ "superContractMosaicId", "0x6789'DDDD" },
							{ "xarMosaicId", "0x4321'EEEE" },

							{ "initialCurrencyAtomicUnits", "77'000'000'000" },
						}
					},
				};
			}

			static bool IsSectionOptional(const std::string&) {
				return false;
			}

			static void AssertZero(const ImmutableConfiguration& config) {
				// Assert:
				EXPECT_EQ(model::NetworkIdentifier::Zero, config.NetworkIdentifier);
				EXPECT_EQ(GenerationHash(), config.GenerationHash);

				EXPECT_FALSE(config.ShouldEnableVerifiableState);
				EXPECT_FALSE(config.ShouldEnableVerifiableReceipts);

				EXPECT_EQ(MosaicId(), config.CurrencyMosaicId);
				EXPECT_EQ(MosaicId(), config.HarvestingMosaicId);
				EXPECT_EQ(MosaicId(), config.StorageMosaicId);
				EXPECT_EQ(MosaicId(), config.StreamingMosaicId);
				EXPECT_EQ(MosaicId(), config.ReviewMosaicId);
				EXPECT_EQ(MosaicId(), config.SuperContractMosaicId);
				EXPECT_EQ(MosaicId(), config.XarMosaicId);

				EXPECT_EQ(Amount(0), config.InitialCurrencyAtomicUnits);
			}

			static void AssertCustom(const ImmutableConfiguration& config) {
				// Assert: notice that ParseKey also works for Hash256 because it is the same type as Key
				EXPECT_EQ(model::NetworkIdentifier::Public_Test, config.NetworkIdentifier);
				EXPECT_EQ(utils::ParseByteArray<GenerationHash>(Nemesis_Generation_Hash), config.GenerationHash);

				EXPECT_TRUE(config.ShouldEnableVerifiableState);
				EXPECT_TRUE(config.ShouldEnableVerifiableReceipts);

				EXPECT_EQ(MosaicId(0x1234'AAAA), config.CurrencyMosaicId);
				EXPECT_EQ(MosaicId(0x9876'BBBB), config.HarvestingMosaicId);
				EXPECT_EQ(MosaicId(0x4321'AAAA), config.StorageMosaicId);
				EXPECT_EQ(MosaicId(0x6789'BBBB), config.StreamingMosaicId);
				EXPECT_EQ(MosaicId(0x4321'CCCC), config.ReviewMosaicId);
				EXPECT_EQ(MosaicId(0x6789'DDDD), config.SuperContractMosaicId);
				EXPECT_EQ(MosaicId(0x4321'EEEE), config.XarMosaicId);

				EXPECT_EQ(Amount(77'000'000'000), config.InitialCurrencyAtomicUnits);
			}
		};
	}

	DEFINE_CONFIGURATION_TESTS(TEST_CLASS, Immutable)

	TEST(TEST_CLASS, CannotLoadImmutableConfigurationWithInvalidNetwork) {
		// Arrange: set an unknown network in the container
		using Traits = ImmutableConfigurationTraits;
		auto container = Traits::CreateProperties();
		auto& networkProperties = container["immutable"];
		auto hasIdentifierKey = [](const auto& pair) { return "networkIdentifier" == pair.first; };
		std::find_if(networkProperties.begin(), networkProperties.end(), hasIdentifierKey)->second = "foonet";

		// Act + Assert:
		EXPECT_THROW(Traits::ConfigurationType::LoadFromBag(std::move(container)), utils::property_malformed_error);
	}

	// endregion

	// region calculated properties

	TEST(TEST_CLASS, CanGetUnresolvedCurrencyMosaicId) {
		// Arrange:
		auto config = ImmutableConfiguration::Uninitialized();
		config.CurrencyMosaicId = MosaicId(1234);

		// Act + Assert:
		EXPECT_EQ(UnresolvedMosaicId(1234), GetUnresolvedCurrencyMosaicId(config));
	}

	TEST(TEST_CLASS, CanGetUnresolvedStorageMosaicId) {
		// Arrange:
		auto config = ImmutableConfiguration::Uninitialized();
		config.StorageMosaicId = MosaicId(1234);

		// Act + Assert:
		EXPECT_EQ(UnresolvedMosaicId(1234), GetUnresolvedStorageMosaicId(config));
	}

	TEST(TEST_CLASS, CanGetUnresolvedStreamingMosaicId) {
		// Arrange:
		auto config = ImmutableConfiguration::Uninitialized();
		config.StreamingMosaicId = MosaicId(1234);

		// Act + Assert:
		EXPECT_EQ(UnresolvedMosaicId(1234), GetUnresolvedStreamingMosaicId(config));
	}

	TEST(TEST_CLASS, CanGetUnresolvedReviewMosaicId) {
		// Arrange:
		auto config = ImmutableConfiguration::Uninitialized();
		config.ReviewMosaicId = MosaicId(1234);

		// Act + Assert:
		EXPECT_EQ(UnresolvedMosaicId(1234), GetUnresolvedReviewMosaicId(config));
	}

	TEST(TEST_CLASS, CanGetUnresolvedSuperContractMosaicId) {
		// Arrange:
		auto config = ImmutableConfiguration::Uninitialized();
		config.SuperContractMosaicId = MosaicId(1234);

		// Act + Assert:
		EXPECT_EQ(UnresolvedMosaicId(1234), GetUnresolvedSuperContractMosaicId(config));
	}

	TEST(TEST_CLASS, CanGetUnresolvedXarMosaicId) {
		// Arrange:
		auto config = ImmutableConfiguration::Uninitialized();
		config.XarMosaicId = MosaicId(1234);

		// Act + Assert:
		EXPECT_EQ(UnresolvedMosaicId(1234), GetUnresolvedXarMosaicId(config));
	}

	// endregion

	// region network identifier parsing

	TEST(TEST_CLASS, CanParseValidNetworkValue) {
		// Arrange:
		auto assertSuccessfulParse = [](const auto& input, const auto& expectedParsedValue) {
			test::AssertParse(input, expectedParsedValue, [](const auto& str, auto& parsedValue) {
				return TryParseValue(str, parsedValue);
			});
		};

		// Assert:
		assertSuccessfulParse("mijin", model::NetworkIdentifier::Mijin);
		assertSuccessfulParse("mijin-test", model::NetworkIdentifier::Mijin_Test);
		assertSuccessfulParse("private", model::NetworkIdentifier::Private);
		assertSuccessfulParse("private-test", model::NetworkIdentifier::Private_Test);
		assertSuccessfulParse("public", model::NetworkIdentifier::Public);
		assertSuccessfulParse("public-test", model::NetworkIdentifier::Public_Test);

		assertSuccessfulParse("0", static_cast<model::NetworkIdentifier>(0));
		assertSuccessfulParse("17", static_cast<model::NetworkIdentifier>(17));
		assertSuccessfulParse("255", static_cast<model::NetworkIdentifier>(255));
	}

	TEST(TEST_CLASS, CannotParseInvalidNetworkValue) {
		// Assert:
		test::AssertEnumParseFailure("mijin", model::NetworkIdentifier::Public, [](const auto& str, auto& parsedValue) {
			return TryParseValue(str, parsedValue);
		});
		test::AssertFailedParse("256", model::NetworkIdentifier::Public, [](const auto& str, auto& parsedValue) {
			return TryParseValue(str, parsedValue);
		});
	}

	// endregion
}}
