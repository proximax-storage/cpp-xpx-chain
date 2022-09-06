/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "plugins/services/globalstore/src/cache/GlobalStoreCache.h"
#include "plugins/services/globalstore/src/state/BaseConverters.h"
#include "plugins/services/globalstore/src/state/GlobalEntry.h"
#include "tests/test/LockFundCacheFactory.h"
#include "tests/test/core/NotificationTestUtils.h"
#include "src/config/LockFundConfiguration.h"
#include "src/observers/Observers.h"
#include "tests/test/plugins/ObserverTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace observers {

#define TEST_CLASS LockFundInstallObserverTests

	using ObserverTestContext = test::ObserverTestContextT<test::LockFundCacheFactory>;

	DEFINE_COMMON_OBSERVER_TESTS(LockFundInstall,)

	namespace {

		void RunTest(
				bool isInstalled,
				NotifyMode notifyMode) {
			// Arrange:
			// Prepare Configurations
			auto initialConfiguration = test::MutableBlockchainConfiguration();
			auto modifiedConfiguration = test::MutableBlockchainConfiguration();

			auto pluginConfig = config::LockFundConfiguration::Uninitialized();
			pluginConfig.Enabled = false;

			auto pluginConfigNew = config::LockFundConfiguration::Uninitialized();
			pluginConfigNew.Enabled = true;

			initialConfiguration.Network.SetPluginConfiguration(pluginConfig);
			modifiedConfiguration.Network.SetPluginConfiguration(pluginConfigNew);

			auto oldConfiguration = initialConfiguration.ToConst();
			modifiedConfiguration.PreviousConfiguration = &oldConfiguration;
			modifiedConfiguration.ActivationHeight = Height(777);

			ObserverTestContext context(notifyMode, Height(777), modifiedConfiguration.ToConst());
			auto& globalStore = context.cache().template sub<cache::GlobalStoreCache>();

			if(isInstalled)
			{
				state::GlobalEntry installedRecord(config::LockFundPluginInstalled_GlobalKey, 5, state::PluginInstallConverter());
				globalStore.insert(installedRecord);
			}
			else if(notifyMode == NotifyMode::Rollback)
			{
				state::GlobalEntry installedRecord(config::LockFundPluginInstalled_GlobalKey, 777, state::PluginInstallConverter());
				globalStore.insert(installedRecord);
			}


			auto notification = test::CreateBlockNotification();
			auto pObserver = CreateLockFundInstallObserver();

			// Act:
			test::ObserveNotification(*pObserver, notification, context);



			// Assert:
			if(!isInstalled)
			{
				if(notifyMode == NotifyMode::Commit)
				{
					auto installationKey = globalStore.find(config::LockFundPluginInstalled_GlobalKey).get();
					auto value = installationKey.template Get<state::PluginInstallConverter>();
					EXPECT_EQ(value, 777);
				}

				else
				{
					EXPECT_FALSE(globalStore.find(config::LockFundPluginInstalled_GlobalKey).tryGet());
				}
			}
			else
			{
				auto installationKey = globalStore.find(config::LockFundPluginInstalled_GlobalKey).get();
				auto value = installationKey.template Get<state::PluginInstallConverter>();
				EXPECT_EQ(value, 5);
			}

		}
	}

	// region testing

	TEST(TEST_CLASS, ObserverCanInstallPlugin_NotInstalled_Commit) {
		// Arrange
		RunTest(false, NotifyMode::Commit);
	}

	TEST(TEST_CLASS, ObserverCanInstallPlugin_NotInstalled_Rollback) {
		// Arrange
		RunTest(false, NotifyMode::Commit);
	}

	TEST(TEST_CLASS, ObserverCanInstallPlugin_Installed_Commit) {
		// Arrange
		RunTest(true, NotifyMode::Rollback);
	}

	TEST(TEST_CLASS, ObserverCanInstallPlugin_Installed_Rollback) {
		// Arrange
		RunTest(true, NotifyMode::Rollback);
	}

	// endregion
}}
