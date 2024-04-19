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

#include "catapult/model/NetworkConfiguration.h"
#include "catapult/crypto/KeyUtils.h"
#include "catapult/utils/ConfigurationUtils.h"
#include "tests/test/nodeps/ConfigurationTestUtils.h"

namespace catapult { namespace model {

#define TEST_CLASS NetworkConfigurationTests

	// region loading

	namespace {
		constexpr auto Nemesis_Public_Key = "C738E237C98760FA72726BA13DC2A1E3C13FA67DE26AF09742E972EE4EE45E1C";

		struct NetworkConfigurationTraits {
			using ConfigurationType = NetworkConfiguration;

			static utils::ConfigurationBag::ValuesContainer CreateProperties() {
				return {
					{
						"network",
						{
							{ "publicKey", Nemesis_Public_Key },
						}
					},
					{
						"chain",
						{
							{ "blockGenerationTargetTime", "10m" },
							{ "blockTimeSmoothingFactor", "765" },

							{ "greedDelta", "0.5" },
							{ "greedExponent", "2" },

							{ "importanceGrouping", "444" },
							{ "maxRollbackBlocks", "720" },
							{ "maxDifficultyBlocks", "15" },

							{ "maxTransactionLifetime", "30m" },
							{ "maxBlockFutureTime", "21m" },

							{ "maxMosaicAtomicUnits", "66'000'000'000" },

							{ "totalChainImportance", "88'000'000'000" },
							{ "minHarvesterBalance", "4'000'000'000" },
							{ "harvestBeneficiaryPercentage", "56" },

							{ "blockPruneInterval", "432" },
							{ "maxTransactionsPerBlock", "120" },

							{ "enableUnconfirmedTransactionMinFeeValidation", "true" },

							{ "enableUndoBlock", "true" },
							{ "enableBlockSync", "true" },

							{ "enableWeightedVoting", "false" },
							{ "committeeSize", "21" },
							{ "committeeApproval", "0.67" },
							{ "committeePhaseTime", "5s" },
							{ "minCommitteePhaseTime", "1s" },
							{ "maxCommitteePhaseTime", "1m" },
							{ "committeeSilenceInterval", "100ms" },
							{ "committeeRequestInterval", "500ms" },
							{ "committeeChainHeightRequestInterval", "30s" },
							{ "committeeTimeAdjustment", "1.1" },
							{ "committeeEndSyncApproval", "0.45" },
							{ "committeeBaseTotalImportance", "100" },
							{ "committeeNotRunningContribution", "0.5" },

							{ "dbrbRegistrationDuration", "24h" },
							{ "dbrbRegistrationGracePeriod", "1h" },

							{ "enableHarvesterExpiration", "true" },
							{ "enableRemovingDbrbProcessOnShutdown", "true" },

							{ "enableDbrbSharding", "true" },
							{ "dbrbShardSize", "6" },
						}
					},
					{
						"bootstrap.harvesters",
						{
							{ "E8D4B7BEB2A531ECA8CC7FD93F79A4C828C24BE33F99CF7C5609FF5CE14605F4", "E92978122F00698856910664C480E8F3C2FDF0A733F42970FBD58A5145BD6F21, A384FBAAADBFF0405DDA0212D8A6C85F9164A08C24AFD15425927BCB274A45D4" },
						}
					},
					{
						"plugin:alpha",
						{
							{ "foo", "alpha" }
						}
					},
					{
						"plugin:beta",
						{
							{ "bar", "11" },
							{ "baz", "zeta" }
						}
					}
				};
			}

			static bool SupportsUnknownProperties() {
				return true;
			}

			static bool IsSectionOptional(const std::string& section) {
				return "network" != section && "chain" != section;
			}

			static bool IsPropertyOptional(const std::string& name) {
				return std::set<std::string>{
					"enableUnconfirmedTransactionMinFeeValidation",
					"enableUndoBlock",
					"enableBlockSync",
					"enableWeightedVoting",
					"committeeSize",
					"committeeApproval",
					"committeePhaseTime",
					"minCommitteePhaseTime",
					"maxCommitteePhaseTime",
					"committeeSilenceInterval",
					"committeeRequestInterval",
					"committeeChainHeightRequestInterval",
					"committeeTimeAdjustment",
					"committeeEndSyncApproval",
					"committeeBaseTotalImportance",
					"committeeNotRunningContribution",

					"dbrbRegistrationDuration",
					"dbrbRegistrationGracePeriod",
					"enableHarvesterExpiration",
					"enableRemovingDbrbProcessOnShutdown",
					"enableDbrbSharding",
					"dbrbShardSize"
				}.count(name);
			}

			static void AssertZero(const NetworkConfiguration& config) {
				// Assert:
				EXPECT_EQ(Key(), config.Info.PublicKey);

				EXPECT_EQ(utils::TimeSpan::FromMinutes(0), config.BlockGenerationTargetTime);
				EXPECT_EQ(0u, config.BlockTimeSmoothingFactor);

				EXPECT_EQ(0.0, config.GreedDelta);
				EXPECT_EQ(0.0, config.GreedExponent);

				EXPECT_EQ(0u, config.ImportanceGrouping);
				EXPECT_EQ(0u, config.MaxRollbackBlocks);
				EXPECT_EQ(0u, config.MaxDifficultyBlocks);

				EXPECT_EQ(utils::TimeSpan::FromMinutes(0), config.MaxTransactionLifetime);
				EXPECT_EQ(utils::TimeSpan::FromMinutes(0), config.MaxBlockFutureTime);

				EXPECT_EQ(Amount(0), config.MaxMosaicAtomicUnits);

				EXPECT_EQ(Importance(0), config.TotalChainImportance);
				EXPECT_EQ(Amount(0), config.MinHarvesterBalance);
				EXPECT_EQ(0u, config.HarvestBeneficiaryPercentage);

				EXPECT_EQ(0u, config.BlockPruneInterval);
				EXPECT_EQ(0u, config.MaxTransactionsPerBlock);

				EXPECT_EQ(false, config.EnableUnconfirmedTransactionMinFeeValidation);

				EXPECT_EQ(0u, config.CommitteeSize);
				EXPECT_EQ(0.0, config.CommitteeApproval);
				EXPECT_EQ(utils::TimeSpan::FromMinutes(0), config.CommitteePhaseTime);
				EXPECT_EQ(utils::TimeSpan::FromMinutes(0), config.MinCommitteePhaseTime);
				EXPECT_EQ(utils::TimeSpan::FromMinutes(0), config.MaxCommitteePhaseTime);
				EXPECT_EQ(utils::TimeSpan::FromMinutes(0), config.CommitteeSilenceInterval);
				EXPECT_EQ(utils::TimeSpan::FromMinutes(0), config.CommitteeRequestInterval);
				EXPECT_EQ(utils::TimeSpan::FromMinutes(0), config.CommitteeChainHeightRequestInterval);
				EXPECT_EQ(0.0, config.CommitteeTimeAdjustment);
				EXPECT_EQ(0.0, config.CommitteeEndSyncApproval);
				EXPECT_EQ(0, config.CommitteeBaseTotalImportance);
				EXPECT_EQ(0.0, config.CommitteeNotRunningContribution);

				EXPECT_EQ(utils::TimeSpan::FromHours(0), config.DbrbRegistrationDuration);
				EXPECT_EQ(utils::TimeSpan::FromHours(0), config.DbrbRegistrationGracePeriod);

				EXPECT_EQ(false, config.EnableHarvesterExpiration);
				EXPECT_EQ(false, config.EnableRemovingDbrbProcessOnShutdown);

				EXPECT_EQ(false, config.EnableDbrbSharding);
				EXPECT_EQ(0, config.DbrbShardSize);

				EXPECT_TRUE(config.Plugins.empty());
			}

			static void AssertCustom(const NetworkConfiguration& config) {
				// Assert: notice that ParseKey also works for Hash256 because it is the same type as Key
				EXPECT_EQ(crypto::ParseKey(Nemesis_Public_Key), config.Info.PublicKey);

				EXPECT_EQ(utils::TimeSpan::FromMinutes(10), config.BlockGenerationTargetTime);
				EXPECT_EQ(765u, config.BlockTimeSmoothingFactor);

				EXPECT_EQ(0.5, config.GreedDelta);
				EXPECT_EQ(2.0, config.GreedExponent);

				EXPECT_EQ(444u, config.ImportanceGrouping);
				EXPECT_EQ(720u, config.MaxRollbackBlocks);
				EXPECT_EQ(15u, config.MaxDifficultyBlocks);

				EXPECT_EQ(utils::TimeSpan::FromMinutes(30), config.MaxTransactionLifetime);
				EXPECT_EQ(utils::TimeSpan::FromMinutes(21), config.MaxBlockFutureTime);

				EXPECT_EQ(Amount(66'000'000'000), config.MaxMosaicAtomicUnits);

				EXPECT_EQ(Importance(88'000'000'000), config.TotalChainImportance);
				EXPECT_EQ(Amount(4'000'000'000), config.MinHarvesterBalance);
				EXPECT_EQ(56u, config.HarvestBeneficiaryPercentage);

				EXPECT_EQ(432u, config.BlockPruneInterval);
				EXPECT_EQ(120u, config.MaxTransactionsPerBlock);

				EXPECT_EQ(true, config.EnableUnconfirmedTransactionMinFeeValidation);

				EXPECT_EQ(21u, config.CommitteeSize);
				EXPECT_EQ(0.67, config.CommitteeApproval);
				EXPECT_EQ(utils::TimeSpan::FromSeconds(5), config.CommitteePhaseTime);
				EXPECT_EQ(utils::TimeSpan::FromSeconds(1), config.MinCommitteePhaseTime);
				EXPECT_EQ(utils::TimeSpan::FromMinutes(1), config.MaxCommitteePhaseTime);
				EXPECT_EQ(utils::TimeSpan::FromMilliseconds(100), config.CommitteeSilenceInterval);
				EXPECT_EQ(utils::TimeSpan::FromMilliseconds(500), config.CommitteeRequestInterval);
				EXPECT_EQ(utils::TimeSpan::FromSeconds(30), config.CommitteeChainHeightRequestInterval);
				EXPECT_EQ(1.1, config.CommitteeTimeAdjustment);
				EXPECT_EQ(0.45, config.CommitteeEndSyncApproval);
				EXPECT_EQ(100, config.CommitteeBaseTotalImportance);
				EXPECT_EQ(0.5, config.CommitteeNotRunningContribution);

				EXPECT_EQ(utils::TimeSpan::FromHours(24), config.DbrbRegistrationDuration);
				EXPECT_EQ(utils::TimeSpan::FromHours(1), config.DbrbRegistrationGracePeriod);

				EXPECT_EQ(false, config.EnableHarvesterExpiration);
				EXPECT_EQ(false, config.EnableRemovingDbrbProcessOnShutdown);

				EXPECT_EQ(false, config.EnableDbrbSharding);
				EXPECT_EQ(6, config.DbrbShardSize);

				EXPECT_EQ(2u, config.Plugins.size());
				const auto& pluginAlphaBag = config.Plugins.find("alpha")->second;
				EXPECT_EQ(1u, pluginAlphaBag.size());
				EXPECT_EQ("alpha", pluginAlphaBag.get<std::string>({ "", "foo" }));

				const auto& pluginBetaBag = config.Plugins.find("beta")->second;
				EXPECT_EQ(2u, pluginBetaBag.size());
				EXPECT_EQ(11u, pluginBetaBag.get<uint64_t>({ "", "bar" }));
				EXPECT_EQ("zeta", pluginBetaBag.get<std::string>({ "", "baz" }));
			}
		};
	}

	DEFINE_CONFIGURATION_TESTS(TEST_CLASS, Network)

	namespace {
		void AssertCannotLoadWithSection(const std::string& section) {
			// Arrange:
			using Traits = NetworkConfigurationTraits;
			CATAPULT_LOG(debug) << "attempting to load configuration with extra section '" << section << "'";

			// - create the properties and add the desired section
			auto properties = Traits::CreateProperties();
			properties.insert({ section, { { "foo", "1234" } } });

			// Act + Assert: the load failed
			EXPECT_THROW(Traits::ConfigurationType::LoadFromBag(std::move(properties)), catapult_invalid_argument);
		}
	}

	TEST(TEST_CLASS, ParseFailsWhenPluginSectionNameContainsInvalidPluginName) {
		// Arrange:
		auto invalidPluginNames = { " ", "$$$", "some long name", "Zeta", "ZETA" };
		for (const auto& pluginName : invalidPluginNames)
			AssertCannotLoadWithSection(std::string("plugin:") + pluginName);
	}

	TEST(TEST_CLASS, ParseSucceedsWhenPluginSectionNameContainsValidPluginName) {
		// Arrange:
		using Traits = NetworkConfigurationTraits;
		auto validPluginNames = { "a", "j", "z", ".", "zeta", "some.long.name" };
		for (const auto& pluginName : validPluginNames) {
			CATAPULT_LOG(debug) << "attempting to load configuration with plugin named " << pluginName;

			// Act: create the properties and add the desired section
			auto properties = Traits::CreateProperties();
			properties.insert({ std::string("plugin:") + pluginName, { { "foo", "1234" } } });
			auto config = Traits::ConfigurationType::LoadFromBag(std::move(properties));

			// Assert:
			EXPECT_EQ(3u, config.Plugins.size());

			const auto& pluginBag = config.Plugins.find(pluginName)->second;
			EXPECT_EQ(1u, pluginBag.size());
			EXPECT_EQ(1234u, pluginBag.get<uint64_t>({ "", "foo" }));
		}
	}

	// endregion

	// region calculated properties

	namespace {
		const uint64_t One_Hour_Ms = []() { return utils::TimeSpan::FromHours(1).millis(); }();

		utils::TimeSpan TimeSpanFromMillis(uint64_t millis) {
			return utils::TimeSpan::FromMilliseconds(millis);
		}
	}

	TEST(TEST_CLASS, CanCalculateDependentSettingsFromCustomNetworkConfiguration) {
		// Arrange:
		auto config = NetworkConfiguration::Uninitialized();
		config.BlockGenerationTargetTime = TimeSpanFromMillis(30'001);
		config.MaxTransactionLifetime = TimeSpanFromMillis(One_Hour_Ms - 1);
		config.MaxRollbackBlocks = 600;
		config.MaxDifficultyBlocks = 45;

		// Act + Assert:
		EXPECT_EQ(TimeSpanFromMillis(30'001 * 600), CalculateFullRollbackDuration(config));
		EXPECT_EQ(TimeSpanFromMillis(30'001 * 150), CalculateRollbackVariabilityBufferDuration(config));
		EXPECT_EQ(TimeSpanFromMillis(30'001 * (600 + 150)), CalculateTransactionCacheDuration(config));
		EXPECT_EQ(645u, CalculateDifficultyHistorySize(config));
	}

	TEST(TEST_CLASS, TransactionCacheDurationIncludesBufferTimeOfAtLeastOneHour) {
		// Arrange:
		auto config = NetworkConfiguration::Uninitialized();
		config.BlockGenerationTargetTime = utils::TimeSpan::FromSeconds(15);
		config.MaxTransactionLifetime = utils::TimeSpan::FromHours(2);
		config.MaxRollbackBlocks = 20;
		config.MaxDifficultyBlocks = 45;

		// Act + Assert:
		EXPECT_EQ(TimeSpanFromMillis(15'000 * 20), CalculateFullRollbackDuration(config));
		EXPECT_EQ(utils::TimeSpan::FromHours(1), CalculateRollbackVariabilityBufferDuration(config));
		EXPECT_EQ(TimeSpanFromMillis(15'000 * 20 + One_Hour_Ms), CalculateTransactionCacheDuration(config));
		EXPECT_EQ(65u, CalculateDifficultyHistorySize(config));
	}

	// endregion

	// region LoadPluginConfiguration

	namespace {
		struct BetaConfiguration {
		public:
			static constexpr size_t Id = 0;
			static constexpr auto Name = "beta";
		public:
			uint64_t Bar;
			std::string Baz;

		public:
			static BetaConfiguration LoadFromBag(const utils::ConfigurationBag& bag) {
				BetaConfiguration config;
				utils::LoadIniProperty(bag, "", "Bar", config.Bar);
				utils::LoadIniProperty(bag, "", "Baz", config.Baz);
				utils::VerifyBagSizeLte(bag, 2);
				return config;
			}

			static BetaConfiguration Uninitialized() {
				return BetaConfiguration();
			}
		};

		struct GammaConfiguration {
		public:
			static constexpr auto Name = "gamma";
		public:
			uint64_t Foo;

			static GammaConfiguration LoadFromBag(const utils::ConfigurationBag& bag) {
				GammaConfiguration config;
				utils::LoadIniProperty(bag, "", "Foo", config.Foo);
				utils::VerifyBagSizeLte(bag, 1);
				return config;
			}

			static GammaConfiguration Uninitialized() {
				return GammaConfiguration();
			}
		};
	}

	TEST(TEST_CLASS, LoadPluginConfigurationSucceedsWhenPluginConfigurationIsPresent) {
		// Arrange:
		using Traits = NetworkConfigurationTraits;
		auto config = Traits::ConfigurationType::LoadFromBag(Traits::CreateProperties());

		// Act:
		auto betaConfig = LoadPluginConfiguration<BetaConfiguration>(config);

		// Assert:
		EXPECT_EQ(11u, betaConfig.Bar);
		EXPECT_EQ("zeta", betaConfig.Baz);
	}

	TEST(TEST_CLASS, LoadPluginConfigurationDefaultWhenPluginConfigurationIsNotPresent) {
		// Arrange:
		using Traits = NetworkConfigurationTraits;
		auto config = Traits::ConfigurationType::LoadFromBag(Traits::CreateProperties());

		// Act + Assert:
		auto loadedConfig = LoadPluginConfiguration<GammaConfiguration>(config);
		auto expectedConfig = GammaConfiguration::Uninitialized();
		EXPECT_EQ(expectedConfig.Foo, loadedConfig.Foo);
	}

	// endregion
}}
