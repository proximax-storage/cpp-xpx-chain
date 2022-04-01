/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/config/SdaExchangeConfiguration.h"
#include "catapult/crypto/KeyUtils.h"
#include "tests/test/nodeps/ConfigurationTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace config {

	namespace {
		struct SdaExchangeConfigurationTraits {
			using ConfigurationType = SdaExchangeConfiguration;

			static utils::ConfigurationBag::ValuesContainer CreateProperties() {
				return {
					{
						"",
						{
							{ "enabled", "true" },
							{ "maxOfferDuration", "57600" },
							{ "longOfferKey", "CFC31B3080B36BC3D59DF4AB936AC72F4DC15CE3C3E1B1EC5EA41415A4C33FEE" },
                            { "sortPolicy", "1"},
						}
					}
				};
			}

			static bool SupportsUnknownProperties() {
				return false;
			}

			static bool IsPropertyOptional(const std::string&) {
				return false;
			}

			static bool IsSectionOptional(const std::string&) {
				return false;
			}

			static void AssertZero(const SdaExchangeConfiguration& config) {
				// Assert:
				EXPECT_EQ(false, config.Enabled);
				EXPECT_EQ(BlockDuration(0), config.maxOfferDuration);
				EXPECT_EQ(Key(), config.longOfferKey);
                EXPECT_EQ(0, config.SortPolicy);
			}

			static void AssertCustom(const SdaExchangeConfiguration& config) {
				// Assert:
				EXPECT_EQ(true, config.Enabled);
				EXPECT_EQ(BlockDuration(57600), config.MaxOfferDuration);
				EXPECT_EQ(crypto::ParseKey("CFC31B3080B36BC3D59DF4AB936AC72F4DC15CE3C3E1B1EC5EA41415A4C33FEE"), config.LongOfferKey);
                EXPECT_EQ(1, config.SortPolicy);
			}
		};
	}

	DEFINE_CONFIGURATION_TESTS(SdaExchangeConfigurationTests, SdaExchange)
}}
