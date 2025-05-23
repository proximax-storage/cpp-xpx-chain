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

#include "tests/int/node/stress/test/LocalNodeSyncIntegrityTestUtils.h"
#include "tests/int/node/stress/test/TransactionsBuilder.h"
#include "tests/int/node/test/LocalNodeRequestTestUtils.h"

namespace catapult { namespace local {

#define TEST_CLASS LocalNodeSyncBlockIntegrityTests

	namespace {
		using BlockChainBuilder = test::BlockChainBuilder;
	}

	// region signature

	namespace {
		template<typename TTestContext>
		std::vector<Hash256> RunInvalidSignatureTest(TTestContext& context) {
			// Arrange:
			std::vector<Hash256> stateHashes;
			test::Accounts accounts(3);

			// - prepare a better (unsigned) block
			std::shared_ptr<model::Block> pUnsignedBlock;
			{
				test::TransactionsBuilder transactionsBuilder(accounts);
				transactionsBuilder.addTransfer(0, 1, Amount(1'000'000));

				auto stateHashCalculator = context.createStateHashCalculator();
				BlockChainBuilder builder(accounts, stateHashCalculator);
				builder.setBlockTimeInterval(utils::TimeSpan::FromSeconds(15)); // better block time will yield better chain
				pUnsignedBlock = builder.asSingleBlock(transactionsBuilder);
				test::FillWithRandomData(pUnsignedBlock->Signature);
			}

			// - prepare a worse (signed) block
			std::shared_ptr<model::Block> pSignedBlock;
			{
				test::TransactionsBuilder transactionsBuilder(accounts);
				transactionsBuilder.addTransfer(0, 2, Amount(550'000));

				auto stateHashCalculator = context.createStateHashCalculator();
				BlockChainBuilder builder(accounts, stateHashCalculator);
				pSignedBlock = builder.asSingleBlock(transactionsBuilder);
			}

			// Act:
			test::ExternalSourceConnection connection;
			auto pIo1 = test::PushEntity(connection, ionet::PacketType::Push_Block, pUnsignedBlock);
			auto pIo2 = test::PushEntity(connection, ionet::PacketType::Push_Block, pSignedBlock);

			// - wait for the chain height to change and for all height readers to disconnect
			test::WaitForHeightAndElements(context, Height(2), 2, 2);
			stateHashes.emplace_back(GetStateHash(context));

			// Assert: the cache has expected balances (from the signed block)
			test::AssertCurrencyBalances(accounts, context.localNode().cache(), {
				{ 2, Amount(550'000) },
			});

			return stateHashes;
		}
	}

	NO_STRESS_TEST(TEST_CLASS, BlockWithoutProperSignatureIsRejected) {
		// Arrange:
		test::StateHashDisabledTestContext context(test::NonNemesisTransactionPlugins::None, [](auto& config) {
			const_cast<config::NodeConfiguration&>(config.Node).TransactionBatchSize = 50;
		});

		// Act + Assert:
		auto stateHashes = RunInvalidSignatureTest(context);

		// Assert: all state hashes are zero
		test::AssertAllZero(stateHashes, 1);
	}

	NO_STRESS_TEST(TEST_CLASS, BlockWithoutProperSignatureIsRejectedWithStateHashEnabled) {
		// Arrange:
		test::StateHashEnabledTestContext context(test::NonNemesisTransactionPlugins::None, [](auto& config) {
			const_cast<config::NodeConfiguration&>(config.Node).TransactionBatchSize = 50;
		});

		// Act + Assert:
		auto stateHashes = RunInvalidSignatureTest(context);

		// Assert: all state hashes are nonzero
		test::AssertAllNonZero(stateHashes, 1);
	}

	// endregion
}}
