/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "catapult/plugins/PluginUtils.h"
#include "src/plugins/LockFundPlugin.h"
#include "src/model/LockFundEntityType.h"
#include "tests/test/plugins/PluginManagerFactory.h"
#include "tests/test/plugins/PluginTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace plugins {

	namespace {
		struct LockFundPluginTraits : public test::EmptyPluginTraits {
		public:
			template<typename TAction>
			static void RunTestAfterRegistration(TAction action) {
				// Arrange:
				auto config = model::NetworkConfiguration::Uninitialized();
				config.Plugins.emplace(PLUGIN_NAME(lockfund), utils::ConfigurationBag({{ "", { { "minRequestUnlockCooldown", "200000" }, { "maxMosaicsSize", "256" } } }}));
				auto manager = test::CreatePluginManager(config);
				RegisterLockFundSubsystem(manager);

				// Act:
				action(manager);
			}

		public:
			static std::vector<model::EntityType> GetTransactionTypes() {
				return { model::Entity_Type_Lock_Fund_Transfer, model::Entity_Type_Lock_Fund_Cancel_Unlock };
			}

			static std::vector<std::string> GetStatelessValidatorNames() {
				return { "LockFundPluginConfigValidator" };
			}

			static std::vector<std::string> GetStatefulValidatorNames() {
				return { "LockFundTransferValidator", "LockFundCancelUnlockValidator"};
			}

			static std::vector<std::string> GetObserverNames()
			{
				return { "LockFundBlockObserver", "LockFundTransferObserver", "LockFundCancelUnlockObserver"};
			}

			static std::vector<std::string> GetPermanentObserverNames()
			{
				return GetObserverNames();
			}
			static std::vector<std::string> GetCacheNames() {
				return { "LockFundCache" };
			}
		};
	}

	DEFINE_PLUGIN_TESTS(LockFundPluginTests, LockFundPluginTraits)
}}
