/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "fastfinality/src/WeightedVotingFsm.h"
#include "catapult/crypto/KeyPair.h"
#include "tests/TestHarness.h"

namespace catapult { namespace fastfinality {

	namespace {
		dbrb::CommitMessage CreateCommitMessage(const std::vector<ionet::Node>& nodes) {
			dbrb::View certificateView;
			dbrb::View currentView;
			certificateView.Data = std::set<std::pair<dbrb::ProcessId, dbrb::MembershipChanges>>{
				{ nodes[1], dbrb::MembershipChanges::Join },
				{ nodes[3], dbrb::MembershipChanges::Join },
			};
			currentView.Data = std::set<std::pair<dbrb::ProcessId, dbrb::MembershipChanges>>{
				{ nodes[0], dbrb::MembershipChanges::Leave },
				{ nodes[2], dbrb::MembershipChanges::Leave },
				{ nodes[4], dbrb::MembershipChanges::Leave },
			};
			auto payload = ionet::CreateSharedPacket<RemoteNodeStatePacket>();
			std::map<dbrb::ProcessId, catapult::Signature> certificate{
				{ nodes[0], test::GenerateRandomArray<Signature_Size>() },
				{ nodes[1], test::GenerateRandomArray<Signature_Size>() },
				{ nodes[2], test::GenerateRandomArray<Signature_Size>() },
				{ nodes[3], test::GenerateRandomArray<Signature_Size>() },
			};
			return dbrb::CommitMessage(nodes[0], payload, certificate, certificateView, currentView);
		}

		template<typename TMessage>
		void RunMessageSerializationTest(std::function<TMessage (const std::vector<ionet::Node>& nodes)> createMessage, consumer<const TMessage&, const TMessage&> callback) {
			auto nodes = config::LoadPeersFromPath("../resources/peers-dbrb.json", model::NetworkIdentifier::Private_Test);
			EXPECT_EQ(6u, nodes.size());

			auto originalMessage = createMessage(nodes);
			auto keyPair = crypto::KeyPair::FromString("CBD84EF8F5F38A25C01308785EA99627DE897D151AFDFCDA7AB07EFD8ED98534");
			auto pPacket = originalMessage.toNetworkPacket(&keyPair);
			auto pUnpackedMessage = dbrb::NetworkPacketConverter().toMessage(*pPacket);
			const auto& unpackedMessage = reinterpret_cast<const TMessage&>(*pUnpackedMessage);
			EXPECT_EQ(originalMessage.Sender, unpackedMessage.Sender);
			EXPECT_EQ(originalMessage.Type, unpackedMessage.Type);
			EXPECT_EQ(originalMessage.Signature, unpackedMessage.Signature);

			callback(originalMessage, unpackedMessage);
		}
	}

	TEST(TEST_CLASS, ValidateReconfigMessageSerialization) {
		RunMessageSerializationTest<dbrb::ReconfigMessage>([](const auto& nodes) {
				dbrb::View view;
				for (const auto& node : nodes)
					view.Data.emplace(node, dbrb::MembershipChanges::Join);
				return dbrb::ReconfigMessage(nodes[0], nodes[1], dbrb::MembershipChanges::Leave, view);
			},
			[](const dbrb::ReconfigMessage& originalMessage, const dbrb::ReconfigMessage& unpackedMessage) {
				EXPECT_EQ(originalMessage.ProcessId, unpackedMessage.ProcessId);
				EXPECT_EQ(originalMessage.MembershipChange, unpackedMessage.MembershipChange);
				EXPECT_EQ(originalMessage.View, unpackedMessage.View);
			});
	}

	TEST(TEST_CLASS, ValidateReconfigConfirmMessageSerialization) {
		RunMessageSerializationTest<dbrb::ReconfigConfirmMessage>([](const auto& nodes) {
				dbrb::View view;
				for (const auto& node : nodes)
					view.Data.emplace(node, dbrb::MembershipChanges::Join);
				return dbrb::ReconfigConfirmMessage(nodes[0], view);
			},
			[](const dbrb::ReconfigConfirmMessage& originalMessage, const dbrb::ReconfigConfirmMessage& unpackedMessage) {
				EXPECT_EQ(originalMessage.View, unpackedMessage.View);
			});
	}

	TEST(TEST_CLASS, ValidateProposeMessageSerialization) {
		RunMessageSerializationTest<dbrb::ProposeMessage>([](const auto& nodes) {
				dbrb::View view1;
				dbrb::View view2;
				for (const auto& node : nodes) {
					view1.Data.emplace(node, dbrb::MembershipChanges::Join);
					view2.Data.emplace(node, dbrb::MembershipChanges::Leave);
				}
				dbrb::Sequence sequence;
				sequence.tryAppend(view1);
				sequence.tryAppend(view2);
				return dbrb::ProposeMessage(nodes[0], sequence, view2);
			},
			[](const dbrb::ProposeMessage& originalMessage, const dbrb::ProposeMessage& unpackedMessage) {
				EXPECT_EQ(originalMessage.ProposedSequence, unpackedMessage.ProposedSequence);
				EXPECT_EQ(originalMessage.ReplacedView, unpackedMessage.ReplacedView);
			});
	}

	TEST(TEST_CLASS, ValidateConvergedMessageSerialization) {
		RunMessageSerializationTest<dbrb::ConvergedMessage>([](const auto& nodes) {
				dbrb::View view1;
				dbrb::View view2;
				dbrb::View view3;
				for (const auto& node : nodes) {
					view1.Data.emplace(node, dbrb::MembershipChanges::Join);
					view2.Data.emplace(node, dbrb::MembershipChanges::Leave);
				}
				view3.Data = std::set<std::pair<dbrb::ProcessId, dbrb::MembershipChanges>>{
					{ nodes[0], dbrb::MembershipChanges::Join },
					{ nodes[2], dbrb::MembershipChanges::Join },
					{ nodes[4], dbrb::MembershipChanges::Join },
				};
				dbrb::Sequence sequence;
				sequence.tryAppend(view1);
				sequence.tryAppend(view2);
				return dbrb::ConvergedMessage(nodes[0], sequence, view2);
			},
			[](const dbrb::ConvergedMessage& originalMessage, const dbrb::ConvergedMessage& unpackedMessage) {
				EXPECT_EQ(originalMessage.ConvergedSequence, unpackedMessage.ConvergedSequence);
				EXPECT_EQ(originalMessage.ReplacedView, unpackedMessage.ReplacedView);
			});
	}

	TEST(TEST_CLASS, ValidateInstallMessageSerialization) {
		RunMessageSerializationTest<dbrb::InstallMessage>([](const auto& nodes) {
				dbrb::View view1;
				dbrb::View view2;
				for (const auto& node : nodes) {
					view1.Data.emplace(node, dbrb::MembershipChanges::Join);
					view2.Data.emplace(node, dbrb::MembershipChanges::Leave);
				}
				dbrb::Sequence sequence;
				sequence.tryAppend(view1);
				sequence.tryAppend(view2);
				return dbrb::InstallMessage(nodes[0], view1, sequence, view2);
			},
			[](const dbrb::InstallMessage& originalMessage, const dbrb::InstallMessage& unpackedMessage) {
				EXPECT_EQ(originalMessage.LeastRecentView, unpackedMessage.LeastRecentView);
				EXPECT_EQ(originalMessage.ConvergedSequence, unpackedMessage.ConvergedSequence);
				EXPECT_EQ(originalMessage.ReplacedView, unpackedMessage.ReplacedView);
			});
	}

	TEST(TEST_CLASS, ValidatePrepareMessageSerialization) {
		RunMessageSerializationTest<dbrb::PrepareMessage>([](const auto& nodes) {
				dbrb::View view;
				for (const auto& node : nodes)
					view.Data.emplace(node, dbrb::MembershipChanges::Join);
				auto payload = ionet::CreateSharedPacket<RemoteNodeStatePacket>();
				return dbrb::PrepareMessage(nodes[0], payload, view);
			},
			[](const dbrb::PrepareMessage& originalMessage, const dbrb::PrepareMessage& unpackedMessage) {
				EXPECT_EQ(originalMessage.View, unpackedMessage.View);
				EXPECT_EQ(originalMessage.Payload->Size, unpackedMessage.Payload->Size);
				EXPECT_EQ_MEMORY(originalMessage.Payload.get(), unpackedMessage.Payload.get(), originalMessage.Payload->Size);
			});
	}

	TEST(TEST_CLASS, ValidateAcknowledgedMessageSerialization) {
		RunMessageSerializationTest<dbrb::AcknowledgedMessage>([](const auto& nodes) {
				dbrb::View view;
				for (const auto& node : nodes)
					view.Data.emplace(node, dbrb::MembershipChanges::Join);
				auto payload = ionet::CreateSharedPacket<RemoteNodeStatePacket>();
				return dbrb::AcknowledgedMessage(nodes[0], payload, view, test::GenerateRandomArray<Signature_Size>());
			},
			[](const dbrb::AcknowledgedMessage& originalMessage, const dbrb::AcknowledgedMessage& unpackedMessage) {
				EXPECT_EQ(originalMessage.View, unpackedMessage.View);
				EXPECT_EQ(originalMessage.Payload->Size, unpackedMessage.Payload->Size);
				EXPECT_EQ_MEMORY(originalMessage.Payload.get(), unpackedMessage.Payload.get(), originalMessage.Payload->Size);
			});
	}

	TEST(TEST_CLASS, ValidateCommitMessageSerialization) {
		RunMessageSerializationTest<dbrb::CommitMessage>([](const auto& nodes) {
				return CreateCommitMessage(nodes);
			},
			[](const dbrb::CommitMessage& originalMessage, const dbrb::CommitMessage& unpackedMessage) {
				EXPECT_EQ(originalMessage.Payload->Size, unpackedMessage.Payload->Size);
				EXPECT_EQ_MEMORY(originalMessage.Payload.get(), unpackedMessage.Payload.get(), originalMessage.Payload->Size);
				EXPECT_EQ(originalMessage.CertificateView, unpackedMessage.CertificateView);
				EXPECT_EQ(originalMessage.CurrentView, unpackedMessage.CurrentView);
			});
	}

	TEST(TEST_CLASS, ValidateStateUpdateMessageSerializationAllStateFields) {
		RunMessageSerializationTest<dbrb::StateUpdateMessage>([](const auto& nodes) {
				dbrb::ProcessState state;
				dbrb::View view;
				for (const auto& node : nodes)
					view.Data.emplace(node, dbrb::MembershipChanges::Join);
				auto payload = ionet::CreateSharedPacket<RemoteNodeStatePacket>();
				dbrb::PrepareMessage prepareMessage(nodes[0], payload, view);
				state.Acknowledgeable = prepareMessage;
				state.Conflicting = std::make_pair(prepareMessage, prepareMessage);
				state.Stored = CreateCommitMessage(nodes);
				dbrb::View pendingChanges;
				view.Data = std::set<std::pair<dbrb::ProcessId, dbrb::MembershipChanges>>{
					{ nodes[1], dbrb::MembershipChanges::Join },
					{ nodes[3], dbrb::MembershipChanges::Join },
				};
				pendingChanges.Data = std::set<std::pair<dbrb::ProcessId, dbrb::MembershipChanges>>{
					{ nodes[0], dbrb::MembershipChanges::Leave },
					{ nodes[2], dbrb::MembershipChanges::Leave },
					{ nodes[4], dbrb::MembershipChanges::Leave },
				};
				return dbrb::StateUpdateMessage(nodes[0], state, view, pendingChanges);
			},
			[](const dbrb::StateUpdateMessage& originalMessage, const dbrb::StateUpdateMessage& unpackedMessage) {
				EXPECT_EQ(originalMessage.State.Acknowledgeable->Sender, unpackedMessage.State.Acknowledgeable->Sender);
				EXPECT_EQ(originalMessage.State.Acknowledgeable->Type, unpackedMessage.State.Acknowledgeable->Type);
				EXPECT_EQ(originalMessage.State.Acknowledgeable->Signature, unpackedMessage.State.Acknowledgeable->Signature);
				EXPECT_EQ(originalMessage.State.Acknowledgeable->View, unpackedMessage.State.Acknowledgeable->View);
				EXPECT_EQ(originalMessage.State.Acknowledgeable->Payload->Size, unpackedMessage.State.Acknowledgeable->Payload->Size);
				EXPECT_EQ_MEMORY(originalMessage.State.Acknowledgeable->Payload.get(), unpackedMessage.State.Acknowledgeable->Payload.get(), originalMessage.State.Acknowledgeable->Payload->Size);

				EXPECT_EQ(originalMessage.State.Conflicting->first.Sender, unpackedMessage.State.Conflicting->first.Sender);
				EXPECT_EQ(originalMessage.State.Conflicting->first.Type, unpackedMessage.State.Conflicting->first.Type);
				EXPECT_EQ(originalMessage.State.Conflicting->first.Signature, unpackedMessage.State.Conflicting->first.Signature);
				EXPECT_EQ(originalMessage.State.Conflicting->first.View, unpackedMessage.State.Conflicting->first.View);
				EXPECT_EQ(originalMessage.State.Conflicting->first.Payload->Size, unpackedMessage.State.Conflicting->first.Payload->Size);
				EXPECT_EQ_MEMORY(originalMessage.State.Conflicting->first.Payload.get(), unpackedMessage.State.Conflicting->first.Payload.get(), originalMessage.State.Conflicting->first.Payload->Size);

				EXPECT_EQ(originalMessage.State.Conflicting->second.Sender, unpackedMessage.State.Conflicting->second.Sender);
				EXPECT_EQ(originalMessage.State.Conflicting->second.Type, unpackedMessage.State.Conflicting->second.Type);
				EXPECT_EQ(originalMessage.State.Conflicting->second.Signature, unpackedMessage.State.Conflicting->second.Signature);
				EXPECT_EQ(originalMessage.State.Conflicting->second.View, unpackedMessage.State.Conflicting->second.View);
				EXPECT_EQ(originalMessage.State.Conflicting->second.Payload->Size, unpackedMessage.State.Conflicting->second.Payload->Size);
				EXPECT_EQ_MEMORY(originalMessage.State.Conflicting->second.Payload.get(), unpackedMessage.State.Conflicting->second.Payload.get(), originalMessage.State.Conflicting->second.Payload->Size);

				EXPECT_EQ(originalMessage.State.Stored->Sender, unpackedMessage.State.Stored->Sender);
				EXPECT_EQ(originalMessage.State.Stored->Type, unpackedMessage.State.Stored->Type);
				EXPECT_EQ(originalMessage.State.Stored->Signature, unpackedMessage.State.Stored->Signature);
				EXPECT_EQ(originalMessage.State.Stored->Payload->Size, unpackedMessage.State.Stored->Payload->Size);
				EXPECT_EQ_MEMORY(originalMessage.State.Stored->Payload.get(), unpackedMessage.State.Stored->Payload.get(), originalMessage.State.Stored->Payload->Size);
				EXPECT_EQ(originalMessage.State.Stored->CertificateView, unpackedMessage.State.Stored->CertificateView);
				EXPECT_EQ(originalMessage.State.Stored->CurrentView, unpackedMessage.State.Stored->CurrentView);

				EXPECT_EQ(originalMessage.View, unpackedMessage.View);
				EXPECT_EQ(originalMessage.PendingChanges, unpackedMessage.PendingChanges);
			});
	}

	TEST(TEST_CLASS, ValidateStateUpdateMessageSerializationWithoutAknowledgeable) {
		RunMessageSerializationTest<dbrb::StateUpdateMessage>([](const auto& nodes) {
				dbrb::ProcessState state;
				dbrb::View view;
				for (const auto& node : nodes)
					view.Data.emplace(node, dbrb::MembershipChanges::Join);
				auto payload = ionet::CreateSharedPacket<RemoteNodeStatePacket>();
				dbrb::PrepareMessage prepareMessage(nodes[0], payload, view);
				state.Conflicting = std::make_pair(prepareMessage, prepareMessage);
				state.Stored = CreateCommitMessage(nodes);
				dbrb::View pendingChanges;
				view.Data = std::set<std::pair<dbrb::ProcessId, dbrb::MembershipChanges>>{
					{ nodes[1], dbrb::MembershipChanges::Join },
					{ nodes[3], dbrb::MembershipChanges::Join },
				};
				pendingChanges.Data = std::set<std::pair<dbrb::ProcessId, dbrb::MembershipChanges>>{
					{ nodes[0], dbrb::MembershipChanges::Leave },
					{ nodes[2], dbrb::MembershipChanges::Leave },
					{ nodes[4], dbrb::MembershipChanges::Leave },
				};
				return dbrb::StateUpdateMessage(nodes[0], state, view, pendingChanges);
			},
			[](const dbrb::StateUpdateMessage& originalMessage, const dbrb::StateUpdateMessage& unpackedMessage) {
				EXPECT_EQ(false, unpackedMessage.State.Acknowledgeable.has_value());

				EXPECT_EQ(originalMessage.State.Conflicting->first.Sender, unpackedMessage.State.Conflicting->first.Sender);
				EXPECT_EQ(originalMessage.State.Conflicting->first.Type, unpackedMessage.State.Conflicting->first.Type);
				EXPECT_EQ(originalMessage.State.Conflicting->first.Signature, unpackedMessage.State.Conflicting->first.Signature);
				EXPECT_EQ(originalMessage.State.Conflicting->first.View, unpackedMessage.State.Conflicting->first.View);
				EXPECT_EQ(originalMessage.State.Conflicting->first.Payload->Size, unpackedMessage.State.Conflicting->first.Payload->Size);
				EXPECT_EQ_MEMORY(originalMessage.State.Conflicting->first.Payload.get(), unpackedMessage.State.Conflicting->first.Payload.get(), originalMessage.State.Conflicting->first.Payload->Size);

				EXPECT_EQ(originalMessage.State.Conflicting->second.Sender, unpackedMessage.State.Conflicting->second.Sender);
				EXPECT_EQ(originalMessage.State.Conflicting->second.Type, unpackedMessage.State.Conflicting->second.Type);
				EXPECT_EQ(originalMessage.State.Conflicting->second.Signature, unpackedMessage.State.Conflicting->second.Signature);
				EXPECT_EQ(originalMessage.State.Conflicting->second.View, unpackedMessage.State.Conflicting->second.View);
				EXPECT_EQ(originalMessage.State.Conflicting->second.Payload->Size, unpackedMessage.State.Conflicting->second.Payload->Size);
				EXPECT_EQ_MEMORY(originalMessage.State.Conflicting->second.Payload.get(), unpackedMessage.State.Conflicting->second.Payload.get(), originalMessage.State.Conflicting->second.Payload->Size);

				EXPECT_EQ(originalMessage.State.Stored->Sender, unpackedMessage.State.Stored->Sender);
				EXPECT_EQ(originalMessage.State.Stored->Type, unpackedMessage.State.Stored->Type);
				EXPECT_EQ(originalMessage.State.Stored->Signature, unpackedMessage.State.Stored->Signature);
				EXPECT_EQ(originalMessage.State.Stored->Payload->Size, unpackedMessage.State.Stored->Payload->Size);
				EXPECT_EQ_MEMORY(originalMessage.State.Stored->Payload.get(), unpackedMessage.State.Stored->Payload.get(), originalMessage.State.Stored->Payload->Size);
				EXPECT_EQ(originalMessage.State.Stored->CertificateView, unpackedMessage.State.Stored->CertificateView);
				EXPECT_EQ(originalMessage.State.Stored->CurrentView, unpackedMessage.State.Stored->CurrentView);

				EXPECT_EQ(originalMessage.View, unpackedMessage.View);
				EXPECT_EQ(originalMessage.PendingChanges, unpackedMessage.PendingChanges);
			});
	}

	TEST(TEST_CLASS, ValidateStateUpdateMessageSerializationWithoutConflicting) {
		RunMessageSerializationTest<dbrb::StateUpdateMessage>([](const auto& nodes) {
				dbrb::ProcessState state;
				dbrb::View view;
				for (const auto& node : nodes)
					view.Data.emplace(node, dbrb::MembershipChanges::Join);
				auto payload = ionet::CreateSharedPacket<RemoteNodeStatePacket>();
				dbrb::PrepareMessage prepareMessage(nodes[0], payload, view);
				state.Acknowledgeable = prepareMessage;
				state.Stored = CreateCommitMessage(nodes);
				dbrb::View pendingChanges;
				view.Data = std::set<std::pair<dbrb::ProcessId, dbrb::MembershipChanges>>{
					{ nodes[1], dbrb::MembershipChanges::Join },
					{ nodes[3], dbrb::MembershipChanges::Join },
				};
				pendingChanges.Data = std::set<std::pair<dbrb::ProcessId, dbrb::MembershipChanges>>{
					{ nodes[0], dbrb::MembershipChanges::Leave },
					{ nodes[2], dbrb::MembershipChanges::Leave },
					{ nodes[4], dbrb::MembershipChanges::Leave },
				};
				return dbrb::StateUpdateMessage(nodes[0], state, view, pendingChanges);
			},
			[](const dbrb::StateUpdateMessage& originalMessage, const dbrb::StateUpdateMessage& unpackedMessage) {
				EXPECT_EQ(originalMessage.State.Acknowledgeable->Sender, unpackedMessage.State.Acknowledgeable->Sender);
				EXPECT_EQ(originalMessage.State.Acknowledgeable->Type, unpackedMessage.State.Acknowledgeable->Type);
				EXPECT_EQ(originalMessage.State.Acknowledgeable->Signature, unpackedMessage.State.Acknowledgeable->Signature);
				EXPECT_EQ(originalMessage.State.Acknowledgeable->View, unpackedMessage.State.Acknowledgeable->View);
				EXPECT_EQ(originalMessage.State.Acknowledgeable->Payload->Size, unpackedMessage.State.Acknowledgeable->Payload->Size);
				EXPECT_EQ_MEMORY(originalMessage.State.Acknowledgeable->Payload.get(), unpackedMessage.State.Acknowledgeable->Payload.get(), originalMessage.State.Acknowledgeable->Payload->Size);

				EXPECT_EQ(false, unpackedMessage.State.Conflicting.has_value());

				EXPECT_EQ(originalMessage.State.Stored->Sender, unpackedMessage.State.Stored->Sender);
				EXPECT_EQ(originalMessage.State.Stored->Type, unpackedMessage.State.Stored->Type);
				EXPECT_EQ(originalMessage.State.Stored->Signature, unpackedMessage.State.Stored->Signature);
				EXPECT_EQ(originalMessage.State.Stored->Payload->Size, unpackedMessage.State.Stored->Payload->Size);
				EXPECT_EQ_MEMORY(originalMessage.State.Stored->Payload.get(), unpackedMessage.State.Stored->Payload.get(), originalMessage.State.Stored->Payload->Size);
				EXPECT_EQ(originalMessage.State.Stored->CertificateView, unpackedMessage.State.Stored->CertificateView);
				EXPECT_EQ(originalMessage.State.Stored->CurrentView, unpackedMessage.State.Stored->CurrentView);

				EXPECT_EQ(originalMessage.View, unpackedMessage.View);
				EXPECT_EQ(originalMessage.PendingChanges, unpackedMessage.PendingChanges);
			});
	}

	TEST(TEST_CLASS, ValidateStateUpdateMessageSerializationWithoutStored) {
		RunMessageSerializationTest<dbrb::StateUpdateMessage>([](const auto& nodes) {
				dbrb::ProcessState state;
				dbrb::View view;
				for (const auto& node : nodes)
					view.Data.emplace(node, dbrb::MembershipChanges::Join);
				auto payload = ionet::CreateSharedPacket<RemoteNodeStatePacket>();
				dbrb::PrepareMessage prepareMessage(nodes[0], payload, view);
				state.Acknowledgeable = prepareMessage;
				state.Conflicting = std::make_pair(prepareMessage, prepareMessage);
				dbrb::View pendingChanges;
				view.Data = std::set<std::pair<dbrb::ProcessId, dbrb::MembershipChanges>>{
					{ nodes[1], dbrb::MembershipChanges::Join },
					{ nodes[3], dbrb::MembershipChanges::Join },
				};
				pendingChanges.Data = std::set<std::pair<dbrb::ProcessId, dbrb::MembershipChanges>>{
					{ nodes[0], dbrb::MembershipChanges::Leave },
					{ nodes[2], dbrb::MembershipChanges::Leave },
					{ nodes[4], dbrb::MembershipChanges::Leave },
				};
				return dbrb::StateUpdateMessage(nodes[0], state, view, pendingChanges);
			},
			[](const dbrb::StateUpdateMessage& originalMessage, const dbrb::StateUpdateMessage& unpackedMessage) {
				EXPECT_EQ(originalMessage.State.Acknowledgeable->Sender, unpackedMessage.State.Acknowledgeable->Sender);
				EXPECT_EQ(originalMessage.State.Acknowledgeable->Type, unpackedMessage.State.Acknowledgeable->Type);
				EXPECT_EQ(originalMessage.State.Acknowledgeable->Signature, unpackedMessage.State.Acknowledgeable->Signature);
				EXPECT_EQ(originalMessage.State.Acknowledgeable->View, unpackedMessage.State.Acknowledgeable->View);
				EXPECT_EQ(originalMessage.State.Acknowledgeable->Payload->Size, unpackedMessage.State.Acknowledgeable->Payload->Size);
				EXPECT_EQ_MEMORY(originalMessage.State.Acknowledgeable->Payload.get(), unpackedMessage.State.Acknowledgeable->Payload.get(), originalMessage.State.Acknowledgeable->Payload->Size);

				EXPECT_EQ(originalMessage.State.Conflicting->first.Sender, unpackedMessage.State.Conflicting->first.Sender);
				EXPECT_EQ(originalMessage.State.Conflicting->first.Type, unpackedMessage.State.Conflicting->first.Type);
				EXPECT_EQ(originalMessage.State.Conflicting->first.Signature, unpackedMessage.State.Conflicting->first.Signature);
				EXPECT_EQ(originalMessage.State.Conflicting->first.View, unpackedMessage.State.Conflicting->first.View);
				EXPECT_EQ(originalMessage.State.Conflicting->first.Payload->Size, unpackedMessage.State.Conflicting->first.Payload->Size);
				EXPECT_EQ_MEMORY(originalMessage.State.Conflicting->first.Payload.get(), unpackedMessage.State.Conflicting->first.Payload.get(), originalMessage.State.Conflicting->first.Payload->Size);

				EXPECT_EQ(originalMessage.State.Conflicting->second.Sender, unpackedMessage.State.Conflicting->second.Sender);
				EXPECT_EQ(originalMessage.State.Conflicting->second.Type, unpackedMessage.State.Conflicting->second.Type);
				EXPECT_EQ(originalMessage.State.Conflicting->second.Signature, unpackedMessage.State.Conflicting->second.Signature);
				EXPECT_EQ(originalMessage.State.Conflicting->second.View, unpackedMessage.State.Conflicting->second.View);
				EXPECT_EQ(originalMessage.State.Conflicting->second.Payload->Size, unpackedMessage.State.Conflicting->second.Payload->Size);
				EXPECT_EQ_MEMORY(originalMessage.State.Conflicting->second.Payload.get(), unpackedMessage.State.Conflicting->second.Payload.get(), originalMessage.State.Conflicting->second.Payload->Size);

				EXPECT_EQ(false, unpackedMessage.State.Stored.has_value());

				EXPECT_EQ(originalMessage.View, unpackedMessage.View);
				EXPECT_EQ(originalMessage.PendingChanges, unpackedMessage.PendingChanges);
			});
	}

	TEST(TEST_CLASS, ValidateStateUpdateMessageSerializationEmptyState) {
		RunMessageSerializationTest<dbrb::StateUpdateMessage>([](const auto& nodes) {
				dbrb::ProcessState state;
				dbrb::View view;
				for (const auto& node : nodes)
					view.Data.emplace(node, dbrb::MembershipChanges::Join);
				dbrb::View pendingChanges;
				view.Data = std::set<std::pair<dbrb::ProcessId, dbrb::MembershipChanges>>{
					{ nodes[1], dbrb::MembershipChanges::Join },
					{ nodes[3], dbrb::MembershipChanges::Join },
				};
				pendingChanges.Data = std::set<std::pair<dbrb::ProcessId, dbrb::MembershipChanges>>{
					{ nodes[0], dbrb::MembershipChanges::Leave },
					{ nodes[2], dbrb::MembershipChanges::Leave },
					{ nodes[4], dbrb::MembershipChanges::Leave },
				};
				return dbrb::StateUpdateMessage(nodes[0], state, view, pendingChanges);
			},
			[](const dbrb::StateUpdateMessage& originalMessage, const dbrb::StateUpdateMessage& unpackedMessage) {
				EXPECT_EQ(false, unpackedMessage.State.Acknowledgeable.has_value());
				EXPECT_EQ(false, unpackedMessage.State.Conflicting.has_value());
				EXPECT_EQ(false, unpackedMessage.State.Stored.has_value());

				EXPECT_EQ(originalMessage.View, unpackedMessage.View);
				EXPECT_EQ(originalMessage.PendingChanges, unpackedMessage.PendingChanges);
			});
	}

	TEST(TEST_CLASS, ValidateDeliverMessageSerialization) {
		RunMessageSerializationTest<dbrb::DeliverMessage>([](const auto& nodes) {
				dbrb::View view;
				for (const auto& node : nodes)
					view.Data.emplace(node, dbrb::MembershipChanges::Join);
				auto payload = ionet::CreateSharedPacket<RemoteNodeStatePacket>();
				return dbrb::DeliverMessage(nodes[0], payload, view);
			},
			[](const dbrb::DeliverMessage& originalMessage, const dbrb::DeliverMessage& unpackedMessage) {
				EXPECT_EQ(originalMessage.View, unpackedMessage.View);
				EXPECT_EQ(originalMessage.Payload->Size, unpackedMessage.Payload->Size);
				EXPECT_EQ_MEMORY(originalMessage.Payload.get(), unpackedMessage.Payload.get(), originalMessage.Payload->Size);
			});
	}
}}
