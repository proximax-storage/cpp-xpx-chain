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
#include "tests/TestHarness.h"

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
			const_cast<bool&>(state.pluginManager().configHolder()->Config(state.cache().height()).BlockChain.ShouldEnableVerifiableState) = true;
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
				m_pServiceState = std::make_unique<test::ServiceTestState>();
				EnableVerifiableState(*m_pServiceState);
				CopyStorage(storage, m_pServiceState->state().storage());
				RegisterSubCacheMerkleRootsHandler(m_pServiceState->state());
				handlers = m_pServiceState->state().packetHandlers();
			}

		private:
			static std::unique_ptr<test::ServiceTestState> m_pServiceState;
		};
	}

	std::unique_ptr<test::ServiceTestState> SubCacheMerkleRootsHandlerTraits::m_pServiceState = nullptr;

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
		FillStorage(serviceState.state().storage(), 12);
		RegisterSubCacheMerkleRootsHandler(serviceState.state());

		// - remove merkle hashes from last block
		auto pBlockElementFromStorage = serviceState.state().storage().view().loadBlockElement(Height(7));
		const_cast<model::BlockElement&>(*pBlockElementFromStorage).SubCacheMerkleRoots.clear();

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
