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

	TEST(TEST_CLASS, WeightedVotingTransitionTable_InvalidLocalChain) {
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

	TEST(TEST_CLASS, WeightedVotingTransitionTable_PeersSelection) {
		// Arrange:
		WeightedVotingActions actions;
		actions.CheckLocalChain = CallbackInvokeCounter{};
		actions.SelectPeers = CallbackInvokeCounter{};

		boost::sml::sm<WeightedVotingTransitionTable> sm{actions};
		sm.process_event(StartLocalChainCheck{});

		// Act:
		sm.process_event(NetworkHeightGreaterThanLocal{});

		EXPECT_TRUE(sm.is(boost::sml::state<PeersSelection>));
		EXPECT_EQ(1, reinterpret_cast<const CallbackInvokeCounter&>(actions.CheckLocalChain).InvokeNum);
		EXPECT_EQ(1, reinterpret_cast<const CallbackInvokeCounter&>(actions.SelectPeers).InvokeNum);
	}

	TEST(TEST_CLASS, WeightedVotingTransitionTable_PeersSelectionFailed) {
		// Arrange:
		WeightedVotingActions actions;
		actions.CheckLocalChain = CallbackInvokeCounter{};

		boost::sml::sm<WeightedVotingTransitionTable> sm{actions};

		bool selectPeersCalled = false;
		actions.SelectPeers = [&sm, &selectPeersCalled] {
			if (!selectPeersCalled) {
				selectPeersCalled = true;
				sm.process_event(PeersSelectionFailed{});
			}
		};

		sm.process_event(StartLocalChainCheck{});

		// Act:
		sm.process_event(NetworkHeightGreaterThanLocal{});

		// Assert:
		EXPECT_TRUE(sm.is(boost::sml::state<PeersSelection>));
		EXPECT_EQ(1, reinterpret_cast<const CallbackInvokeCounter&>(actions.CheckLocalChain).InvokeNum);
		EXPECT_TRUE(selectPeersCalled);
	}

	TEST(TEST_CLASS, WeightedVotingTransitionTable_BlocksDownloading) {
		// Arrange:
		WeightedVotingActions actions;
		actions.CheckLocalChain = CallbackInvokeCounter{};
		actions.DownloadBlocks = CallbackInvokeCounter{};

		boost::sml::sm<WeightedVotingTransitionTable> sm{actions};
		actions.SelectPeers = [&sm] { sm.process_event(PeersSelectionSucceeded{}); };
		sm.process_event(StartLocalChainCheck{});

		// Act:
		sm.process_event(NetworkHeightGreaterThanLocal{});

		// Assert:
		EXPECT_TRUE(sm.is(boost::sml::state<BlocksDownloading>));
		EXPECT_EQ(1, reinterpret_cast<const CallbackInvokeCounter&>(actions.CheckLocalChain).InvokeNum);
		EXPECT_EQ(1, reinterpret_cast<const CallbackInvokeCounter&>(actions.DownloadBlocks).InvokeNum);
	}
}}
