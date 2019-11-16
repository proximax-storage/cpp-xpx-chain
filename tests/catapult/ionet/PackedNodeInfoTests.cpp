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

#include "catapult/ionet/PackedNodeInfo.h"
#include "catapult/utils/MemoryUtils.h"
#include "tests/test/core/VariableSizedEntityTestUtils.h"
#include "tests/test/nodeps/NumericTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace ionet {

#define TEST_CLASS PackedNodeInfoTests

	// region sizes

	TEST(TEST_CLASS, PackedConnectionStateHasExpectedSize) {
		// Arrange:
		auto expectedSize = sizeof(ServiceIdentifier) + 3 * sizeof(uint32_t);

		// Assert:
		EXPECT_EQ(expectedSize, sizeof(PackedConnectionState));
		EXPECT_EQ(16u, sizeof(PackedConnectionState));
	}

	TEST(TEST_CLASS, PackedNodeInteractionsHasExpectedSize) {
		// Arrange:
		auto expectedSize = 2 * sizeof(uint32_t);

		// Assert:
		EXPECT_EQ(expectedSize, sizeof(PackedNodeInteractions));
		EXPECT_EQ(8u, sizeof(PackedNodeInteractions));
	}

	TEST(TEST_CLASS, PackedNodeInfoHasExpectedSize) {
		// Arrange:
		auto expectedSize =
				sizeof(uint32_t) // size
				+ Key_Size // identity key
				+ sizeof(NodeSource) // node source
				+ sizeof(NodeInteractions) // node interactions
				+ sizeof(uint8_t); // number of connection states

		// Assert:
		EXPECT_EQ(expectedSize, sizeof(PackedNodeInfo));
		EXPECT_EQ(49u, sizeof(PackedNodeInfo));
	}

	// endregion

	// region CalculateRealSize

	TEST(TEST_CLASS, CanCalculateRealSizeWithReasonableValues) {
		// Arrange:
		PackedNodeInfo nodeInfo;
		nodeInfo.Size = 0;
		nodeInfo.ConnectionStatesCount = 100;

		// Act:
		auto realSize = PackedNodeInfo::CalculateRealSize(nodeInfo);

		// Assert:
		EXPECT_EQ(sizeof(PackedNodeInfo) + 100 * sizeof(PackedConnectionState), realSize);
	}

	TEST(TEST_CLASS, CalculateRealSizeDoesNotOverflowWithMaxValues) {
		// Arrange:
		PackedNodeInfo nodeInfo;
		nodeInfo.Size = 0;
		test::SetMaxValue(nodeInfo.ConnectionStatesCount);

		// Act:
		auto realSize = PackedNodeInfo::CalculateRealSize(nodeInfo);

		// Assert:
		EXPECT_EQ(sizeof(PackedNodeInfo) + nodeInfo.ConnectionStatesCount * sizeof(PackedConnectionState), realSize);
		EXPECT_GE(std::numeric_limits<uint32_t>::max(), realSize);
	}

	// endregion

	// region data pointers

	namespace {
		struct PackedNodeInfoTraits {
			static auto GenerateEntityWithAttachments(uint16_t count) {
				uint32_t entitySize = sizeof(PackedNodeInfo) + count * sizeof(PackedConnectionState);
				auto pNodeInfo = utils::MakeUniqueWithSize<PackedNodeInfo>(entitySize);
				pNodeInfo->Size = entitySize;
				pNodeInfo->ConnectionStatesCount = static_cast<uint8_t>(count);
				return pNodeInfo;
			}

			template<typename TEntity>
			static auto GetAttachmentPointer(TEntity& entity) {
				return entity.ConnectionStatesPtr();
			}
		};
	}

	DEFINE_ATTACHMENT_POINTER_TESTS(TEST_CLASS, PackedNodeInfoTraits) // ConnectionStatesPtr

	// endregion

	// region Update

	TEST(TEST_CLASS, CanCopyConnectionStateValuesIntoPackedConnectionState) {
		// Arrange:
		auto connectionState = ConnectionState();
		connectionState.Age = 1;
		connectionState.NumConsecutiveFailures = 5;
		connectionState.BanAge = 9;

		// Act:
		auto packedConnectionState = PackedConnectionState();
		packedConnectionState.ServiceId = ServiceIdentifier(123);
		packedConnectionState.Update(connectionState);

		// Assert:
		EXPECT_EQ(ServiceIdentifier(123), packedConnectionState.ServiceId);
		EXPECT_EQ(1u, packedConnectionState.Age);
		EXPECT_EQ(5u, packedConnectionState.NumConsecutiveFailures);
		EXPECT_EQ(9u, packedConnectionState.BanAge);
	}

	TEST(TEST_CLASS, CanCopyNodeInteractionsValuesIntoPackedNodeInteractions) {
		// Arrange:
		auto interactions = NodeInteractions();
		interactions.NumSuccesses = 123;
		interactions.NumFailures = 234;

		// Act:
		auto packedInteractions = PackedNodeInteractions();
		packedInteractions.Update(interactions);

		// Assert:
		EXPECT_EQ(123u, packedInteractions.NumSuccesses);
		EXPECT_EQ(234u, packedInteractions.NumFailures);
	}

	// endregion
}}
