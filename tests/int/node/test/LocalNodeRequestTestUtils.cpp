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

#include "LocalNodeRequestTestUtils.h"
#include "sdk/src/extensions/BlockExtensions.h"
#include "plugins/txes/mosaic/src/plugins/MosaicDefinitionTransactionPlugin.h"
#include "plugins/txes/mosaic/src/plugins/MosaicSupplyChangeTransactionPlugin.h"
#include "plugins/txes/namespace/src/plugins/MosaicAliasTransactionPlugin.h"
#include "plugins/txes/namespace/src/plugins/RegisterNamespaceTransactionPlugin.h"
#include "plugins/txes/transfer/src/plugins/TransferTransactionPlugin.h"
#include "tests/test/core/BlockTestUtils.h"
#include "tests/test/core/mocks/MockLocalNodeConfigurationHolder.h"
#include "tests/test/local/RealTransactionFactory.h"

namespace catapult { namespace test {

	// region ExternalSourceConnection

	model::TransactionRegistry ExternalSourceConnection::CreateTransactionRegistry() {
		auto registry = model::TransactionRegistry();
		registry.registerPlugin(plugins::CreateMosaicDefinitionTransactionPlugin(config::CreateMockConfigurationHolder()));
		registry.registerPlugin(plugins::CreateMosaicSupplyChangeTransactionPlugin());
		registry.registerPlugin(plugins::CreateMosaicAliasTransactionPlugin());
		registry.registerPlugin(plugins::CreateRegisterNamespaceTransactionPlugin(config::CreateMockConfigurationHolder()));
		registry.registerPlugin(plugins::CreateTransferTransactionPlugin());
		return registry;
	}

	// endregion

	// region push

	std::shared_ptr<ionet::PacketIo> PushPayload(ExternalSourceConnection& connection, const ionet::PacketPayload& payload) {
		CATAPULT_LOG(debug) << " >>>> starting push";
		std::atomic_bool isWriteFinished(false);
		connection.connect([&isWriteFinished, &payload](const auto& pPacketSocket) {
			CATAPULT_LOG(debug) << "writing entity";
			pPacketSocket->write(payload, [&isWriteFinished](auto code) {
				CATAPULT_LOG(debug) << "write result: " << code;
				isWriteFinished = true;
			});
		});

		WAIT_FOR(isWriteFinished);
		CATAPULT_LOG(debug) << " <<< push finished";
		return connection.io();
	}

	namespace {
		crypto::KeyPair GetNemesisAccountKeyPair() {
			return crypto::KeyPair::FromString(Mijin_Test_Private_Keys[0]); // use a nemesis account
		}

		std::shared_ptr<model::Block> CreateBlock() {
			constexpr auto Network_Identifier = model::NetworkIdentifier::Mijin_Test;
			auto signer = GetNemesisAccountKeyPair();

			mocks::MockMemoryBlockStorage storage;
			auto pNemesisBlockElement = storage.loadBlockElement(Height(1));

			model::PreviousBlockContext context(*pNemesisBlockElement);
			auto pBlock = model::CreateBlock(context, Network_Identifier, signer.publicKey(), model::Transactions());
			pBlock->Timestamp = context.Timestamp + Timestamp(60000);
			pBlock->Difficulty = Difficulty(NEMESIS_BLOCK_DIFFICULTY);
			pBlock->Version = MakeVersion(Network_Identifier, 3);
			pBlock->FeeInterest = 1;
			pBlock->FeeInterestDenominator = 2;
			extensions::BlockExtensions(GetDefaultGenerationHash()).signFullBlock(signer, *pBlock);
			return std::move(pBlock);
		}
	}

	std::shared_ptr<ionet::PacketIo> PushValidBlock(ExternalSourceConnection& connection) {
		auto pBlock = CreateBlock();
		return PushEntity(connection, ionet::PacketType::Push_Block, pBlock);
	}

	std::shared_ptr<ionet::PacketIo> PushValidTransaction(ExternalSourceConnection& connection) {
		auto recipient = test::GenerateRandomUnresolvedAddress();
		auto pTransaction = CreateTransferTransaction(GetNemesisAccountKeyPair(), recipient, Amount(10000));
		return PushEntity(connection, ionet::PacketType::Push_Transactions, std::shared_ptr<model::Transaction>(std::move(pTransaction)));
	}

	// endregion

	// region height

	Height GetLocalNodeHeightViaApi(ExternalSourceConnection& connection) {
		struct ChainInfoResult {
		public:
			ChainInfoResult() : IsHeightReceived(false)
			{}

		public:
			catapult::Height Height;
			std::atomic_bool IsHeightReceived;
		};

		auto pChainInfoResult = std::make_shared<ChainInfoResult>();
		connection.apiCall([pChainInfoResult](const auto& pRemoteChainApi) {
			pRemoteChainApi->chainInfo().then([pChainInfoResult](auto&& infoFuture) {
				pChainInfoResult->Height = infoFuture.get().Height;
				pChainInfoResult->IsHeightReceived = true;
			});
		});

		WAIT_FOR(pChainInfoResult->IsHeightReceived);
		return Height(pChainInfoResult->Height);
	}

	void WaitForLocalNodeHeight(ExternalSourceConnection& connection, Height height) {
		// use exponential backoff to reduce log noise
		auto sleepMs = 100;
		supplier<Height> heightSupplierWithBackoff = [&connection, desiredHeight = height, &sleepMs]() {
			auto currentHeight = GetLocalNodeHeightViaApi(connection);
			if (desiredHeight != currentHeight) {
				Sleep(sleepMs);
				sleepMs *= 2;
			}

			return currentHeight;
		};

		WAIT_FOR_VALUE_EXPR(height, heightSupplierWithBackoff());

	}

	// endregion
}}
