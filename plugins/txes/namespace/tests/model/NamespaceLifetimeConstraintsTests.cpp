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

#include "src/model/NamespaceLifetimeConstraints.h"
#include "tests/TestHarness.h"

namespace catapult { namespace model {

#define TEST_CLASS NamespaceLifetimeConstraintsTests

	TEST(TEST_CLASS, CanCreateNamespaceLifetimeConstraints) {
		// Act:
		auto pluginConfig = config::NamespaceConfiguration::Uninitialized();
		pluginConfig.MaxNamespaceDuration = utils::BlockSpan::FromHours(123);
		pluginConfig.NamespaceGracePeriodDuration = utils::BlockSpan::FromHours(234);
		auto networkConfig = model::NetworkConfiguration::Uninitialized();
		networkConfig.BlockGenerationTargetTime = utils::TimeSpan::FromHours(1);
		networkConfig.SetPluginConfiguration(pluginConfig);
		NamespaceLifetimeConstraints constraints(networkConfig);

		// Assert:
		EXPECT_EQ(BlockDuration(123 + 234), constraints.maxNamespaceDuration());
	}
}}
