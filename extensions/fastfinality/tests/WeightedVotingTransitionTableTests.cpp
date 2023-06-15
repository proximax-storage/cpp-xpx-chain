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

		void ArrangeCommitteeSelectionActions(WeightedVotingActions& actions) {
            // Arrange:
            actions.CheckLocalChain = CallbackInvokeCounter{};
            actions.DetectStage = CallbackInvokeCounter{};
            actions.SelectCommittee = CallbackInvokeCounter{};
		}

		void ProcessCommitteeSelectionEvents(boost::sml::sm<WeightedVotingTransitionTable>& sm, const CommitteeSelectionResult& e) {
            // Arrange:
            sm.process_event(StartLocalChainCheck{});
            sm.process_event(NetworkHeightEqualToLocal{});
            sm.process_event(StageDetectionSucceeded{});
            sm.process_event(e);
		}

		void CheckCommitteeSelectionActions(const WeightedVotingActions& actions) {
			// Assert:
			EXPECT_EQ(1, reinterpret_cast<const CallbackInvokeCounter&>(actions.CheckLocalChain).InvokeNum);
			EXPECT_EQ(1, reinterpret_cast<const CallbackInvokeCounter&>(actions.DetectStage).InvokeNum);
			EXPECT_EQ(1, reinterpret_cast<const CallbackInvokeCounter&>(actions.SelectCommittee).InvokeNum);
		}

		void ArrangeProposalWaitingActions(WeightedVotingActions& actions) {
            // Arrange:
            ArrangeCommitteeSelectionActions(actions);
            actions.WaitForProposal = CallbackInvokeCounter{};
		}

		void ProcessProposalWaitingEvents(boost::sml::sm<WeightedVotingTransitionTable>& sm) {
            // Arrange:
            ProcessCommitteeSelectionEvents(sm, CommitteeSelectionResult{false, CommitteePhase::Propose});
            sm.process_event(BlockProposing{});
		}

		void CheckProposalWaitingActions(const WeightedVotingActions& actions) {
			// Assert:
            EXPECT_EQ(1, reinterpret_cast<const CallbackInvokeCounter&>(actions.WaitForProposal).InvokeNum);
            CheckCommitteeSelectionActions(actions);
		}

		void ArrangeProposalValidationActions(WeightedVotingActions& actions) {
            // Arrange:
            ArrangeProposalWaitingActions(actions);
            actions.ValidateProposal = CallbackInvokeCounter{};
		}

		void ProcessProposalValidationEvents(boost::sml::sm<WeightedVotingTransitionTable>& sm) {
            // Arrange:
            ProcessProposalWaitingEvents(sm);
            sm.process_event(BlockProposing{});
            sm.process_event(ProposalReceived{});
		}

		void CheckProposalValidationActions(const WeightedVotingActions& actions) {
			// Assert:
            CheckProposalWaitingActions(actions);
            EXPECT_EQ(1, reinterpret_cast<const CallbackInvokeCounter&>(actions.ValidateProposal).InvokeNum);
		}

		void ArrangePrevoteActions(WeightedVotingActions& actions) {
            // Arrange:
            ArrangeProposalValidationActions(actions);
            actions.AddPrevote = CallbackInvokeCounter{};
            actions.WaitForPrevotePhaseEnd = CallbackInvokeCounter{};
		}

		void ProcessPrevoteEvents(boost::sml::sm<WeightedVotingTransitionTable>& sm) {
            // Arrange:
            ProcessProposalValidationEvents(sm);
            sm.process_event(ProposalValid{});
		}

		void CheckPrevoteActions(const WeightedVotingActions& actions) {
			// Assert:
            CheckProposalWaitingActions(actions);
            EXPECT_EQ(1, reinterpret_cast<const CallbackInvokeCounter&>(actions.AddPrevote).InvokeNum);
            EXPECT_EQ(1, reinterpret_cast<const CallbackInvokeCounter&>(actions.WaitForPrevotePhaseEnd).InvokeNum);
		}

		void ArrangePrecommitActions(WeightedVotingActions& actions) {
            // Arrange:
            ArrangePrevoteActions(actions);
            actions.AddPrecommit = CallbackInvokeCounter{};
            actions.WaitForPrecommitPhaseEnd = CallbackInvokeCounter{};
		}

		void ProcessPrecommitEvents(boost::sml::sm<WeightedVotingTransitionTable>& sm) {
            // Arrange:
            ProcessPrevoteEvents(sm);
            sm.process_event(SumOfPrevotesSufficient{});
		}

		void CheckPrecommitActions(const WeightedVotingActions& actions) {
			// Assert:
            CheckPrevoteActions(actions);
            EXPECT_EQ(1, reinterpret_cast<const CallbackInvokeCounter&>(actions.AddPrecommit).InvokeNum);
            EXPECT_EQ(1, reinterpret_cast<const CallbackInvokeCounter&>(actions.WaitForPrecommitPhaseEnd).InvokeNum);
		}

		void ArrangeCommitActions(WeightedVotingActions& actions) {
            // Arrange:
            ArrangePrecommitActions(actions);
            actions.UpdateConfirmedBlock = CallbackInvokeCounter{};
            actions.CommitConfirmedBlock = CallbackInvokeCounter{};
		}

		void ProcessCommitEvents(boost::sml::sm<WeightedVotingTransitionTable>& sm) {
            // Arrange:
            ProcessPrecommitEvents(sm);
            sm.process_event(SumOfPrecommitsSufficient{});
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

	TEST(TEST_CLASS, WeightedVotingTransitionTable_StopWaiting) {
		// Arrange:
		WeightedVotingActions actions;

		// Act:
		boost::sml::sm<WeightedVotingTransitionTable> sm{actions};
        sm.process_event(StopWaiting{});

		// Assert:
        EXPECT_TRUE(sm.is(boost::sml::X));
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

	TEST(TEST_CLASS, WeightedVotingTransitionTable_NetworkHeightLessThanLocal) {
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

	TEST(TEST_CLASS, WeightedVotingTransitionTable_NetworkHeightGreaterThanLocal) {
		// Arrange:
		WeightedVotingActions actions;
		actions.CheckLocalChain = CallbackInvokeCounter{};
		actions.DownloadBlocks = CallbackInvokeCounter{};

		// Act:
		boost::sml::sm<WeightedVotingTransitionTable> sm{actions};
		sm.process_event(StartLocalChainCheck{});
		sm.process_event(NetworkHeightGreaterThanLocal{});

		// Assert:
		EXPECT_TRUE(sm.is(boost::sml::state<BlocksDownloading>));
		EXPECT_EQ(1, reinterpret_cast<const CallbackInvokeCounter&>(actions.CheckLocalChain).InvokeNum);
		EXPECT_EQ(1, reinterpret_cast<const CallbackInvokeCounter&>(actions.DownloadBlocks).InvokeNum);
	}

	TEST(TEST_CLASS, WeightedVotingTransitionTable_StageDetectionSucceeded) {
		// Arrange:
		WeightedVotingActions actions;
		actions.CheckLocalChain = CallbackInvokeCounter{};
		actions.DetectStage = CallbackInvokeCounter{};

		// Act:
		boost::sml::sm<WeightedVotingTransitionTable> sm{actions};
		sm.process_event(StartLocalChainCheck{});
		sm.process_event(NetworkHeightEqualToLocal{});
		sm.process_event(StageDetectionSucceeded{});

		// Assert:
		EXPECT_TRUE(sm.is(boost::sml::state<CommitteeSelection>));
		EXPECT_EQ(1, reinterpret_cast<const CallbackInvokeCounter&>(actions.CheckLocalChain).InvokeNum);
		EXPECT_EQ(1, reinterpret_cast<const CallbackInvokeCounter&>(actions.DetectStage).InvokeNum);
	}

	TEST(TEST_CLASS, WeightedVotingTransitionTable_DownloadFailed) {
		// Arrange:
		WeightedVotingActions actions;
		actions.CheckLocalChain = CallbackInvokeCounter{};
		actions.DownloadBlocks = CallbackInvokeCounter{};

		// Act:
		boost::sml::sm<WeightedVotingTransitionTable> sm{actions};
		sm.process_event(StartLocalChainCheck{});
		sm.process_event(NetworkHeightGreaterThanLocal{});
		sm.process_event(DownloadBlocksFailed{});

		// Assert:
		EXPECT_TRUE(sm.is(boost::sml::state<LocalChainCheck>));
		EXPECT_EQ(2, reinterpret_cast<const CallbackInvokeCounter&>(actions.CheckLocalChain).InvokeNum);
		EXPECT_EQ(1, reinterpret_cast<const CallbackInvokeCounter&>(actions.DownloadBlocks).InvokeNum);
	}

	TEST(TEST_CLASS, WeightedVotingTransitionTable_DownloadSucceeded) {
		// Arrange:
		WeightedVotingActions actions;
		actions.CheckLocalChain = CallbackInvokeCounter{};
		actions.DownloadBlocks = CallbackInvokeCounter{};

		// Act:
		boost::sml::sm<WeightedVotingTransitionTable> sm{actions};
		sm.process_event(StartLocalChainCheck{});
		sm.process_event(NetworkHeightGreaterThanLocal{});
		sm.process_event(DownloadBlocksSucceeded{});

		// Assert:
		EXPECT_TRUE(sm.is(boost::sml::state<LocalChainCheck>));
		EXPECT_EQ(2, reinterpret_cast<const CallbackInvokeCounter&>(actions.CheckLocalChain).InvokeNum);
		EXPECT_EQ(1, reinterpret_cast<const CallbackInvokeCounter&>(actions.DownloadBlocks).InvokeNum);
	}

	TEST(TEST_CLASS, WeightedVotingTransitionTable_CommitteeSelection_ProposeBlock) {
		// Arrange:
		WeightedVotingActions actions;
        ArrangeCommitteeSelectionActions(actions);
		actions.ProposeBlock = CallbackInvokeCounter{};

		// Act:
        boost::sml::sm<WeightedVotingTransitionTable> sm{actions};
        ProcessCommitteeSelectionEvents(sm, CommitteeSelectionResult{true, CommitteePhase::Propose});
        sm.process_event(BlockProposing{});

        // Assert:
        EXPECT_TRUE(sm.is(boost::sml::state<BlockProposing>));
		EXPECT_EQ(1, reinterpret_cast<const CallbackInvokeCounter&>(actions.ProposeBlock).InvokeNum);
        CheckCommitteeSelectionActions(actions);
	}

	TEST(TEST_CLASS, WeightedVotingTransitionTable_CommitteeSelection_WaitForProposal) {
        // Arrange:
        WeightedVotingActions actions;
        ArrangeCommitteeSelectionActions(actions);
        actions.WaitForProposal = CallbackInvokeCounter{};

        // Act:
        boost::sml::sm<WeightedVotingTransitionTable> sm{actions};
        ProcessCommitteeSelectionEvents(sm, CommitteeSelectionResult{false, CommitteePhase::Propose});
        sm.process_event(BlockProposing{});

		// Assert:
		EXPECT_TRUE(sm.is(boost::sml::state<ProposalWaiting>));
		EXPECT_EQ(1, reinterpret_cast<const CallbackInvokeCounter&>(actions.WaitForProposal).InvokeNum);
        CheckCommitteeSelectionActions(actions);
	}

	TEST(TEST_CLASS, WeightedVotingTransitionTable_CommitteeSelection_RequestConfirmedBlock) {
		// Arrange:
		WeightedVotingActions actions;
        ArrangeCommitteeSelectionActions(actions);
		actions.RequestConfirmedBlock = CallbackInvokeCounter{};

        // Act:
        boost::sml::sm<WeightedVotingTransitionTable> sm{actions};
        ProcessCommitteeSelectionEvents(sm, CommitteeSelectionResult{false, CommitteePhase::None});
        sm.process_event(BlockProposing{});

		// Assert:
		EXPECT_TRUE(sm.is(boost::sml::state<ConfirmedBlockRequest>));
		EXPECT_EQ(1, reinterpret_cast<const CallbackInvokeCounter&>(actions.RequestConfirmedBlock).InvokeNum);
        CheckCommitteeSelectionActions(actions);
	}

	TEST(TEST_CLASS, WeightedVotingTransitionTable_BlockProposing_Failed) {
		// Arrange:
		WeightedVotingActions actions;
        ArrangeCommitteeSelectionActions(actions);
		actions.ProposeBlock = CallbackInvokeCounter{};
		actions.RequestConfirmedBlock = CallbackInvokeCounter{};

		// Act:
        boost::sml::sm<WeightedVotingTransitionTable> sm{actions};
        ProcessCommitteeSelectionEvents(sm, CommitteeSelectionResult{true, CommitteePhase::Propose});
		sm.process_event(BlockProposing{});
		sm.process_event(BlockProposingFailed{});

		// Assert:
		EXPECT_TRUE(sm.is(boost::sml::state<ConfirmedBlockRequest>));
		EXPECT_EQ(1, reinterpret_cast<const CallbackInvokeCounter&>(actions.ProposeBlock).InvokeNum);
		EXPECT_EQ(1, reinterpret_cast<const CallbackInvokeCounter&>(actions.RequestConfirmedBlock).InvokeNum);
        CheckCommitteeSelectionActions(actions);
	}

	TEST(TEST_CLASS, WeightedVotingTransitionTable_BlockProposing_Succeeded) {
		// Arrange:
		WeightedVotingActions actions;
        ArrangeCommitteeSelectionActions(actions);
		actions.ProposeBlock = CallbackInvokeCounter{};
		actions.AddPrevote = CallbackInvokeCounter{};
		actions.WaitForPrevotePhaseEnd = CallbackInvokeCounter{};

		// Act:
        boost::sml::sm<WeightedVotingTransitionTable> sm{actions};
        ProcessCommitteeSelectionEvents(sm, CommitteeSelectionResult{true, CommitteePhase::Propose});
		sm.process_event(BlockProposing{});
		sm.process_event(BlockProposingSucceeded{});
		sm.process_event(Prevote{});

		// Assert:
		EXPECT_TRUE(sm.is(boost::sml::state<Prevote>));
		EXPECT_EQ(1, reinterpret_cast<const CallbackInvokeCounter&>(actions.ProposeBlock).InvokeNum);
		EXPECT_EQ(1, reinterpret_cast<const CallbackInvokeCounter&>(actions.AddPrevote).InvokeNum);
		EXPECT_EQ(1, reinterpret_cast<const CallbackInvokeCounter&>(actions.WaitForPrevotePhaseEnd).InvokeNum);
        CheckCommitteeSelectionActions(actions);
	}

	TEST(TEST_CLASS, WeightedVotingTransitionTable_ProposalWaiting_NotReceived) {
		// Arrange:
		WeightedVotingActions actions;
        ArrangeProposalWaitingActions(actions);
		actions.RequestConfirmedBlock = CallbackInvokeCounter{};

		// Act:
        boost::sml::sm<WeightedVotingTransitionTable> sm{actions};
        ProcessProposalWaitingEvents(sm);
		sm.process_event(ProposalNotReceived{});

		// Assert:
        EXPECT_TRUE(sm.is(boost::sml::state<ConfirmedBlockRequest>));
		EXPECT_EQ(1, reinterpret_cast<const CallbackInvokeCounter&>(actions.RequestConfirmedBlock).InvokeNum);
        CheckProposalWaitingActions(actions);
	}

	TEST(TEST_CLASS, WeightedVotingTransitionTable_ProposalWaiting_Received) {
		// Arrange:
		WeightedVotingActions actions;
        ArrangeProposalWaitingActions(actions);
		actions.ValidateProposal = CallbackInvokeCounter{};

		// Act:
        boost::sml::sm<WeightedVotingTransitionTable> sm{actions};
        ProcessProposalWaitingEvents(sm);
		sm.process_event(ProposalReceived{});

		// Assert:
        EXPECT_TRUE(sm.is(boost::sml::state<ProposalValidation>));
		EXPECT_EQ(1, reinterpret_cast<const CallbackInvokeCounter&>(actions.ValidateProposal).InvokeNum);
        CheckProposalWaitingActions(actions);
    }

	TEST(TEST_CLASS, WeightedVotingTransitionTable_ProposalValidation_UnexpectedBlockHeight) {
		// Arrange:
		WeightedVotingActions actions;
        ArrangeProposalValidationActions(actions);

		// Act:
        boost::sml::sm<WeightedVotingTransitionTable> sm{actions};
        ProcessProposalValidationEvents(sm);
		sm.process_event(UnexpectedBlockHeight{});

		// Assert:
        EXPECT_TRUE(sm.is(boost::sml::state<LocalChainCheck>));
		EXPECT_EQ(2, reinterpret_cast<const CallbackInvokeCounter&>(actions.CheckLocalChain).InvokeNum);
        EXPECT_EQ(1, reinterpret_cast<const CallbackInvokeCounter&>(actions.DetectStage).InvokeNum);
        EXPECT_EQ(1, reinterpret_cast<const CallbackInvokeCounter&>(actions.SelectCommittee).InvokeNum);
        EXPECT_EQ(1, reinterpret_cast<const CallbackInvokeCounter&>(actions.WaitForProposal).InvokeNum);
        EXPECT_EQ(1, reinterpret_cast<const CallbackInvokeCounter&>(actions.ValidateProposal).InvokeNum);
    }

	TEST(TEST_CLASS, WeightedVotingTransitionTable_ProposalValidation_ProposalInvalid) {
		// Arrange:
		WeightedVotingActions actions;
        ArrangeProposalValidationActions(actions);
		actions.RequestConfirmedBlock = CallbackInvokeCounter{};

		// Act:
        boost::sml::sm<WeightedVotingTransitionTable> sm{actions};
        ProcessProposalValidationEvents(sm);
		sm.process_event(ProposalInvalid{});

		// Assert:
        CheckProposalValidationActions(actions);
        EXPECT_TRUE(sm.is(boost::sml::state<ConfirmedBlockRequest>));
		EXPECT_EQ(1, reinterpret_cast<const CallbackInvokeCounter&>(actions.RequestConfirmedBlock).InvokeNum);
	}

	TEST(TEST_CLASS, WeightedVotingTransitionTable_ProposalValidation_ProposalValid) {
		// Arrange:
		WeightedVotingActions actions;
        ArrangeProposalValidationActions(actions);
		actions.AddPrevote = CallbackInvokeCounter{};

		// Act:
        boost::sml::sm<WeightedVotingTransitionTable> sm{actions};
        ProcessProposalValidationEvents(sm);
		sm.process_event(ProposalValid{});

		// Assert:
        CheckProposalValidationActions(actions);
        EXPECT_TRUE(sm.is(boost::sml::state<Prevote>));
		EXPECT_EQ(1, reinterpret_cast<const CallbackInvokeCounter&>(actions.AddPrevote).InvokeNum);
	}

	TEST(TEST_CLASS, WeightedVotingTransitionTable_Prevote_SumOfPrevotesInsufficient) {
		// Arrange:
		WeightedVotingActions actions;
        ArrangePrevoteActions(actions);
		actions.RequestConfirmedBlock = CallbackInvokeCounter{};

		// Act:
        boost::sml::sm<WeightedVotingTransitionTable> sm{actions};
        ProcessPrevoteEvents(sm);
		sm.process_event(SumOfPrevotesInsufficient{});

		// Assert:
        CheckPrevoteActions(actions);
        EXPECT_TRUE(sm.is(boost::sml::state<ConfirmedBlockRequest>));
        EXPECT_EQ(1, reinterpret_cast<const CallbackInvokeCounter&>(actions.RequestConfirmedBlock).InvokeNum);
	}

	TEST(TEST_CLASS, WeightedVotingTransitionTable_Prevote_SumOfPrevotesSufficient) {
		// Arrange:
		WeightedVotingActions actions;
        ArrangePrevoteActions(actions);
		actions.AddPrecommit = CallbackInvokeCounter{};
		actions.WaitForPrecommitPhaseEnd = CallbackInvokeCounter{};

		// Act:
        boost::sml::sm<WeightedVotingTransitionTable> sm{actions};
        ProcessPrevoteEvents(sm);
		sm.process_event(SumOfPrevotesSufficient{});

        // Assert:
        CheckPrevoteActions(actions);
        EXPECT_TRUE(sm.is(boost::sml::state<Precommit>));
        EXPECT_EQ(1, reinterpret_cast<const CallbackInvokeCounter&>(actions.AddPrecommit).InvokeNum);
        EXPECT_EQ(1, reinterpret_cast<const CallbackInvokeCounter&>(actions.WaitForPrecommitPhaseEnd).InvokeNum);
	}

	TEST(TEST_CLASS, WeightedVotingTransitionTable_Precommit_SumOfPrecommitsInsufficient) {
		// Arrange:
		WeightedVotingActions actions;
        ArrangePrecommitActions(actions);
		actions.RequestConfirmedBlock = CallbackInvokeCounter{};

		// Act:
        boost::sml::sm<WeightedVotingTransitionTable> sm{actions};
        ProcessPrecommitEvents(sm);
		sm.process_event(SumOfPrecommitsInsufficient{});

		// Assert:
        CheckPrecommitActions(actions);
        EXPECT_TRUE(sm.is(boost::sml::state<ConfirmedBlockRequest>));
		EXPECT_EQ(1, reinterpret_cast<const CallbackInvokeCounter&>(actions.RequestConfirmedBlock).InvokeNum);
	}

	TEST(TEST_CLASS, WeightedVotingTransitionTable_Precommit_SumOfPrecommitsSufficient) {
		// Arrange:
		WeightedVotingActions actions;
        ArrangePrecommitActions(actions);
		actions.UpdateConfirmedBlock = CallbackInvokeCounter{};
		actions.CommitConfirmedBlock = CallbackInvokeCounter{};

		// Act:
        boost::sml::sm<WeightedVotingTransitionTable> sm{actions};
        ProcessPrecommitEvents(sm);
		sm.process_event(SumOfPrecommitsSufficient{});

		// Assert:
        CheckPrecommitActions(actions);
        EXPECT_TRUE(sm.is(boost::sml::state<Commit>));
		EXPECT_EQ(1, reinterpret_cast<const CallbackInvokeCounter&>(actions.UpdateConfirmedBlock).InvokeNum);
		EXPECT_EQ(1, reinterpret_cast<const CallbackInvokeCounter&>(actions.CommitConfirmedBlock).InvokeNum);
	}

	TEST(TEST_CLASS, WeightedVotingTransitionTable_CommitBlockFailed) {
		// Arrange:
		WeightedVotingActions actions;
        ArrangeCommitActions(actions);
		actions.IncrementRound = CallbackInvokeCounter{};

		// Act:
        boost::sml::sm<WeightedVotingTransitionTable> sm{actions};
        ProcessCommitEvents(sm);
		sm.process_event(CommitBlockFailed{});

		// Assert:
        EXPECT_TRUE(sm.is(boost::sml::state<CommitteeSelection>));
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
        ArrangeCommitActions(actions);
		actions.ResetRound = CallbackInvokeCounter{};

		// Act:
        boost::sml::sm<WeightedVotingTransitionTable> sm{actions};
        ProcessCommitEvents(sm);
		sm.process_event(CommitBlockSucceeded{});

		// Assert:
        EXPECT_TRUE(sm.is(boost::sml::state<CommitteeSelection>));
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
        ArrangeCommitteeSelectionActions(actions);
		actions.RequestConfirmedBlock = CallbackInvokeCounter{};

		// Act:
        boost::sml::sm<WeightedVotingTransitionTable> sm{actions};
        ProcessCommitteeSelectionEvents(sm, CommitteeSelectionResult{false, CommitteePhase::None});
		sm.process_event(BlockProposing{});
		sm.process_event(UnexpectedBlockHeight{});

		// Assert:
        EXPECT_TRUE(sm.is(boost::sml::state<LocalChainCheck>));
        EXPECT_EQ(2, reinterpret_cast<const CallbackInvokeCounter&>(actions.CheckLocalChain).InvokeNum);
        EXPECT_EQ(1, reinterpret_cast<const CallbackInvokeCounter&>(actions.DetectStage).InvokeNum);
        EXPECT_EQ(1, reinterpret_cast<const CallbackInvokeCounter&>(actions.SelectCommittee).InvokeNum);
	}

	TEST(TEST_CLASS, WeightedVotingTransitionTable_ConfirmedBlockRequest_ConfirmedBlockNotReceived) {
		// Arrange:
		WeightedVotingActions actions;
        ArrangeCommitteeSelectionActions(actions);
		actions.RequestConfirmedBlock = CallbackInvokeCounter{};
		actions.IncrementRound = CallbackInvokeCounter{};
		actions.SelectCommittee = CallbackInvokeCounter{};

		// Act:
        boost::sml::sm<WeightedVotingTransitionTable> sm{actions};
        ProcessCommitteeSelectionEvents(sm, CommitteeSelectionResult{false, CommitteePhase::None});
		sm.process_event(BlockProposing{});
		sm.process_event(ConfirmedBlockNotReceived{});

		// Assert:
        EXPECT_TRUE(sm.is(boost::sml::state<CommitteeSelection>));
		EXPECT_EQ(1, reinterpret_cast<const CallbackInvokeCounter&>(actions.RequestConfirmedBlock).InvokeNum);
		EXPECT_EQ(1, reinterpret_cast<const CallbackInvokeCounter&>(actions.IncrementRound).InvokeNum);
		EXPECT_EQ(2, reinterpret_cast<const CallbackInvokeCounter&>(actions.SelectCommittee).InvokeNum);
	}

	TEST(TEST_CLASS, WeightedVotingTransitionTable_ConfirmedBlockRequest_ConfirmedBlockReceived) {
		// Arrange:
		WeightedVotingActions actions;
        ArrangeCommitteeSelectionActions(actions);
		actions.RequestConfirmedBlock = CallbackInvokeCounter{};
		actions.CommitConfirmedBlock = CallbackInvokeCounter{};

		// Act:
        boost::sml::sm<WeightedVotingTransitionTable> sm{actions};
        ProcessCommitteeSelectionEvents(sm, CommitteeSelectionResult{false, CommitteePhase::None});
		sm.process_event(BlockProposing{});
		sm.process_event(ConfirmedBlockReceived{});

		// Assert:
        CheckCommitteeSelectionActions(actions);
        EXPECT_TRUE(sm.is(boost::sml::state<Commit>));
		EXPECT_EQ(1, reinterpret_cast<const CallbackInvokeCounter&>(actions.RequestConfirmedBlock).InvokeNum);
		EXPECT_EQ(1, reinterpret_cast<const CallbackInvokeCounter&>(actions.CommitConfirmedBlock).InvokeNum);
	}

}}
