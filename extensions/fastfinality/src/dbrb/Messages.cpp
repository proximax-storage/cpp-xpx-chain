/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Messages.h"
#include "catapult/crypto/Signer.h"

namespace catapult { namespace dbrb {

	namespace {

#pragma pack(push, 1)

		struct ReconfigMessagePacket : public MessagePacket {
			static constexpr ionet::PacketType Packet_Type = ionet::PacketType::Dbrb_Reconfig_Message;
		};

		struct ReconfigConfirmMessagePacket : public MessagePacket {
			static constexpr ionet::PacketType Packet_Type = ionet::PacketType::Dbrb_Reconfig_Confirm_Message;
		};

		struct ProposeMessagePacket : public MessagePacket {
			static constexpr ionet::PacketType Packet_Type = ionet::PacketType::Dbrb_Propose_Message;
		};

		struct ConvergedMessagePacket : public MessagePacket {
			static constexpr ionet::PacketType Packet_Type = ionet::PacketType::Dbrb_Converged_Message;
		};

		struct InstallMessagePacket : public MessagePacket {
			static constexpr ionet::PacketType Packet_Type = ionet::PacketType::Dbrb_Install_Message;
		};

		struct PrepareMessagePacket : public MessagePacket {
			static constexpr ionet::PacketType Packet_Type = ionet::PacketType::Dbrb_Prepare_Message;
		};

		struct AcknowledgedMessagePacket : public MessagePacket {
			static constexpr ionet::PacketType Packet_Type = ionet::PacketType::Dbrb_Acknowledged_Message;
		};

		struct CommitMessagePacket : public MessagePacket {
			static constexpr ionet::PacketType Packet_Type = ionet::PacketType::Dbrb_Commit_Message;
		};

		struct StateUpdateMessagePacket : public MessagePacket {
			static constexpr ionet::PacketType Packet_Type = ionet::PacketType::Dbrb_State_Update_Message;
		};

		struct DeliverMessagePacket : public MessagePacket {
			static constexpr ionet::PacketType Packet_Type = ionet::PacketType::Dbrb_Deliver_Message;
		};

#pragma pack(pop)

		void MaybeSignMessage(const crypto::KeyPair* pKeyPair, MessagePacket* pPacket, Message* pMessage) {
			if (pKeyPair) {
				auto hash = CalculateHash(pPacket->buffers());
				crypto::Sign(*pKeyPair, hash, pMessage->Signature);
			}
			pPacket->Signature = pMessage->Signature;
		}

		auto ToPrepareMessage(const ionet::Packet& packet) {
			const auto* pMessagePacket = reinterpret_cast<const PrepareMessagePacket*>(&packet);
			auto pData = pMessagePacket->payload();
			auto sender = UnpackProcessId(pData);
			auto payloadSize = *reinterpret_cast<const uint32_t*>(pData);
			auto payload = ionet::CreateSharedPacket<ionet::Packet>(payloadSize);
			memcpy(payload.get(), pData, payloadSize);
			pData += payloadSize;
			auto view = UnpackView(pData);

			auto pMessage = std::make_unique<PrepareMessage>(sender, payload, view);
			pMessage->Signature = pMessagePacket->Signature;

			return pMessage;
		}

		auto ToCommitMessage(const ionet::Packet& packet) {
			const auto* pMessagePacket = reinterpret_cast<const CommitMessagePacket*>(&packet);
			auto pData = pMessagePacket->payload();
			auto sender = UnpackProcessId(pData);
			auto initiator = UnpackProcessId(pData);
			auto payloadSize = *reinterpret_cast<const uint32_t*>(pData);
			auto payload = ionet::CreateSharedPacket<ionet::Packet>(payloadSize);
			memcpy(payload.get(), pData, payloadSize);
			pData += payloadSize;

			auto certificateSize = *reinterpret_cast<const uint32_t*>(pData);
			pData += sizeof(uint32_t);
			std::map<ProcessId, Signature> certificate;
			for (auto i = 0u; i < certificateSize; ++i) {
				const auto& networkNode = *reinterpret_cast<const ionet::NetworkNode*>(pData);
				auto node = ionet::UnpackNode(networkNode);
				pData += networkNode.Size;
				Signature signature;
				memcpy(signature.data(), pData, Signature_Size);
				pData += Signature_Size;
				certificate.emplace(node, signature);
			}
			auto certificateView = UnpackView(pData);
			auto currentView = UnpackView(pData);

			auto pMessage = std::make_unique<CommitMessage>(sender, initiator, payload, certificate, certificateView, currentView);
			pMessage->Signature = pMessagePacket->Signature;

			return pMessage;
		}
	}

	std::unique_ptr<Message> NetworkPacketConverter::toMessage(const ionet::Packet& packet) const {
		const auto* pHandler = findConverter(packet);
		if (!pHandler)
			CATAPULT_THROW_RUNTIME_ERROR_1("packet converter not registered", packet.Type)

		CATAPULT_LOG(trace) << "processing " << packet;
		return (*pHandler)(packet);
	}

	void NetworkPacketConverter::registerConverter(ionet::PacketType type, const PacketConverter& converter) {
		auto rawType = utils::to_underlying_type(type);
		if (rawType >= m_converters.size())
			m_converters.resize(rawType + 1);

		if (m_converters[rawType])
			CATAPULT_THROW_RUNTIME_ERROR_1("converter for type is already registered", type)

		m_converters[rawType] = converter;
	}

	const NetworkPacketConverter::PacketConverter* NetworkPacketConverter::findConverter(const ionet::Packet& packet) const {
		auto rawType = utils::to_underlying_type(packet.Type);
		if (rawType >= m_converters.size()) {
			CATAPULT_LOG(error) << "requested unknown converter: " << packet;
			return nullptr;
		}

		const auto& converter = m_converters[rawType];
		return converter ? &converter : nullptr;
	}

	NetworkPacketConverter::NetworkPacketConverter() {
		registerConverter(ionet::PacketType::Dbrb_Reconfig_Message, [](const ionet::Packet& packet) {
			const auto* pMessagePacket = reinterpret_cast<const ReconfigMessagePacket*>(&packet);
			auto pData = pMessagePacket->payload();
			auto sender = UnpackProcessId(pData);
			auto processId = UnpackProcessId(pData);
			auto change = MembershipChanges(*pData++);
			auto view = UnpackView(pData);

			auto pMessage = std::make_unique<ReconfigMessage>(sender, processId, change, view);
			pMessage->Signature = pMessagePacket->Signature;

			return pMessage;
		});

		registerConverter(ionet::PacketType::Dbrb_Reconfig_Confirm_Message, [](const ionet::Packet& packet) {
			const auto* pMessagePacket = reinterpret_cast<const ReconfigConfirmMessagePacket*>(&packet);
			auto pData = pMessagePacket->payload();
			auto sender = UnpackProcessId(pData);
			auto view = UnpackView(pData);

			auto pMessage = std::make_unique<ReconfigConfirmMessage>(sender, view);
			pMessage->Signature = pMessagePacket->Signature;

			return pMessage;
		});

		registerConverter(ionet::PacketType::Dbrb_Propose_Message, [](const ionet::Packet& packet) {
			const auto* pMessagePacket = reinterpret_cast<const ProposeMessagePacket*>(&packet);
			auto pData = pMessagePacket->payload();
			auto sender = UnpackProcessId(pData);
			auto proposedSequenceSize = *reinterpret_cast<const uint32_t*>(pData);
			pData += sizeof(uint32_t);
			View replacedView = UnpackView(pData);
			Sequence proposedSequence;
			for (auto i = 0u; i < proposedSequenceSize; ++i)
				proposedSequence.tryAppend(UnpackView(pData));

			auto pMessage = std::make_unique<ProposeMessage>(sender, proposedSequence, replacedView);
			pMessage->Signature = pMessagePacket->Signature;

			return pMessage;
		});

		registerConverter(ionet::PacketType::Dbrb_Converged_Message, [](const ionet::Packet& packet) {
			const auto* pMessagePacket = reinterpret_cast<const ConvergedMessagePacket*>(&packet);
			auto pData = pMessagePacket->payload();
			auto sender = UnpackProcessId(pData);
			auto convergedSequenceSize = *reinterpret_cast<const uint32_t*>(pData);
			pData += sizeof(uint32_t);
			View replacedView = UnpackView(pData);
			Sequence convergedSequence;
			for (auto i = 0u; i < convergedSequenceSize; ++i)
				convergedSequence.tryAppend(UnpackView(pData));

			auto pMessage = std::make_unique<ConvergedMessage>(sender, convergedSequence, replacedView);
			pMessage->Signature = pMessagePacket->Signature;

			return pMessage;
		});

		registerConverter(ionet::PacketType::Dbrb_Install_Message, [](const ionet::Packet& packet) {
			const auto* pMessagePacket = reinterpret_cast<const InstallMessagePacket*>(&packet);
			auto pData = pMessagePacket->payload();
			auto sender = UnpackProcessId(pData);
			auto convergedSequenceSize = *reinterpret_cast<const uint32_t*>(pData);
			pData += sizeof(uint32_t);
			View leastRecentView = UnpackView(pData);
			View replacedView = UnpackView(pData);
			Sequence convergedSequence;
			for (auto i = 0u; i < convergedSequenceSize; ++i)
				convergedSequence.tryAppend(UnpackView(pData));

			auto pMessage = std::make_unique<InstallMessage>(sender, leastRecentView, convergedSequence, replacedView);
			pMessage->Signature = pMessagePacket->Signature;

			return pMessage;
		});

		registerConverter(ionet::PacketType::Dbrb_Prepare_Message, [](const ionet::Packet& packet) {
			return ToPrepareMessage(packet);
		});

		registerConverter(ionet::PacketType::Dbrb_Acknowledged_Message, [](const ionet::Packet& packet) {
			const auto* pMessagePacket = reinterpret_cast<const AcknowledgedMessagePacket*>(&packet);
			auto pData = pMessagePacket->payload();
			auto sender = UnpackProcessId(pData);
			auto initiator = UnpackProcessId(pData);
			auto payloadSignature = *reinterpret_cast<const Signature*>(pData);
			pData += Signature_Size;
			auto payloadSize = *reinterpret_cast<const uint32_t*>(pData);
			auto payload = ionet::CreateSharedPacket<ionet::Packet>(payloadSize);
			memcpy(payload.get(), pData, payloadSize);
			pData += payloadSize;
			auto view = UnpackView(pData);

			auto pMessage = std::make_unique<AcknowledgedMessage>(sender, initiator, payload, view, payloadSignature);
			pMessage->Signature = pMessagePacket->Signature;

			return pMessage;
		});

		registerConverter(ionet::PacketType::Dbrb_Commit_Message, [](const ionet::Packet& packet) {
			return ToCommitMessage(packet);
		});

		registerConverter(ionet::PacketType::Dbrb_State_Update_Message, [](const ionet::Packet& packet) {
			const auto* pMessagePacket = reinterpret_cast<const StateUpdateMessagePacket*>(&packet);
			auto pData = pMessagePacket->payload();
			auto sender = UnpackProcessId(pData);

			ProcessState state;
			if (*pData++) {
				const auto* pPrepareMessagePacket = reinterpret_cast<const PrepareMessagePacket*>(pData);
				state.Acknowledgeable = *ToPrepareMessage(*pPrepareMessagePacket);
				pData += pPrepareMessagePacket->Size;
			}

			if (*pData++) {
				const auto* pPrepareMessagePacket = reinterpret_cast<const PrepareMessagePacket*>(pData);
				pData += pPrepareMessagePacket->Size;
				state.Conflicting = *ToPrepareMessage(*pPrepareMessagePacket);
			}

			if (*pData++) {
				const auto* pCommitMessagePacket = reinterpret_cast<const CommitMessagePacket*>(pData);
				state.Stored = *ToCommitMessage(*pCommitMessagePacket);
				pData += pCommitMessagePacket->Size;
			}
			View view = UnpackView(pData);
			View pendingChanges = UnpackView(pData);

			auto pMessage = std::make_unique<StateUpdateMessage>(sender, state, view, pendingChanges);
			pMessage->Signature = pMessagePacket->Signature;

			return pMessage;
		});

		registerConverter(ionet::PacketType::Dbrb_Deliver_Message, [](const ionet::Packet& packet) {
			const auto* pMessagePacket = reinterpret_cast<const DeliverMessagePacket*>(&packet);
			auto pData = pMessagePacket->payload();
			auto sender = UnpackProcessId(pData);
			auto initiator = UnpackProcessId(pData);
			auto payloadSize = *reinterpret_cast<const uint32_t*>(pData);
			auto payload = ionet::CreateSharedPacket<ionet::Packet>(payloadSize);
			memcpy(payload.get(), pData, payloadSize);
			pData += payloadSize;
			View view = UnpackView(pData);

			auto pMessage = std::make_unique<DeliverMessage>(sender, initiator, payload, view);
			pMessage->Signature = pMessagePacket->Signature;

			return pMessage;
		});
	}

	std::shared_ptr<MessagePacket> ReconfigMessage::toNetworkPacket(const crypto::KeyPair* pKeyPair) {
		auto pPackedSender = ionet::PackNode(Sender);
		uint32_t payloadSize = pPackedSender->Size + 1u;
		auto pPackedProcessId = ionet::PackNode(ProcessId);
		payloadSize += pPackedProcessId->Size;
		auto packedView = PackView(View, payloadSize);

		auto pPacket = ionet::CreateSharedPacket<ReconfigMessagePacket>(payloadSize);

		auto pData = pPacket->payload();
		CopyProcessId(pData, pPackedSender);
		CopyProcessId(pData, pPackedProcessId);
		*pData++ = utils::to_underlying_type(MembershipChange);
		CopyView(pData, packedView);

		MaybeSignMessage(pKeyPair, pPacket.get(), this);

		return pPacket;
	}

	std::shared_ptr<MessagePacket> ReconfigConfirmMessage::toNetworkPacket(const crypto::KeyPair* pKeyPair) {
		auto pPackedSender = ionet::PackNode(Sender);
		uint32_t payloadSize = pPackedSender->Size;
		auto packedView = PackView(View, payloadSize);
		auto pPacket = ionet::CreateSharedPacket<ReconfigConfirmMessagePacket>(payloadSize);

		auto pData = pPacket->payload();
		CopyProcessId(pData, pPackedSender);
		CopyView(pData, packedView);

		MaybeSignMessage(pKeyPair, pPacket.get(), this);

		return pPacket;
	}

	std::shared_ptr<MessagePacket> ProposeMessage::toNetworkPacket(const crypto::KeyPair* pKeyPair) {
		auto pPackedSender = ionet::PackNode(Sender);
		uint32_t payloadSize = pPackedSender->Size;
		std::vector<const View*> views;
		std::vector<PackedView> packedViews;

		views.reserve(ProposedSequence.data().size() + 1u);
		packedViews.reserve(views.size());

		views.emplace_back(&ReplacedView);
		packedViews.emplace_back(PackView(ReplacedView, payloadSize));

		for (const auto& view : ProposedSequence.data()) {
			views.emplace_back(&view);
			packedViews.emplace_back(PackView(view, payloadSize));
		}

		auto pPacket = ionet::CreateSharedPacket<ProposeMessagePacket>(payloadSize);

		auto pData = pPacket->payload();
		CopyProcessId(pData, pPackedSender);
		*reinterpret_cast<uint32_t*>(pData) = utils::checked_cast<size_t, uint32_t>(ProposedSequence.data().size());
		pData += sizeof(uint32_t);
		for (const auto& packedView : packedViews)
			CopyView(pData, packedView);

		MaybeSignMessage(pKeyPair, pPacket.get(), this);

		return pPacket;
	}

	std::shared_ptr<MessagePacket> ConvergedMessage::toNetworkPacket(const crypto::KeyPair* pKeyPair) {
		auto pPackedSender = ionet::PackNode(Sender);
		uint32_t payloadSize = pPackedSender->Size;
		std::vector<const View*> views;
		std::vector<PackedView> packedViews;

		views.reserve(ConvergedSequence.data().size() + 1u);
		packedViews.reserve(views.size());

		views.emplace_back(&ReplacedView);
		packedViews.emplace_back(PackView(ReplacedView, payloadSize));

		for (const auto& view : ConvergedSequence.data()) {
			views.emplace_back(&view);
			packedViews.emplace_back(PackView(view, payloadSize));
		}

		auto pPacket = ionet::CreateSharedPacket<ConvergedMessagePacket>(payloadSize);

		auto pData = pPacket->payload();
		CopyProcessId(pData, pPackedSender);
		*reinterpret_cast<uint32_t*>(pData) = utils::checked_cast<size_t, uint32_t>(ConvergedSequence.data().size());
		pData += sizeof(uint32_t);
		for (const auto& packedView : packedViews)
			CopyView(pData, packedView);

		MaybeSignMessage(pKeyPair, pPacket.get(), this);

		return pPacket;
	}

	std::shared_ptr<MessagePacket> InstallMessage::toNetworkPacket(const crypto::KeyPair* pKeyPair) {
		auto pPackedSender = ionet::PackNode(Sender);
		uint32_t payloadSize = pPackedSender->Size;
		std::vector<PackedView> packedViews;

		packedViews.reserve(ConvergedSequence.data().size() + 2u);
		packedViews.emplace_back(PackView(LeastRecentView, payloadSize));
		packedViews.emplace_back(PackView(ReplacedView, payloadSize));
		for (const auto& view : ConvergedSequence.data())
			packedViews.emplace_back(PackView(view, payloadSize));

		auto pPacket = ionet::CreateSharedPacket<InstallMessagePacket>(payloadSize);

		auto pData = pPacket->payload();
		CopyProcessId(pData, pPackedSender);
		*reinterpret_cast<uint32_t*>(pData) = utils::checked_cast<size_t, uint32_t>(ConvergedSequence.data().size());
		pData += sizeof(uint32_t);
		for (const auto& packedView : packedViews)
			CopyView(pData, packedView);

		MaybeSignMessage(pKeyPair, pPacket.get(), this);

		return pPacket;
	}

	std::optional<InstallMessageData> InstallMessage::tryGetMessageData() const {
		const auto& sequenceData = Sequence.data();
		if (sequenceData.size() < 2u)
			return {};

		const std::vector<View> convergedSequenceData(sequenceData.begin() + 1u, sequenceData.end());

		const auto& replacedView = *Sequence.maybeLeastRecent();
		const auto convergedSequence = *Sequence::fromViews(convergedSequenceData);
		const auto& leastRecentView = *convergedSequence.maybeLeastRecent();

		return InstallMessageData{ leastRecentView, convergedSequence, replacedView };
	}

	std::shared_ptr<MessagePacket> PrepareMessage::toNetworkPacket(const crypto::KeyPair* pKeyPair) {
		uint32_t payloadSize = Payload->Size;
		auto pPackedSender = ionet::PackNode(Sender);
		payloadSize += pPackedSender->Size;
		auto packedView = PackView(View, payloadSize);
		auto pPacket = ionet::CreateSharedPacket<PrepareMessagePacket>(payloadSize);

		auto pData = pPacket->payload();
		CopyProcessId(pData, pPackedSender);
		memcpy(pData, Payload.get(), Payload->Size);
		pData += Payload->Size;

		CopyView(pData, packedView);

		MaybeSignMessage(pKeyPair, pPacket.get(), this);

		return pPacket;
	}

	std::shared_ptr<MessagePacket> AcknowledgedMessage::toNetworkPacket(const crypto::KeyPair* pKeyPair) {
		uint32_t payloadSize = Payload->Size + Signature_Size;
		auto pPackedSender = ionet::PackNode(Sender);
		payloadSize += pPackedSender->Size;
		auto pPackedInitiator = ionet::PackNode(Initiator);
		payloadSize += pPackedInitiator->Size;
		auto packedView = PackView(View, payloadSize);
		auto pPacket = ionet::CreateSharedPacket<AcknowledgedMessagePacket>(payloadSize);

		auto pData = pPacket->payload();
		CopyProcessId(pData, pPackedSender);
		CopyProcessId(pData, pPackedInitiator);
		memcpy(pData, PayloadSignature.data(), Signature_Size);
		pData += Signature_Size;
		memcpy(pData, Payload.get(), Payload->Size);
		pData += Payload->Size;

		CopyView(pData, packedView);

		MaybeSignMessage(pKeyPair, pPacket.get(), this);

		return pPacket;
	}

	std::shared_ptr<MessagePacket> CommitMessage::toNetworkPacket(const crypto::KeyPair* pKeyPair) {
		uint32_t certificateSize = utils::checked_cast<size_t, uint32_t>(Certificate.size());
		auto pPackedSender = ionet::PackNode(Sender);
		uint32_t payloadSize = pPackedSender->Size;
		auto pPackedInitiator = ionet::PackNode(Initiator);
		payloadSize += pPackedInitiator->Size;
		payloadSize += Payload->Size;

		payloadSize += sizeof(uint32_t);
		std::vector<std::pair<model::UniqueEntityPtr<ionet::NetworkNode>, catapult::Signature>> certificate;
		for (const auto& pair : Certificate) {
			certificate.emplace_back(ionet::PackNode(pair.first), pair.second);
			payloadSize += certificate.back().first->Size + Signature_Size;
		}
		auto packedCertificateView = PackView(CertificateView, payloadSize);
		auto packedCurrentView = PackView(CurrentView, payloadSize);

		auto pPacket = ionet::CreateSharedPacket<CommitMessagePacket>(payloadSize);

		auto pData = pPacket->payload();
		CopyProcessId(pData, pPackedSender);
		CopyProcessId(pData, pPackedInitiator);
		memcpy(pData, Payload.get(), Payload->Size);
		pData += Payload->Size;

		*reinterpret_cast<uint32_t*>(pData) = certificateSize;
		pData += sizeof(uint32_t);
		for (const auto& pair : Certificate) {
			auto pNode = ionet::PackNode(pair.first);
			memcpy(pData, static_cast<const void*>(pNode.get()), pNode->Size);
			pData += pNode->Size;
			memcpy(pData, pair.second.data(), Signature_Size);
			pData += Signature_Size;
		}

		CopyView(pData, packedCertificateView);
		CopyView(pData, packedCurrentView);

		MaybeSignMessage(pKeyPair, pPacket.get(), this);

		return pPacket;
	}

	std::shared_ptr<MessagePacket> StateUpdateMessage::toNetworkPacket(const crypto::KeyPair* pKeyPair) {
		auto pPackedSender = ionet::PackNode(Sender);
		uint32_t payloadSize = pPackedSender->Size;

		std::shared_ptr<ionet::Packet> pAcknowledgeable;
		payloadSize++;
		if (State.Acknowledgeable) {
			pAcknowledgeable = State.Acknowledgeable.value().toNetworkPacket(nullptr);
			payloadSize += pAcknowledgeable->Size;
		}

		std::shared_ptr<ionet::Packet> pConflicting;
		payloadSize++;
		if (State.Conflicting) {
			pConflicting = State.Conflicting.value().toNetworkPacket(nullptr);
			payloadSize += pConflicting->Size;
		}

		std::shared_ptr<ionet::Packet> pStored;
		payloadSize++;
		if (State.Stored) {
			pStored = State.Stored.value().toNetworkPacket(nullptr);
			payloadSize += pStored->Size;
		}

		auto view = PackView(View, payloadSize);
		auto pendingChanges = PackView(PendingChanges, payloadSize);

		auto pPacket = ionet::CreateSharedPacket<StateUpdateMessagePacket>(payloadSize);

		auto pData = pPacket->payload();
		CopyProcessId(pData, pPackedSender);

		*pData++ = State.Acknowledgeable.has_value();
		if (State.Acknowledgeable) {
			memcpy(pData, pAcknowledgeable.get(), pAcknowledgeable->Size);
			pData += pAcknowledgeable->Size;
		}

		*pData++ = State.Conflicting.has_value();
		if (State.Conflicting) {
			memcpy(pData, pConflicting.get(), pConflicting->Size);
			pData += pConflicting->Size;
		}

		*pData++ = State.Stored.has_value();
		if (State.Stored) {
			memcpy(pData, pStored.get(), pStored->Size);
			pData += pStored->Size;
		}

		CopyView(pData, view);
		CopyView(pData, pendingChanges);

		MaybeSignMessage(pKeyPair, pPacket.get(), this);

		return pPacket;
	}

	std::shared_ptr<MessagePacket> DeliverMessage::toNetworkPacket(const crypto::KeyPair* pKeyPair) {
		auto pPackedSender = ionet::PackNode(Sender);
		uint32_t payloadSize = pPackedSender->Size;
		auto pPackedInitiator = ionet::PackNode(Initiator);
		payloadSize += pPackedInitiator->Size;
		payloadSize += Payload->Size;
		auto packedView = PackView(View, payloadSize);

		auto pPacket = ionet::CreateSharedPacket<DeliverMessagePacket>(payloadSize);

		auto pData = pPacket->payload();
		CopyProcessId(pData, pPackedSender);
		CopyProcessId(pData, pPackedInitiator);
		memcpy(pData, Payload.get(), Payload->Size);
		pData += Payload->Size;

		CopyView(pData, packedView);

		MaybeSignMessage(pKeyPair, pPacket.get(), this);

		return pPacket;
	}
}}