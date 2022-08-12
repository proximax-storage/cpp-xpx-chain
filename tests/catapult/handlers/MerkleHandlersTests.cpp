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

#include "catapult/handlers/MerkleHandlers.h"
#include "catapult/api/ChainPackets.h"
#include "tests/catapult/handlers/test/HeightRequestHandlerTests.h"
#include "tests/test/local/ServiceLocatorTestContext.h"

namespace catapult { namespace handlers {

#define TEST_CLASS MerkleHandlersTests

	namespace {
		constexpr auto FillStorage = test::VariableSizedBlockChain::FillStorage;

		void CopyStorage(const io::BlockStorageCache& source, io::BlockStorageCache& dest) {
			auto storageView = source.view();
			auto storageModifier = dest.modifier();
			// dest storage already contains nemesis block (height 1)
			for (auto height = Height{2u}; height <= storageView.chainHeight(); height = height + Height{1}) {
				storageModifier.saveBlock(*storageView.loadBlockElement(height));
			}
		}

		void EnableVerifiableState(test::ServiceTestState& serviceState) {
			auto state = serviceState.state();
			const_cast<bool&>(state.pluginManager().immutableConfig().ShouldEnableVerifiableState) = true;
		}
	}

	// region SubCacheMerkleRootsHandler

	namespace {
		using SubCacheMerkleRootsRequestPacket = api::HeightPacket<ionet::PacketType::Sub_Cache_Merkle_Roots>;

		struct SubCacheMerkleRootsHandlerTraits {

			static ionet::PacketType ResponsePacketType() {
				return SubCacheMerkleRootsRequestPacket::Packet_Type;
			}

			static auto CreateRequestPacket() {
				return ionet::CreateSharedPacket<SubCacheMerkleRootsRequestPacket>();
			}

			static void Register(ionet::ServerPacketHandlers& handlers, const io::BlockStorageCache& storage) {
				static std::unique_ptr<test::ServiceTestState> pServiceState;
				pServiceState = std::make_unique<test::ServiceTestState>();
				EnableVerifiableState(*pServiceState);
				CopyStorage(storage, pServiceState->state().storage());
				RegisterSubCacheMerkleRootsHandler(pServiceState->state());
				handlers = pServiceState->state().packetHandlers();
			}
		};
	}

	DEFINE_HEIGHT_REQUEST_HANDLER_TESTS(SubCacheMerkleRootsHandlerTraits, SubCacheMerkleRootsHandler)

	namespace {
		void AssertCanRetrieveSubCacheMerkleRoots(size_t numBlocks, Height requestHeight) {
			// Arrange:
			auto serviceState = test::ServiceTestState();
			EnableVerifiableState(serviceState);
			FillStorage(serviceState.state().storage(), numBlocks);
			RegisterSubCacheMerkleRootsHandler(serviceState.state());

			auto pPacket = ionet::CreateSharedPacket<SubCacheMerkleRootsRequestPacket>();
			pPacket->Height = requestHeight;

			// Act:
			ionet::ServerPacketHandlerContext context({}, "");
			EXPECT_TRUE(serviceState.state().packetHandlers().process(*pPacket, context));

			// Assert:
			const auto Num_Expected_Hashes = 3u;
			auto expectedSize = sizeof(ionet::PacketHeader) + Num_Expected_Hashes * Hash256_Size;
			test::AssertPacketHeader(context, expectedSize, SubCacheMerkleRootsRequestPacket::Packet_Type);

			auto pBlockElementFromStorage = serviceState.state().storage().view().loadBlockElement(requestHeight);
			ASSERT_EQ(Num_Expected_Hashes, pBlockElementFromStorage->SubCacheMerkleRoots.size());

			auto pHashesFromPacket = reinterpret_cast<const Hash256*>(test::GetSingleBufferData(context));
			for (auto i = 0u; i < Num_Expected_Hashes; ++i)
				EXPECT_EQ(pBlockElementFromStorage->SubCacheMerkleRoots[i], pHashesFromPacket[i]) << "hash at " << i;
		}
	}

	TEST(TEST_CLASS, SubCacheMerkleRootsHandler_WritesSubCacheMerkleRootsWhenPresent) {
		// Assert:
		AssertCanRetrieveSubCacheMerkleRoots(12, Height(7));
	}

	TEST(TEST_CLASS, SubCacheMerkleRootsHandler_CanRetrieveLastBlockSubCacheMerkleRoots) {
		// Assert:
		AssertCanRetrieveSubCacheMerkleRoots(12, Height(12));
	}

	TEST(TEST_CLASS, SubCacheMerkleRootsHandler_WritesEmptyResponseWhenSubCacheMerkleRootsAreNotPresent) {
		// Arrange:
		auto serviceState = test::ServiceTestState();
		EnableVerifiableState(serviceState);
		RegisterSubCacheMerkleRootsHandler(serviceState.state());

		{
			auto storageModifier = serviceState.state().storage().modifier();
			for (auto i = 2u; i <= 12u; ++i) {
				auto size = static_cast<uint32_t>(sizeof(model::BlockHeader) + i * 100);
				std::vector<uint8_t> buffer(size);
				auto pBlock = reinterpret_cast<model::Block *>(buffer.data());
				pBlock->Size = size;
				pBlock->Height = Height(i);
				pBlock->Difficulty = Difficulty::Min() + Difficulty::Unclamped(1000 + i);
				pBlock->TransactionsPtr()->Size = size - sizeof(model::BlockHeader);
				auto blockElement = test::BlockToBlockElement(*pBlock, test::GenerateRandomByteArray<Hash256>());
				if (i == 7)
					blockElement.SubCacheMerkleRoots.clear();
				storageModifier.saveBlock(blockElement);
			}

			storageModifier.commit();
		}

		auto pPacket = ionet::CreateSharedPacket<SubCacheMerkleRootsRequestPacket>();
		pPacket->Height = Height(7);

		// Act:
		ionet::ServerPacketHandlerContext context({}, "");
		EXPECT_TRUE(serviceState.state().packetHandlers().process(*pPacket, context));

		// Assert: only a payload header is written
		test::AssertPacketHeader(context, sizeof(ionet::PacketHeader), SubCacheMerkleRootsRequestPacket::Packet_Type);
		EXPECT_TRUE(context.response().buffers().empty());
	}

	// endregion
}}
