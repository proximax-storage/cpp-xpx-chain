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

#include "catapult/ionet/NodePacketIoPair.h"
#include "tests/test/core/mocks/MockPacketIo.h"
#include "tests/TestHarness.h"

namespace catapult { namespace ionet {

#define TEST_CLASS NodePacketIoPairTests

	TEST(TEST_CLASS, CanCreateEmptyPair) {
		// Act:
		NodePacketIoPair pair;

		// Assert:
		EXPECT_EQ(Node(), pair.node());
		EXPECT_FALSE(!!pair.io());
		EXPECT_FALSE(!!pair);
	}

	TEST(TEST_CLASS, CanCreateNonEmptyPair) {
		// Act:
		auto node = Node(test::GenerateRandomByteArray<Key>(), ionet::NodeEndpoint(), ionet::NodeMetadata());
		auto pPacketIo = std::make_shared<mocks::MockPacketIo>();
		NodePacketIoPair pair(node, pPacketIo);

		// Assert:
		EXPECT_EQ(node, pair.node());
		EXPECT_EQ(pPacketIo, pair.io());
		EXPECT_TRUE(!!pair);
	}
}}
