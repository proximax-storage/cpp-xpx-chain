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

#include "src/validators/Validators.h"
#include "src/state/HashLockInfo.h"
#include "plugins/txes/lock_shared/tests/validators/LockDurationValidatorTests.h"
#include "tests/test/core/mocks/MockBlockchainConfigurationHolder.h"

namespace catapult { namespace validators {

#define TEST_CLASS HashLockDurationValidatorTests

	DEFINE_COMMON_VALIDATOR_TESTS(HashLockDuration)

	namespace {
		struct HashTraits {
		public:
			using NotificationType = model::HashLockDurationNotification<1>;
			static constexpr auto Failure_Result = Failure_LockHash_Invalid_Duration;

			static auto CreateConfigHolder(BlockDuration maxDuration) {
				auto pluginConfig = config::HashLockConfiguration::Uninitialized();
				pluginConfig.MaxHashLockDuration = utils::BlockSpan::FromHours(maxDuration.unwrap());
				auto networkConfig = model::NetworkConfiguration::Uninitialized();
				networkConfig.BlockGenerationTargetTime = utils::TimeSpan::FromHours(1);
				networkConfig.SetPluginConfiguration(pluginConfig);
				auto pConfigHolder = config::CreateMockConfigurationHolder(networkConfig);
				return pConfigHolder;
			}

			static auto CreateValidator() {
				return CreateHashLockDurationValidator();
			}
		};
	}

	DEFINE_DURATION_VALIDATOR_TESTS(HashTraits)
}}
