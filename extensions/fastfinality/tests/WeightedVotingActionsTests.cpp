/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "fastfinality/src/WeightedVotingActions.cpp"
#include "tests/test/local/ServiceLocatorTestContext.h"
#include "tests/test/core/BlockTestUtils.h"
#include "tests/test/core/ThreadPoolTestUtils.h"
#include "tests/test/core/mocks/MockCommitteeManager.h"

namespace catapult { namespace fastfinality {

	namespace {
		class TestWeightedVotingActions : public ::testing::Test {
		protected:
			void SetUp() {
				pBlock = utils::UniqueToShared(test::GenerateEmptyRandomBlock());
				pBlock->Signer = committeeManager.committee().BlockProposer;
			}

			void TearDown() {}

			std::shared_ptr<model::Block> pBlock;
			const model::BlockElementSupplier lastBlockElementSupplier;
			test::ServiceTestState serviceState = test::ServiceTestState();
			mocks::MockCommitteeManager committeeManager = mocks::MockCommitteeManager();
			const std::shared_ptr<thread::IoThreadPool> pPool = test::CreateStartedIoThreadPool();
		};
	} // namespace

	TEST_F(TestWeightedVotingActions, ValidateBlockCosignatures_Success) {
		ASSERT_TRUE(ValidateBlockCosignatures(pBlock, committeeManager, 0.67));
	}

	TEST_F(TestWeightedVotingActions, ValidateBlockCosignatures_InvalidSigner) {
		pBlock->Signer = test::GenerateRandomByteArray<Key>();
		ASSERT_FALSE(ValidateBlockCosignatures(pBlock, committeeManager, 0.67));
	}

	TEST_F(TestWeightedVotingActions, ValidateBlockCosignatures_NumCosignatures) {
		pBlock->Size *= 2;
		ASSERT_FALSE(ValidateBlockCosignatures(pBlock, committeeManager, 0.67));
	}

	TEST_F(TestWeightedVotingActions, ValidateBlockCosignatures_Cosignatures) {
		auto numCosignatures = 3;
		std::shared_ptr<model::Block> pBlockWithCosigs =
				test::GenerateBlockWithTransactionsAndCosignatures(5, Height(3), numCosignatures);

		auto newCommittee = committeeManager.committee();
		for (int i = 0; i < numCosignatures - 1; i++)
			newCommittee.Cosigners.insert((pBlockWithCosigs->CosignaturesPtr() + i)->Signer);

		committeeManager.setCommittee(newCommittee);
		pBlockWithCosigs->Signer = pBlock->Signer;
		ASSERT_FALSE(ValidateBlockCosignatures(pBlockWithCosigs, committeeManager, 0.67));
	}

	TEST_F(TestWeightedVotingActions, ValidateProposedBlock_AbortedState) {
		ASSERT_FALSE(ValidateProposedBlock(pBlock, serviceState.state(), lastBlockElementSupplier, pPool));
	}

}} // namespace catapult::fastfinality
