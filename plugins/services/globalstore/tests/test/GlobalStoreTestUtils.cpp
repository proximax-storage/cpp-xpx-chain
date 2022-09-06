/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/


#include "tests/test/other/MutableBlockchainConfiguration.h"
#include "tests/test/core/mocks/MockBlockchainConfigurationHolder.h"
#include "GlobalStoreTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace test {
	std::shared_ptr<config::BlockchainConfigurationHolder> CreateGlobalStoreConfigHolder(model::NetworkIdentifier networkIdentifier) {
		auto pluginConfig = config::GlobalStoreConfiguration::Uninitialized();
		pluginConfig.Enabled = true;
		auto networkConfig = model::NetworkConfiguration::Uninitialized();
		networkConfig.SetPluginConfiguration(pluginConfig);
		test::MutableBlockchainConfiguration config;
		config.Network = networkConfig;
		config.Immutable.NetworkIdentifier = networkIdentifier;
		return config::CreateMockConfigurationHolder(config.ToConst());
	}

	state::GlobalEntry CreateGlobalEntry(const Hash256& key) {

		auto generatedVal = test::GenerateRandomArray<30>();
		std::vector<uint8_t> randomValue(generatedVal.begin(), generatedVal.end());
		state::GlobalEntry entry(key, randomValue);
		return entry;
	}

	void AssertEqual(const state::GlobalEntry& expected, const state::GlobalEntry& actual) {
		EXPECT_EQ(expected.GetKey(), actual.GetKey());
		EXPECT_EQ(expected.Get(), actual.Get());
	}
}}
