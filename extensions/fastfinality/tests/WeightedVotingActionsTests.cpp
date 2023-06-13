/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/catapult/model/BlockUtils.cpp"
#include "tests/test/core/BlockTestUtils.h"
#include "tests/test/core/ThreadPoolTestUtils.h"
#include "tests/test/core/mocks/MockCommitteeManager.h"
#include "tests/test/local/ServiceLocatorTestContext.h"
#include "fastfinality/src/WeightedVotingActions.cpp"

namespace catapult { namespace fastfinality {

	namespace {
		class TestWeightedVotingActions : public ::testing::Test {
		protected:
			void SetUp() {
				pBlock = utils::UniqueToShared(test::GenerateEmptyRandomBlock());
				pBlock->Signer = committeeManager.committee().BlockProposer;
				model::SignBlockHeader(keyPair, *pBlock);
			}

			void TearDown() {}

			std::shared_ptr<model::Block> pBlock;
			const model::BlockElementSupplier lastBlockElementSupplier;
			test::ServiceTestState serviceState = test::ServiceTestState();
			const crypto::KeyPair keyPair = crypto::KeyPair::FromPrivate(test::GenerateRandomPrivateKey());
			mocks::MockCommitteeManager committeeManager = mocks::MockCommitteeManager(keyPair.publicKey());
			const std::shared_ptr<thread::IoThreadPool> pPool = test::CreateStartedIoThreadPool();

			std::shared_ptr<model::Block> CreateValidBlockWithCosigners(uint8_t numCosignatures) {
				std::shared_ptr<model::Block> pBlockWithCosigs =
						test::GenerateBlockWithTransactionsAndCosignatures(5, Height(3), numCosignatures);

				auto pCosignature = pBlockWithCosigs->CosignaturesPtr();
				for (auto i = 0u; i < pBlockWithCosigs->CosignaturesCount(); ++i, ++pCosignature) {
					const crypto::KeyPair keyPair = crypto::KeyPair::FromPrivate(test::GenerateRandomPrivateKey());
					pCosignature->Signer = keyPair.publicKey();
					model::CosignBlockHeader(keyPair, *pBlockWithCosigs, pCosignature->Signature);
				}

				pBlockWithCosigs->Signer = pBlock->Signer;
				model::SignBlockHeader(keyPair, *pBlockWithCosigs);

				return pBlockWithCosigs;
			}

			chain::Committee CreateNewCommitteeFromBlockCosigners(std::shared_ptr<model::Block> pBlockWithCosigs) {
				auto newCommittee = committeeManager.committee();
				auto pCosignature = pBlockWithCosigs->CosignaturesPtr();
				for (auto i = 0u; i < pBlockWithCosigs->CosignaturesCount(); ++i, ++pCosignature)
					newCommittee.Cosigners.insert(pCosignature->Signer);

				return newCommittee;
			}
		};
	}

	TEST_F(TestWeightedVotingActions, ValidateBlockCosignatures_Success) {
		ASSERT_TRUE(ValidateBlockCosignatures(pBlock, committeeManager, 0.67));
	}

	TEST_F(TestWeightedVotingActions, ValidateBlockCosignatures_InvalidSigner) {
		pBlock->Signer = test::GenerateRandomByteArray<Key>();
		ASSERT_FALSE(ValidateBlockCosignatures(pBlock, committeeManager, 0.67));
	}

	TEST_F(TestWeightedVotingActions, ValidateBlockCosignatures_InvalidHeaderSignature) {
		pBlock->Signature = test::GenerateRandomByteArray<Signature>();
		ASSERT_FALSE(ValidateBlockCosignatures(pBlock, committeeManager, 0.67));
	}

	TEST_F(TestWeightedVotingActions, ValidateBlockCosignatures_NumCosignatures) {
		pBlock->Size *= 2;
		ASSERT_FALSE(ValidateBlockCosignatures(pBlock, committeeManager, 0.67));
	}

	TEST_F(TestWeightedVotingActions, ValidateBlockCosignatures_InvalidCosigner) {
		auto pBlockWithCosigs = CreateValidBlockWithCosigners(3);
		auto newCommittee = CreateNewCommitteeFromBlockCosigners(pBlockWithCosigs);
		committeeManager.setCommittee(newCommittee);
		pBlockWithCosigs->CosignaturesPtr()->Signer = test::GenerateRandomByteArray<Key>();

		ASSERT_FALSE(ValidateBlockCosignatures(pBlockWithCosigs, committeeManager, 0.67));
	}

	TEST_F(TestWeightedVotingActions, ValidateBlockCosignatures_InvalidCosignature) {
		auto pBlockWithCosigs = CreateValidBlockWithCosigners(3);
		auto newCommittee = CreateNewCommitteeFromBlockCosigners(pBlockWithCosigs);
		committeeManager.setCommittee(newCommittee);
		pBlockWithCosigs->CosignaturesPtr()->Signature = test::GenerateRandomByteArray<Signature>();

		ASSERT_FALSE(ValidateBlockCosignatures(pBlockWithCosigs, committeeManager, 0.67));
	}

	TEST_F(TestWeightedVotingActions, ValidateBlockCosignatures_Cosignatures_Success) {
		auto pBlockWithCosigs = CreateValidBlockWithCosigners(3);
		auto newCommittee = CreateNewCommitteeFromBlockCosigners(pBlockWithCosigs);
		committeeManager.setCommittee(newCommittee);
		ASSERT_TRUE(ValidateBlockCosignatures(pBlockWithCosigs, committeeManager, 0.67));
	}

	TEST_F(TestWeightedVotingActions, ValidateProposedBlock_AbortedState) {
		ASSERT_FALSE(ValidateProposedBlock(pBlock, serviceState.state(), lastBlockElementSupplier, pPool));
	}

}}
