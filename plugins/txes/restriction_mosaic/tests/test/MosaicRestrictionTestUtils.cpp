/**
*** Copyright (c) 2016-2019, Jaguar0625, gimre, BloodyRookie, Tech Bureau, Corp.
*** Copyright (c) 2020-present, Jaguar0625, gimre, BloodyRookie.
*** All rights reserved.
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
#include "src/config/MosaicRestrictionConfiguration.h"
#include "tests/test/other/MutableBlockchainConfiguration.h"
#include "tests/test/core/mocks/MockBlockchainConfigurationHolder.h"
#include "tests/test/MosaicRestrictionTestUtils.h"
#include "catapult/model/Address.h"
#include "tests/TestHarness.h"

namespace catapult { namespace test {

	state::MosaicRestrictionEntry GenerateMosaicRestrictionEntry(const Hash256& hash) {
		// hack to set the mosaic restriction unique key (required for cache tests)
		auto addressRestriction = state::MosaicAddressRestriction(
				test::GenerateRandomValue<MosaicId>(),
				test::GenerateRandomByteArray<Address>());
		auto entry = state::MosaicRestrictionEntry(addressRestriction);
		const_cast<Hash256&>(entry.uniqueKey()) = hash;
		return entry;
	}

	namespace {
		void AssertEqual(const state::MosaicAddressRestriction& expected, const state::MosaicAddressRestriction& actual) {
			EXPECT_EQ(expected.mosaicId(), actual.mosaicId());
			EXPECT_EQ(expected.address(), actual.address());
			EXPECT_EQ(expected.keys(), actual.keys());

			for (auto key : expected.keys())
				EXPECT_EQ(expected.get(key), actual.get(key)) << "key " << key;
		}

		void AssertEqual(const state::MosaicGlobalRestriction& expected, const state::MosaicGlobalRestriction& actual) {
			EXPECT_EQ(expected.mosaicId(), actual.mosaicId());
			EXPECT_EQ(expected.keys(), actual.keys());

			for (auto key : expected.keys()) {
				state::MosaicGlobalRestriction::RestrictionRule expectedRule;
				state::MosaicGlobalRestriction::RestrictionRule actualRule;
				expected.tryGet(key, expectedRule);
				actual.tryGet(key, actualRule);

				EXPECT_EQ(expectedRule.ReferenceMosaicId, actualRule.ReferenceMosaicId) << "key " << key;
				EXPECT_EQ(expectedRule.RestrictionValue, actualRule.RestrictionValue) << "key " << key;
				EXPECT_EQ(expectedRule.RestrictionType, actualRule.RestrictionType) << "key " << key;
			}
		}
	}

	void AssertEqual(const state::MosaicRestrictionEntry& expected, const state::MosaicRestrictionEntry& actual) {
		ASSERT_EQ(expected.entryType(), actual.entryType());

		if (state::MosaicRestrictionEntry::EntryType::Address == expected.entryType())
			AssertEqual(expected.asAddressRestriction(), actual.asAddressRestriction());
		else
			AssertEqual(expected.asGlobalRestriction(), actual.asGlobalRestriction());
	}

	std::shared_ptr<config::BlockchainConfigurationHolder> CreateMosaicRestrictionConfigHolder(model::NetworkIdentifier networkIdentifier) {
		auto pluginConfig = config::MosaicRestrictionConfiguration::Uninitialized();
		pluginConfig.Enabled = true;
		pluginConfig.MaxMosaicRestrictionValues = 10;
		auto networkConfig = model::NetworkConfiguration::Uninitialized();
		networkConfig.BlockGenerationTargetTime = utils::TimeSpan::FromHours(1);
		networkConfig.SetPluginConfiguration(pluginConfig);
		test::MutableBlockchainConfiguration config;
		config.Network = networkConfig;
		config.Immutable.NetworkIdentifier = networkIdentifier;
		return config::CreateMockConfigurationHolder(config.ToConst());
	}

	Address ConvertToAddress(const Key& key)
	{
		return model::PublicKeyToAddress(key, model::NetworkIdentifier::Zero);
	}

	std::vector<Address> ConvertToAddress(const std::vector<Key>& keys)
	{
		std::vector<Address> result;
		for(auto& key : keys)
		{
			result.push_back(model::PublicKeyToAddress(key, model::NetworkIdentifier::Zero));
		}
		return result;
	}
}}
