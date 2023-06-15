/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "fastfinality/src/WeightedVotingTransitionTable.h"
#include "tests/TestHarness.h"

namespace catapult { namespace fastfinality {

#define TEST_CLASS WeightedVotingServiceTests

	namespace {
		struct CallbackInvokeCounter {
			int InvokeNum = 0;
			void operator()() {
				InvokeNum++;
			}
		};

		template <typename F>
		void Assert(const std::vector<std::pair<action&, int>>& expectedNums) {
			// Assert:
			for (const auto& a : expectedNums) {
				EXPECT_EQ(a.second, reinterpret_cast<const CallbackInvokeCounter&>(a.first).InvokeNum);
			}
		}


		void CheckCommitteeSelection(boost::sml::sm<WeightedVotingTransitionTable> sm, WeightedVotingActions actions, CommitteeSelectionResult e) {
			// Arrange:
			actions.CheckLocalChain = CallbackInvokeCounter{};
			actions.DetectStage = CallbackInvokeCounter{};
			actions.SelectCommittee = CallbackInvokeCounter{};

			sm.process_event(StartLocalChainCheck{});

			// Act:
			sm.process_event(NetworkHeightEqualToLocal{});
			sm.process_event(StageDetectionSucceeded{});
			sm.process_event(e);

			// Assert:
			EXPECT_TRUE(sm.is(boost::sml::X));
			EXPECT_EQ(1, reinterpret_cast<const CallbackInvokeCounter&>(actions.CheckLocalChain).InvokeNum);
			EXPECT_EQ(1, reinterpret_cast<const CallbackInvokeCounter&>(actions.DetectStage).InvokeNum);
			EXPECT_EQ(1, reinterpret_cast<const CallbackInvokeCounter&>(actions.SelectCommittee).InvokeNum);
		}
	}

	TEST(TEST_CLASS, WeightedVotingTransitionTable_InitialState) {
		// Arrange:
		WeightedVotingActions actions;

		// Act:
		boost::sml::sm<WeightedVotingTransitionTable> sm{actions};

		// Assert:
		EXPECT_TRUE(sm.is(boost::sml::state<InitialState>));
	}

	TEST(TEST_CLASS, WeightedVotingTransitionTable_CheckLocalChain) {
		// Arrange:
		WeightedVotingActions actions;
		actions.CheckLocalChain = CallbackInvokeCounter{};

		// Act:
		boost::sml::sm<WeightedVotingTransitionTable> sm{actions};
		sm.process_event(StartLocalChainCheck{});

		// Assert:
		EXPECT_TRUE(sm.is(boost::sml::state<LocalChainCheck>));
		EXPECT_EQ(1, reinterpret_cast<const CallbackInvokeCounter&>(actions.CheckLocalChain).InvokeNum);
	}

	TEST(TEST_CLASS, WeightedVotingTransitionTable_NetworkHeightDetectionFailure) {
		// Arrange:
		WeightedVotingActions actions;
		actions.CheckLocalChain = CallbackInvokeCounter{};

		boost::sml::sm<WeightedVotingTransitionTable> sm{actions};
		sm.process_event(StartLocalChainCheck{});

		// Act:
		sm.process_event(NetworkHeightDetectionFailure{});

		// Assert:
		EXPECT_TRUE(sm.is(boost::sml::state<LocalChainCheck>));
		EXPECT_EQ(2, reinterpret_cast<const CallbackInvokeCounter&>(actions.CheckLocalChain).InvokeNum);
	}

	TEST(TEST_CLASS, WeightedVotingTransitionTable_InvalidLocalChain_Less) {
		// Arrange:
		WeightedVotingActions actions;
		actions.CheckLocalChain = CallbackInvokeCounter{};
		actions.ResetLocalChain = CallbackInvokeCounter{};

		boost::sml::sm<WeightedVotingTransitionTable> sm{actions};
		sm.process_event(StartLocalChainCheck{});

		// Act:
		sm.process_event(NetworkHeightLessThanLocal{});

		// Assert:
		EXPECT_TRUE(sm.is(boost::sml::X));
		EXPECT_EQ(1, reinterpret_cast<const CallbackInvokeCounter&>(actions.CheckLocalChain).InvokeNum);
		EXPECT_EQ(1, reinterpret_cast<const CallbackInvokeCounter&>(actions.ResetLocalChain).InvokeNum);
	}

	TEST(TEST_CLASS, WeightedVotingTransitionTable_InvalidLocalChain_Greater) {
		// Arrange:
		WeightedVotingActions actions;
		actions.CheckLocalChain = CallbackInvokeCounter{};
		actions.DownloadBlocks = CallbackInvokeCounter{};

		boost::sml::sm<WeightedVotingTransitionTable> sm{actions};
		sm.process_event(StartLocalChainCheck{});

		// Act:
		sm.process_event(NetworkHeightGreaterThanLocal{});

		// Assert:
		EXPECT_TRUE(sm.is(boost::sml::X));
		EXPECT_EQ(1, reinterpret_cast<const CallbackInvokeCounter&>(actions.CheckLocalChain).InvokeNum);
		EXPECT_EQ(1, reinterpret_cast<const CallbackInvokeCounter&>(actions.DownloadBlocks).InvokeNum);
	}

	TEST(TEST_CLASS, WeightedVotingTransitionTable_DownloadFailed) {
		// Arrange:
		WeightedVotingActions actions;
		actions.CheckLocalChain = CallbackInvokeCounter{};
		actions.DownloadBlocks = CallbackInvokeCounter{};

		boost::sml::sm<WeightedVotingTransitionTable> sm{actions};
		sm.process_event(StartLocalChainCheck{});

		// Act:
		sm.process_event(NetworkHeightGreaterThanLocal{});
		sm.process_event(DownloadBlocksFailed{});

		// Assert:
		EXPECT_TRUE(sm.is(boost::sml::X));
		EXPECT_EQ(2, reinterpret_cast<const CallbackInvokeCounter&>(actions.CheckLocalChain).InvokeNum);
		EXPECT_EQ(1, reinterpret_cast<const CallbackInvokeCounter&>(actions.DownloadBlocks).InvokeNum);
	}

	TEST(TEST_CLASS, WeightedVotingTransitionTable_DownloadSucceeded) {
		// Arrange:
		WeightedVotingActions actions;
		actions.CheckLocalChain = CallbackInvokeCounter{};
		actions.DownloadBlocks = CallbackInvokeCounter{};

		boost::sml::sm<WeightedVotingTransitionTable> sm{actions};
		sm.process_event(StartLocalChainCheck{});

		// Act:
		sm.process_event(NetworkHeightGreaterThanLocal{});
		sm.process_event(DownloadBlocksSucceeded{});

		// Assert:
		EXPECT_TRUE(sm.is(boost::sml::X));
		EXPECT_EQ(2, reinterpret_cast<const CallbackInvokeCounter&>(actions.CheckLocalChain).InvokeNum);
		EXPECT_EQ(1, reinterpret_cast<const CallbackInvokeCounter&>(actions.DownloadBlocks).InvokeNum);
	}

	TEST(TEST_CLASS, WeightedVotingTransitionTable_CommitteeSelection_ProposeBlock) {
		// Arrange:
		WeightedVotingActions actions;
		actions.ProposeBlock = CallbackInvokeCounter{};
		boost::sml::sm<WeightedVotingTransitionTable> sm{actions};

		// Act:
		CheckCommitteeSelection(sm, actions, CommitteeSelectionResult{true, CommitteePhase::Propose});
		sm.process_event(BlockProposing{});

		// Assert:
		EXPECT_EQ(1, reinterpret_cast<const CallbackInvokeCounter&>(actions.ProposeBlock).InvokeNum);
	}

	TEST(TEST_CLASS, WeightedVotingTransitionTable_CommitteeSelection_WaitForProposal) {
		// Arrange:
		WeightedVotingActions actions;
		actions.WaitForProposal = CallbackInvokeCounter{};
		boost::sml::sm<WeightedVotingTransitionTable> sm{actions};

		// Act:
		CheckCommitteeSelection(sm, actions, CommitteeSelectionResult{false, CommitteePhase::Propose});
		sm.process_event(BlockProposing{});

		// Assert:
		EXPECT_EQ(1, reinterpret_cast<const CallbackInvokeCounter&>(actions.WaitForProposal).InvokeNum);
	}

	TEST(TEST_CLASS, WeightedVotingTransitionTable_CommitteeSelection_RequestConfirmedBlock) {
		// Arrange:
		WeightedVotingActions actions;
		actions.RequestConfirmedBlock = CallbackInvokeCounter{};
		boost::sml::sm<WeightedVotingTransitionTable> sm{actions};

		// Act:
		CheckCommitteeSelection(sm, actions, CommitteeSelectionResult{false, CommitteePhase::None});
		sm.process_event(BlockProposing{});

		// Assert:
		EXPECT_EQ(1, reinterpret_cast<const CallbackInvokeCounter&>(actions.RequestConfirmedBlock).InvokeNum);
	}

	TEST(TEST_CLASS, WeightedVotingTransitionTable_BlockProposing_Failed) {
		// Arrange:
		WeightedVotingActions actions;
		actions.ProposeBlock = CallbackInvokeCounter{};
		actions.RequestConfirmedBlock = CallbackInvokeCounter{};
		boost::sml::sm<WeightedVotingTransitionTable> sm{actions};

		// Act:
		CheckCommitteeSelection(sm, actions, CommitteeSelectionResult{true, CommitteePhase::Propose});
		sm.process_event(BlockProposing{});
		sm.process_event(BlockProposingFailed{});

		// Assert:
		EXPECT_EQ(1, reinterpret_cast<const CallbackInvokeCounter&>(actions.ProposeBlock).InvokeNum);
		EXPECT_EQ(1, reinterpret_cast<const CallbackInvokeCounter&>(actions.RequestConfirmedBlock).InvokeNum);
	}

	TEST(TEST_CLASS, WeightedVotingTransitionTable_BlockProposing_Succeeded) {
		// Arrange:
		WeightedVotingActions actions;
		actions.ProposeBlock = CallbackInvokeCounter{};
		actions.AddPrevote = CallbackInvokeCounter{};
		actions.WaitForPrevotePhaseEnd = CallbackInvokeCounter{};
		boost::sml::sm<WeightedVotingTransitionTable> sm{actions};

		// Act:
		CheckCommitteeSelection(sm, actions, CommitteeSelectionResult{true, CommitteePhase::Propose});
		sm.process_event(BlockProposing{});
		sm.process_event(BlockProposingSucceeded{});
		sm.process_event(Prevote{});

		// Assert:
		EXPECT_EQ(1, reinterpret_cast<const CallbackInvokeCounter&>(actions.ProposeBlock).InvokeNum);
		EXPECT_EQ(1, reinterpret_cast<const CallbackInvokeCounter&>(actions.AddPrevote).InvokeNum);
		EXPECT_EQ(1, reinterpret_cast<const CallbackInvokeCounter&>(actions.WaitForPrevotePhaseEnd).InvokeNum);
	}

	TEST(TEST_CLASS, WeightedVotingTransitionTable_ProposalWaiting_Received) {
		// Arrange:
		WeightedVotingActions actions;
		actions.WaitForProposal = CallbackInvokeCounter{};
		actions.RequestConfirmedBlock = CallbackInvokeCounter{};
		boost::sml::sm<WeightedVotingTransitionTable> sm{actions};

		// Act:
		CheckCommitteeSelection(sm, actions, CommitteeSelectionResult{false, CommitteePhase::Propose});
		sm.process_event(BlockProposing{});
		sm.process_event(ProposalNotReceived{});

		// Assert:
		EXPECT_EQ(1, reinterpret_cast<const CallbackInvokeCounter&>(actions.WaitForProposal).InvokeNum);
		EXPECT_EQ(1, reinterpret_cast<const CallbackInvokeCounter&>(actions.RequestConfirmedBlock).InvokeNum);
	}

	TEST(TEST_CLASS, WeightedVotingTransitionTable_ProposalWaiting_NotReceived) {
		// Arrange:
		WeightedVotingActions actions;
		actions.WaitForProposal = CallbackInvokeCounter{};
		actions.ValidateProposal = CallbackInvokeCounter{};
		boost::sml::sm<WeightedVotingTransitionTable> sm{actions};

		// Act:
		CheckCommitteeSelection(sm, actions, CommitteeSelectionResult{false, CommitteePhase::Propose});
		sm.process_event(BlockProposing{});
		sm.process_event(ProposalReceived{});

		// Assert:
		EXPECT_EQ(1, reinterpret_cast<const CallbackInvokeCounter&>(actions.WaitForProposal).InvokeNum);
		EXPECT_EQ(1, reinterpret_cast<const CallbackInvokeCounter&>(actions.ValidateProposal).InvokeNum);
	}

	TEST(TEST_CLASS, WeightedVotingTransitionTable_ProposalValidation_UnexpectedBlockHeight) {
		// Arrange:
		WeightedVotingActions actions;
		actions.WaitForProposal = CallbackInvokeCounter{};
		actions.ValidateProposal = CallbackInvokeCounter{};
		boost::sml::sm<WeightedVotingTransitionTable> sm{actions};

		// Act:
		CheckCommitteeSelection(sm, actions, CommitteeSelectionResult{false, CommitteePhase::Propose});
		sm.process_event(BlockProposing{});
		sm.process_event(ProposalReceived{});
		sm.process_event(UnexpectedBlockHeight{});

		// Assert:
		EXPECT_EQ(1, reinterpret_cast<const CallbackInvokeCounter&>(actions.WaitForProposal).InvokeNum);
		EXPECT_EQ(1, reinterpret_cast<const CallbackInvokeCounter&>(actions.ValidateProposal).InvokeNum);
		EXPECT_EQ(2, reinterpret_cast<const CallbackInvokeCounter&>(actions.CheckLocalChain).InvokeNum);
	}

	TEST(TEST_CLASS, WeightedVotingTransitionTable_ProposalValidation_ProposalInvalid) {
		// Arrange:
		WeightedVotingActions actions;
		actions.WaitForProposal = CallbackInvokeCounter{};
		actions.ValidateProposal = CallbackInvokeCounter{};
		actions.RequestConfirmedBlock = CallbackInvokeCounter{};
		boost::sml::sm<WeightedVotingTransitionTable> sm{actions};

		// Act:
		CheckCommitteeSelection(sm, actions, CommitteeSelectionResult{false, CommitteePhase::Propose});
		sm.process_event(BlockProposing{});
		sm.process_event(ProposalReceived{});
		sm.process_event(ProposalInvalid{});

		// Assert:
		EXPECT_EQ(1, reinterpret_cast<const CallbackInvokeCounter&>(actions.WaitForProposal).InvokeNum);
		EXPECT_EQ(1, reinterpret_cast<const CallbackInvokeCounter&>(actions.ValidateProposal).InvokeNum);
		EXPECT_EQ(1, reinterpret_cast<const CallbackInvokeCounter&>(actions.RequestConfirmedBlock).InvokeNum);
	}

	TEST(TEST_CLASS, WeightedVotingTransitionTable_ProposalValidation_ProposalValid) {
		// Arrange:
		WeightedVotingActions actions;
		actions.WaitForProposal = CallbackInvokeCounter{};
		actions.ValidateProposal = CallbackInvokeCounter{};
		actions.AddPrevote = CallbackInvokeCounter{};
		boost::sml::sm<WeightedVotingTransitionTable> sm{actions};

		// Act:
		CheckCommitteeSelection(sm, actions, CommitteeSelectionResult{false, CommitteePhase::Propose});
		sm.process_event(BlockProposing{});
		sm.process_event(ProposalReceived{});
		sm.process_event(ProposalValid{});

		// Assert:
		EXPECT_EQ(1, reinterpret_cast<const CallbackInvokeCounter&>(actions.WaitForProposal).InvokeNum);
		EXPECT_EQ(1, reinterpret_cast<const CallbackInvokeCounter&>(actions.ValidateProposal).InvokeNum);
		EXPECT_EQ(1, reinterpret_cast<const CallbackInvokeCounter&>(actions.AddPrevote).InvokeNum);
	}

	TEST(TEST_CLASS, WeightedVotingTransitionTable_Prevote_SumOfPrevotesInsufficient) {
		// Arrange:
		WeightedVotingActions actions;
		actions.WaitForProposal = CallbackInvokeCounter{};
		actions.ValidateProposal = CallbackInvokeCounter{};
		actions.AddPrevote = CallbackInvokeCounter{};
		actions.WaitForPrevotePhaseEnd = CallbackInvokeCounter{};
		actions.RequestConfirmedBlock = CallbackInvokeCounter{};
		boost::sml::sm<WeightedVotingTransitionTable> sm{actions};

		// Act:
		CheckCommitteeSelection(sm, actions, CommitteeSelectionResult{false, CommitteePhase::Propose});
		sm.process_event(BlockProposing{});
		sm.process_event(ProposalReceived{});
		sm.process_event(ProposalValid{});
		sm.process_event(SumOfPrevotesInsufficient{});

		// Assert:
		EXPECT_EQ(1, reinterpret_cast<const CallbackInvokeCounter&>(actions.WaitForProposal).InvokeNum);
		EXPECT_EQ(1, reinterpret_cast<const CallbackInvokeCounter&>(actions.ValidateProposal).InvokeNum);
		EXPECT_EQ(1, reinterpret_cast<const CallbackInvokeCounter&>(actions.AddPrevote).InvokeNum);
		EXPECT_EQ(1, reinterpret_cast<const CallbackInvokeCounter&>(actions.WaitForPrevotePhaseEnd).InvokeNum);
		EXPECT_EQ(1, reinterpret_cast<const CallbackInvokeCounter&>(actions.RequestConfirmedBlock).InvokeNum);
	}

	TEST(TEST_CLASS, WeightedVotingTransitionTable_Prevote_SumOfPrevotesSufficient) {
		// Arrange:
		WeightedVotingActions actions;
		actions.WaitForProposal = CallbackInvokeCounter{};
		actions.ValidateProposal = CallbackInvokeCounter{};
		actions.AddPrevote = CallbackInvokeCounter{};
		actions.WaitForPrevotePhaseEnd = CallbackInvokeCounter{};
		actions.AddPrecommit = CallbackInvokeCounter{};
		actions.WaitForPrecommitPhaseEnd = CallbackInvokeCounter{};
		boost::sml::sm<WeightedVotingTransitionTable> sm{actions};

		// Act:
		CheckCommitteeSelection(sm, actions, CommitteeSelectionResult{false, CommitteePhase::Propose});
		sm.process_event(BlockProposing{});
		sm.process_event(ProposalReceived{});
		sm.process_event(ProposalValid{});
		sm.process_event(SumOfPrevotesSufficient{});

		// Assert:
		EXPECT_EQ(1, reinterpret_cast<const CallbackInvokeCounter&>(actions.WaitForProposal).InvokeNum);
		EXPECT_EQ(1, reinterpret_cast<const CallbackInvokeCounter&>(actions.ValidateProposal).InvokeNum);
		EXPECT_EQ(1, reinterpret_cast<const CallbackInvokeCounter&>(actions.AddPrevote).InvokeNum);
		EXPECT_EQ(1, reinterpret_cast<const CallbackInvokeCounter&>(actions.WaitForPrevotePhaseEnd).InvokeNum);
		EXPECT_EQ(1, reinterpret_cast<const CallbackInvokeCounter&>(actions.AddPrecommit).InvokeNum);
		EXPECT_EQ(1, reinterpret_cast<const CallbackInvokeCounter&>(actions.WaitForPrecommitPhaseEnd).InvokeNum);
	}

	TEST(TEST_CLASS, WeightedVotingTransitionTable_Precommit_SumOfPrecommitsInsufficient) {
		// Arrange:
		WeightedVotingActions actions;
		actions.WaitForProposal = CallbackInvokeCounter{};
		actions.ValidateProposal = CallbackInvokeCounter{};
		actions.AddPrevote = CallbackInvokeCounter{};
		actions.WaitForPrevotePhaseEnd = CallbackInvokeCounter{};
		actions.AddPrecommit = CallbackInvokeCounter{};
		actions.WaitForPrecommitPhaseEnd = CallbackInvokeCounter{};
		actions.RequestConfirmedBlock = CallbackInvokeCounter{};
		boost::sml::sm<WeightedVotingTransitionTable> sm{actions};

		// Act:
		CheckCommitteeSelection(sm, actions, CommitteeSelectionResult{false, CommitteePhase::Propose});
		sm.process_event(BlockProposing{});
		sm.process_event(ProposalReceived{});
		sm.process_event(ProposalValid{});
		sm.process_event(SumOfPrevotesSufficient{});
		sm.process_event(SumOfPrecommitsInsufficient{});

		// Assert:
		EXPECT_EQ(1, reinterpret_cast<const CallbackInvokeCounter&>(actions.WaitForProposal).InvokeNum);
		EXPECT_EQ(1, reinterpret_cast<const CallbackInvokeCounter&>(actions.ValidateProposal).InvokeNum);
		EXPECT_EQ(1, reinterpret_cast<const CallbackInvokeCounter&>(actions.AddPrevote).InvokeNum);
		EXPECT_EQ(1, reinterpret_cast<const CallbackInvokeCounter&>(actions.WaitForPrevotePhaseEnd).InvokeNum);
		EXPECT_EQ(1, reinterpret_cast<const CallbackInvokeCounter&>(actions.AddPrecommit).InvokeNum);
		EXPECT_EQ(1, reinterpret_cast<const CallbackInvokeCounter&>(actions.WaitForPrecommitPhaseEnd).InvokeNum);
		EXPECT_EQ(1, reinterpret_cast<const CallbackInvokeCounter&>(actions.RequestConfirmedBlock).InvokeNum);
	}

	TEST(TEST_CLASS, WeightedVotingTransitionTable_Precommit_SumOfPrecommitsSufficient) {
		// Arrange:
		WeightedVotingActions actions;
		actions.WaitForProposal = CallbackInvokeCounter{};
		actions.ValidateProposal = CallbackInvokeCounter{};
		actions.AddPrevote = CallbackInvokeCounter{};
		actions.WaitForPrevotePhaseEnd = CallbackInvokeCounter{};
		actions.AddPrecommit = CallbackInvokeCounter{};
		actions.WaitForPrecommitPhaseEnd = CallbackInvokeCounter{};
		actions.UpdateConfirmedBlock = CallbackInvokeCounter{};
		actions.CommitConfirmedBlock = CallbackInvokeCounter{};
		boost::sml::sm<WeightedVotingTransitionTable> sm{actions};

		// Act:
		CheckCommitteeSelection(sm, actions, CommitteeSelectionResult{false, CommitteePhase::Propose});
		sm.process_event(BlockProposing{});
		sm.process_event(ProposalReceived{});
		sm.process_event(ProposalValid{});
		sm.process_event(SumOfPrevotesSufficient{});
		sm.process_event(SumOfPrecommitsSufficient{});

		// Assert:
		EXPECT_EQ(1, reinterpret_cast<const CallbackInvokeCounter&>(actions.WaitForProposal).InvokeNum);
		EXPECT_EQ(1, reinterpret_cast<const CallbackInvokeCounter&>(actions.ValidateProposal).InvokeNum);
		EXPECT_EQ(1, reinterpret_cast<const CallbackInvokeCounter&>(actions.AddPrevote).InvokeNum);
		EXPECT_EQ(1, reinterpret_cast<const CallbackInvokeCounter&>(actions.WaitForPrevotePhaseEnd).InvokeNum);
		EXPECT_EQ(1, reinterpret_cast<const CallbackInvokeCounter&>(actions.AddPrecommit).InvokeNum);
		EXPECT_EQ(1, reinterpret_cast<const CallbackInvokeCounter&>(actions.WaitForPrecommitPhaseEnd).InvokeNum);
		EXPECT_EQ(1, reinterpret_cast<const CallbackInvokeCounter&>(actions.UpdateConfirmedBlock).InvokeNum);
		EXPECT_EQ(1, reinterpret_cast<const CallbackInvokeCounter&>(actions.CommitConfirmedBlock).InvokeNum);
	}

	TEST(TEST_CLASS, WeightedVotingTransitionTable_CommitBlockFailed) {
		// Arrange:
		WeightedVotingActions actions;
		actions.WaitForProposal = CallbackInvokeCounter{};
		actions.ValidateProposal = CallbackInvokeCounter{};
		actions.AddPrevote = CallbackInvokeCounter{};
		actions.WaitForPrevotePhaseEnd = CallbackInvokeCounter{};
		actions.AddPrecommit = CallbackInvokeCounter{};
		actions.WaitForPrecommitPhaseEnd = CallbackInvokeCounter{};
		actions.UpdateConfirmedBlock = CallbackInvokeCounter{};
		actions.CommitConfirmedBlock = CallbackInvokeCounter{};
		actions.IncrementRound = CallbackInvokeCounter{};
		boost::sml::sm<WeightedVotingTransitionTable> sm{actions};

		// Act:
		CheckCommitteeSelection(sm, actions, CommitteeSelectionResult{false, CommitteePhase::Propose});
		sm.process_event(BlockProposing{});
		sm.process_event(ProposalReceived{});
		sm.process_event(ProposalValid{});
		sm.process_event(SumOfPrevotesSufficient{});
		sm.process_event(SumOfPrecommitsSufficient{});
		sm.process_event(CommitBlockFailed{});

		// Assert:
		EXPECT_EQ(1, reinterpret_cast<const CallbackInvokeCounter&>(actions.WaitForProposal).InvokeNum);
		EXPECT_EQ(1, reinterpret_cast<const CallbackInvokeCounter&>(actions.ValidateProposal).InvokeNum);
		EXPECT_EQ(1, reinterpret_cast<const CallbackInvokeCounter&>(actions.AddPrevote).InvokeNum);
		EXPECT_EQ(1, reinterpret_cast<const CallbackInvokeCounter&>(actions.WaitForPrevotePhaseEnd).InvokeNum);
		EXPECT_EQ(1, reinterpret_cast<const CallbackInvokeCounter&>(actions.AddPrecommit).InvokeNum);
		EXPECT_EQ(1, reinterpret_cast<const CallbackInvokeCounter&>(actions.WaitForPrecommitPhaseEnd).InvokeNum);
		EXPECT_EQ(1, reinterpret_cast<const CallbackInvokeCounter&>(actions.UpdateConfirmedBlock).InvokeNum);
		EXPECT_EQ(1, reinterpret_cast<const CallbackInvokeCounter&>(actions.CommitConfirmedBlock).InvokeNum);
		EXPECT_EQ(1, reinterpret_cast<const CallbackInvokeCounter&>(actions.IncrementRound).InvokeNum);
		EXPECT_EQ(2, reinterpret_cast<const CallbackInvokeCounter&>(actions.SelectCommittee).InvokeNum);
	}

	TEST(TEST_CLASS, WeightedVotingTransitionTable_CommitBlockSucceeded) {
		// Arrange:
		WeightedVotingActions actions;
		actions.WaitForProposal = CallbackInvokeCounter{};
		actions.ValidateProposal = CallbackInvokeCounter{};
		actions.AddPrevote = CallbackInvokeCounter{};
		actions.WaitForPrevotePhaseEnd = CallbackInvokeCounter{};
		actions.AddPrecommit = CallbackInvokeCounter{};
		actions.WaitForPrecommitPhaseEnd = CallbackInvokeCounter{};
		actions.UpdateConfirmedBlock = CallbackInvokeCounter{};
		actions.CommitConfirmedBlock = CallbackInvokeCounter{};
		actions.ResetRound = CallbackInvokeCounter{};
		boost::sml::sm<WeightedVotingTransitionTable> sm{actions};

		// Act:
		CheckCommitteeSelection(sm, actions, CommitteeSelectionResult{false, CommitteePhase::Propose});
		sm.process_event(BlockProposing{});
		sm.process_event(ProposalReceived{});
		sm.process_event(ProposalValid{});
		sm.process_event(SumOfPrevotesSufficient{});
		sm.process_event(SumOfPrecommitsSufficient{});
		sm.process_event(CommitBlockSucceeded{});

		// Assert:
		EXPECT_EQ(1, reinterpret_cast<const CallbackInvokeCounter&>(actions.WaitForProposal).InvokeNum);
		EXPECT_EQ(1, reinterpret_cast<const CallbackInvokeCounter&>(actions.ValidateProposal).InvokeNum);
		EXPECT_EQ(1, reinterpret_cast<const CallbackInvokeCounter&>(actions.AddPrevote).InvokeNum);
		EXPECT_EQ(1, reinterpret_cast<const CallbackInvokeCounter&>(actions.WaitForPrevotePhaseEnd).InvokeNum);
		EXPECT_EQ(1, reinterpret_cast<const CallbackInvokeCounter&>(actions.AddPrecommit).InvokeNum);
		EXPECT_EQ(1, reinterpret_cast<const CallbackInvokeCounter&>(actions.WaitForPrecommitPhaseEnd).InvokeNum);
		EXPECT_EQ(1, reinterpret_cast<const CallbackInvokeCounter&>(actions.UpdateConfirmedBlock).InvokeNum);
		EXPECT_EQ(1, reinterpret_cast<const CallbackInvokeCounter&>(actions.CommitConfirmedBlock).InvokeNum);
		EXPECT_EQ(1, reinterpret_cast<const CallbackInvokeCounter&>(actions.ResetRound).InvokeNum);
		EXPECT_EQ(2, reinterpret_cast<const CallbackInvokeCounter&>(actions.SelectCommittee).InvokeNum);
	}

	TEST(TEST_CLASS, WeightedVotingTransitionTable_ConfirmedBlockRequest_UnexpectedBlockHeight) {
		// Arrange:
		WeightedVotingActions actions;
		actions.RequestConfirmedBlock = CallbackInvokeCounter{};
		boost::sml::sm<WeightedVotingTransitionTable> sm{actions};

		// Act:
		CheckCommitteeSelection(sm, actions, CommitteeSelectionResult{false, CommitteePhase::None});
		sm.process_event(BlockProposing{});
		sm.process_event(UnexpectedBlockHeight{});

		// Assert:
		EXPECT_EQ(1, reinterpret_cast<const CallbackInvokeCounter&>(actions.RequestConfirmedBlock).InvokeNum);
		EXPECT_EQ(2, reinterpret_cast<const CallbackInvokeCounter&>(actions.CheckLocalChain).InvokeNum);
	}

	TEST(TEST_CLASS, WeightedVotingTransitionTable_ConfirmedBlockRequest_ConfirmedBlockNotReceived) {
		// Arrange:
		WeightedVotingActions actions;
		actions.RequestConfirmedBlock = CallbackInvokeCounter{};
		actions.IncrementRound = CallbackInvokeCounter{};
		actions.SelectCommittee = CallbackInvokeCounter{};
		boost::sml::sm<WeightedVotingTransitionTable> sm{actions};

		// Act:
		CheckCommitteeSelection(sm, actions, CommitteeSelectionResult{false, CommitteePhase::None});
		sm.process_event(BlockProposing{});
		sm.process_event(ConfirmedBlockNotReceived{});

		// Assert:
		EXPECT_EQ(1, reinterpret_cast<const CallbackInvokeCounter&>(actions.RequestConfirmedBlock).InvokeNum);
		EXPECT_EQ(1, reinterpret_cast<const CallbackInvokeCounter&>(actions.IncrementRound).InvokeNum);
		EXPECT_EQ(1, reinterpret_cast<const CallbackInvokeCounter&>(actions.SelectCommittee).InvokeNum);
	}

	TEST(TEST_CLASS, WeightedVotingTransitionTable_ConfirmedBlockRequest_ConfirmedBlockReceived) {
		// Arrange:
		WeightedVotingActions actions;
		actions.RequestConfirmedBlock = CallbackInvokeCounter{};
		actions.CommitConfirmedBlock = CallbackInvokeCounter{};
		boost::sml::sm<WeightedVotingTransitionTable> sm{actions};

		// Act:
		CheckCommitteeSelection(sm, actions, CommitteeSelectionResult{false, CommitteePhase::None});
		sm.process_event(BlockProposing{});
		sm.process_event(ConfirmedBlockReceived{});

		// Assert:
		EXPECT_EQ(1, reinterpret_cast<const CallbackInvokeCounter&>(actions.RequestConfirmedBlock).InvokeNum);
		EXPECT_EQ(1, reinterpret_cast<const CallbackInvokeCounter&>(actions.CommitConfirmedBlock).InvokeNum);
	}

}}
