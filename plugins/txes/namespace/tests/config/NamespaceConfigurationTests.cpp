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

#include "src/config/NamespaceConfiguration.h"
#include "catapult/crypto/KeyUtils.h"
#include "tests/test/nodeps/ConfigurationTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace config {

	namespace {
		constexpr auto Namespace_Rental_Fee_Sink_Public_Key = "75D8BB873DA8F5CCA741435DE76A46AAA2840803EBBBB0E931195B048D77F88C";

		struct NamespaceConfigurationTraits {
			using ConfigurationType = NamespaceConfiguration;

			static utils::ConfigurationBag::ValuesContainer CreateProperties() {
				return {
					{
						"",
						{
							{ "maxNameSize", "123" },
							{ "maxNamespaceDuration", "234h" },
							{ "namespaceGracePeriodDuration", "20d" },
							{ "reservedRootNamespaceNames", "alpha,omega" },

							{ "namespaceRentalFeeSinkPublicKey", Namespace_Rental_Fee_Sink_Public_Key },
							{ "rootNamespaceRentalFeePerBlock", "78" },
							{ "childNamespaceRentalFee", "11223322" },

							{ "maxChildNamespaces", "1234" }
						}
					}
				};
			}

			static bool IsSectionOptional(const std::string&) {
				return false;
			}

			static void AssertZero(const NamespaceConfiguration& config) {
				// Assert:
				EXPECT_EQ(0u, config.MaxNameSize);
				EXPECT_EQ(utils::BlockSpan(), config.MaxNamespaceDuration);
				EXPECT_EQ(utils::BlockSpan(), config.NamespaceGracePeriodDuration);
				EXPECT_EQ(std::unordered_set<std::string>(), config.ReservedRootNamespaceNames);

				EXPECT_EQ(Key(), config.NamespaceRentalFeeSinkPublicKey);
				EXPECT_EQ(Amount(), config.RootNamespaceRentalFeePerBlock);
				EXPECT_EQ(Amount(), config.ChildNamespaceRentalFee);

				EXPECT_EQ(0u, config.MaxChildNamespaces);
			}

			static void AssertCustom(const NamespaceConfiguration& config) {
				// Assert:
				EXPECT_EQ(123u, config.MaxNameSize);
				EXPECT_EQ(utils::BlockSpan::FromHours(234), config.MaxNamespaceDuration);
				EXPECT_EQ(utils::BlockSpan::FromDays(20), config.NamespaceGracePeriodDuration);
				EXPECT_EQ((std::unordered_set<std::string>{ "alpha", "omega" }), config.ReservedRootNamespaceNames);

				EXPECT_EQ(crypto::ParseKey(Namespace_Rental_Fee_Sink_Public_Key), config.NamespaceRentalFeeSinkPublicKey);
				EXPECT_EQ(Amount(78), config.RootNamespaceRentalFeePerBlock);
				EXPECT_EQ(Amount(11223322), config.ChildNamespaceRentalFee);

				EXPECT_EQ(1234u, config.MaxChildNamespaces);
			}
		};
	}

	DEFINE_CONFIGURATION_TESTS(NamespaceConfigurationTests, Namespace)
}}
