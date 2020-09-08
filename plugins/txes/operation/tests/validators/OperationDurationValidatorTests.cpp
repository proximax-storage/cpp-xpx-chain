/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/config/OperationConfiguration.h"
#include "src/validators/Validators.h"
#include "plugins/txes/lock_shared/tests/validators/LockDurationValidatorTests.h"
#include "tests/test/core/mocks/MockBlockchainConfigurationHolder.h"

namespace catapult { namespace validators {

#define TEST_CLASS OperationDurationValidatorTests

	DEFINE_COMMON_VALIDATOR_TESTS(OperationDuration)

	namespace {
		auto networkConfig = model::NetworkConfiguration::Uninitialized();
		struct SecretTraits {
		public:
			using NotificationType = model::OperationDurationNotification<1>;
			static constexpr auto Failure_Result = Failure_Operation_Invalid_Duration;

			static auto CreateConfigHolder(BlockDuration maxDuration) {
				auto pluginConfig = config::OperationConfiguration::Uninitialized();
				pluginConfig.MaxOperationDuration = utils::BlockSpan::FromHours(maxDuration.unwrap());
				networkConfig.BlockGenerationTargetTime = utils::TimeSpan::FromHours(1);
				networkConfig.SetPluginConfiguration(pluginConfig);
				auto pConfigHolder = config::CreateMockConfigurationHolder(networkConfig);
				return pConfigHolder;
			}

			static auto CreateValidator() {
				return CreateOperationDurationValidator();
			}
		};
	}

	DEFINE_DURATION_VALIDATOR_TESTS(SecretTraits)
}}
