/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/validators/Validators.h"
#include "tests/test/cache/CacheTestUtils.h"
#include "tests/TestHarness.h"
#include "tests/test/core/mocks/MockBlockchainConfigurationHolder.h"
#include "tests/test/plugins/ValidatorTestUtils.h"
#include "tests/test/core/NotificationTestUtils.h"
#include "src/config/NetworkConfigConfiguration.h"
#include "tests/test/other/MutableBlockchainConfiguration.h"
#include "catapult/model/PluginConfiguration.h"

namespace catapult { namespace validators {

#define TEST_CLASS PluginAvailableValidatorTests

	DEFINE_COMMON_VALIDATOR_TESTS(PluginAvailable, plugins::PluginManager(config::CreateMockConfigurationHolder(), plugins::StorageConfiguration()))

	namespace {
		/// Network config plugin configuration settings.
		struct TestPluginConfiguration : public model::PluginConfiguration {
		public:
			static constexpr size_t Id = 1;
			static constexpr auto Name = "catapult.plugins.testplugin";

		public:
			/// Creates an uninitialized network config plugin configuration.
			static TestPluginConfiguration Uninitialized(){
				return TestPluginConfiguration();
			}

			/// Loads an network config plugin configuration from \a bag.
			static TestPluginConfiguration LoadFromBag(const utils::ConfigurationBag& bag) {
				return TestPluginConfiguration();
			}
		};

		template<typename TConfigPreparer>
		std::shared_ptr<plugins::PluginManager> CreatePluginManager(TConfigPreparer preparer) {
			test::MutableBlockchainConfiguration config;
			config.Immutable.InitialCurrencyAtomicUnits = Amount(100);
			auto pluginConfig = config::NetworkConfigConfiguration::Uninitialized();
			config.Network.SetPluginConfiguration(pluginConfig);
			config.Network.Plugins.emplace(PLUGIN_NAME(config), utils::ConfigurationBag({}));
			auto oldConfig = config.ToConst();
			test::MutableBlockchainConfiguration newConfig;
			newConfig.Network.SetPluginConfiguration(pluginConfig);
			newConfig.Network.Plugins.emplace(PLUGIN_NAME(config), utils::ConfigurationBag({}));
			// Take care as this will become an invalid pointer.
			newConfig.PreviousConfiguration = &oldConfig;
			preparer(config, newConfig);
			auto pConfigHolder = config::CreateMockConfigurationHolderWithNemesisConfig(oldConfig);
			std::dynamic_pointer_cast<config::MockBlockchainConfigurationHolder>(pConfigHolder)->InsertConfig(newConfig.ActivationHeight, newConfig.Network, newConfig.SupportedEntityVersions);
			return std::make_shared<plugins::PluginManager>(pConfigHolder, plugins::StorageConfiguration());
		}

		template<typename TConfigPreparer>
		void AssertValidationResult(const ValidationResult& expectedResult, TConfigPreparer preparer) {
			// Arrange:
			auto notification = test::CreateBlockNotification();
			auto cache = test::CreateEmptyCatapultCache();
			auto cacheView = cache.createView();
			auto readOnlyCache = cacheView.toReadOnly();
			auto manager = CreatePluginManager(preparer);
			auto context = test::CreateValidatorContext(manager->configHolder()->Config(Height(10)), Height(10), readOnlyCache);
			auto pValidator = CreatePluginAvailableValidator(*manager);

			// Act:
			auto result = test::ValidateNotification(*pValidator, notification, context);

			// Assert:
			EXPECT_EQ(expectedResult, result);
		}
	}

	TEST(TEST_CLASS, SuccessWhenRequiredPluginsAreEnabled) {
		// Assert:
		AssertValidationResult(ValidationResult::Success, [](test::MutableBlockchainConfiguration& config, test::MutableBlockchainConfiguration& newConfig){
			newConfig.ActivationHeight = Height(10);
		});
	}

	TEST(TEST_CLASS, FailureWhenRequiredPluginsAreUnloaded) {
		// Assert:
		AssertValidationResult(Failure_NetworkConfig_Required_Plugins_Not_Matching, [](test::MutableBlockchainConfiguration& config, test::MutableBlockchainConfiguration& newConfig){
			newConfig.ActivationHeight = Height(10);
			auto testPluginConfig =  TestPluginConfiguration::Uninitialized();
			newConfig.Network.SetPluginConfiguration(testPluginConfig);
			newConfig.Network.Plugins.emplace(TestPluginConfiguration::Name, utils::ConfigurationBag({}));
		});
	}
}}
