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

			MembershipChanges MembershipChange;
		};

		struct ReconfigConfirmMessagePacket : public MessagePacket {
			static constexpr ionet::PacketType Packet_Type = ionet::PacketType::Dbrb_Reconfig_Confirm_Message;
		};

		struct ProposeMessagePacket : public MessagePacket {
			static constexpr ionet::PacketType Packet_Type = ionet::PacketType::Dbrb_Propose_Message;

			uint32_t ProposedSequenceSize;
		};

		struct ConvergedMessagePacket : public MessagePacket {
			static constexpr ionet::PacketType Packet_Type = ionet::PacketType::Dbrb_Converged_Message;

			uint32_t ConvergedSequenceSize;
		};

		struct InstallMessagePacket : public MessagePacket {
			static constexpr ionet::PacketType Packet_Type = ionet::PacketType::Dbrb_Install_Message;

			uint32_t ConvergedSequenceSize;
		};

		struct PrepareMessagePacket : public MessagePacket {
			static constexpr ionet::PacketType Packet_Type = ionet::PacketType::Dbrb_Prepare_Message;

			uint32_t PayloadSize;
		};

		struct AcknowledgedMessagePacket : public MessagePacket {
			static constexpr ionet::PacketType Packet_Type = ionet::PacketType::Dbrb_Acknowledged_Message;

			uint32_t PayloadSize;
			catapult::Signature Signature;
		};

		struct CommitMessagePacket : public MessagePacket {
			static constexpr ionet::PacketType Packet_Type = ionet::PacketType::Dbrb_Commit_Message;

			uint32_t PayloadSize;
			uint32_t CertificateSize;
		};

		struct StateUpdateMessagePacket : public MessagePacket {
			static constexpr ionet::PacketType Packet_Type = ionet::PacketType::Dbrb_State_Update_Message;

			bool HasAcknowledgeable;
			bool HasConflicting;
			bool HasStored;
		};

		struct DeliverMessagePacket : public MessagePacket {
			static constexpr ionet::PacketType Packet_Type = ionet::PacketType::Dbrb_Deliver_Message;

			uint32_t PayloadSize;
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
			const auto pMessagePacket = ionet::CoercePacket<PrepareMessagePacket>(&packet);
			auto pData = pMessagePacket->payload();
			auto sender = UnpackProcessId(pData);
			auto payload = ionet::CreateSharedPacket<ionet::Packet>(pMessagePacket->PayloadSize);
			memcpy(payload.get(), pData, pMessagePacket->PayloadSize);
			pData += pMessagePacket->PayloadSize;

			auto pMessage = std::make_unique<PrepareMessage>(sender, payload, UnpackView(pData));
			pMessage->Signature = pMessagePacket->Signature;

			return pMessage;
		}

		auto ToCommitMessage(const ionet::Packet& packet) {
			const auto pMessagePacket = ionet::CoercePacket<CommitMessagePacket>(&packet);
			auto pData = pMessagePacket->payload();
			auto sender = UnpackProcessId(pData);
			auto payload = ionet::CreateSharedPacket<ionet::Packet>(pMessagePacket->PayloadSize);
			memcpy(payload.get(), pData, pMessagePacket->PayloadSize);
			pData += pMessagePacket->PayloadSize;

			std::map<ProcessId, Signature> certificate;
			for (auto i = 0u; i < pMessagePacket->CertificateSize; ++i) {
				const auto& networkNode = *reinterpret_cast<const ionet::NetworkNode*>(pData);
				auto node = ionet::UnpackNode(networkNode);
				pData += networkNode.Size;
				Signature signature;
				memcpy(signature.data(), pData, Signature_Size);
				pData += Signature_Size;
				certificate.emplace(node, signature);
			}

			auto pMessage = std::make_unique<CommitMessage>(sender, payload, certificate, UnpackView(pData), UnpackView(pData));
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
			const auto pMessagePacket = ionet::CoercePacket<ReconfigMessagePacket>(&packet);
			auto pData = pMessagePacket->payload();

			auto pMessage = std::make_unique<ReconfigMessage>(UnpackProcessId(pData), UnpackProcessId(pData), pMessagePacket->MembershipChange, UnpackView(pData));
			pMessage->Signature = pMessagePacket->Signature;

			return pMessage;
		});

		registerConverter(ionet::PacketType::Dbrb_Reconfig_Confirm_Message, [](const ionet::Packet& packet) {
			const auto pMessagePacket = ionet::CoercePacket<ReconfigConfirmMessagePacket>(&packet);
			auto pData = pMessagePacket->payload();

			auto pMessage = std::make_unique<ReconfigConfirmMessage>(UnpackProcessId(pData), UnpackView(pData));
			pMessage->Signature = pMessagePacket->Signature;

			return pMessage;
		});

		registerConverter(ionet::PacketType::Dbrb_Propose_Message, [](const ionet::Packet& packet) {
			const auto pMessagePacket = ionet::CoercePacket<ProposeMessagePacket>(&packet);
			auto pData = pMessagePacket->payload();
			auto sender = UnpackProcessId(pData);
			View replacedView = UnpackView(pData);
			Sequence proposedSequence;
			for (auto i = 0u; i < pMessagePacket->ProposedSequenceSize; ++i)
				proposedSequence.tryAppend(UnpackView(pData));

			auto pMessage = std::make_unique<ProposeMessage>(sender, proposedSequence, replacedView);
			pMessage->Signature = pMessagePacket->Signature;

			return pMessage;
		});

		registerConverter(ionet::PacketType::Dbrb_Converged_Message, [](const ionet::Packet& packet) {
			const auto pMessagePacket = ionet::CoercePacket<ConvergedMessagePacket>(&packet);
			auto pData = pMessagePacket->payload();
			auto sender = UnpackProcessId(pData);
			View replacedView = UnpackView(pData);
			Sequence convergedSequence;
			for (auto i = 0u; i < pMessagePacket->ConvergedSequenceSize; ++i)
				convergedSequence.tryAppend(UnpackView(pData));

			auto pMessage = std::make_unique<ConvergedMessage>(sender, convergedSequence, replacedView);
			pMessage->Signature = pMessagePacket->Signature;

			return pMessage;
		});

		registerConverter(ionet::PacketType::Dbrb_Install_Message, [](const ionet::Packet& packet) {
			const auto pMessagePacket = ionet::CoercePacket<InstallMessagePacket>(&packet);
			auto pData = pMessagePacket->payload();
			auto sender = UnpackProcessId(pData);
			View leastRecentView = UnpackView(pData);
			View replacedView = UnpackView(pData);
			Sequence convergedSequence;
			for (auto i = 0u; i < pMessagePacket->ConvergedSequenceSize; ++i)
				convergedSequence.tryAppend(UnpackView(pData));

			auto pMessage = std::make_unique<InstallMessage>(sender, leastRecentView, convergedSequence, replacedView);
			pMessage->Signature = pMessagePacket->Signature;

			return pMessage;
		});

		registerConverter(ionet::PacketType::Dbrb_Prepare_Message, [](const ionet::Packet& packet) {
			return ToPrepareMessage(packet);
		});

		registerConverter(ionet::PacketType::Dbrb_Acknowledged_Message, [](const ionet::Packet& packet) {
			const auto pMessagePacket = ionet::CoercePacket<AcknowledgedMessagePacket>(&packet);
			auto pData = pMessagePacket->payload();
			auto sender = UnpackProcessId(pData);
			auto payload = ionet::CreateSharedPacket<ionet::Packet>(pMessagePacket->PayloadSize);
			memcpy(payload.get(), pData, pMessagePacket->PayloadSize);
			pData += pMessagePacket->PayloadSize;

			auto pMessage = std::make_unique<AcknowledgedMessage>(sender, payload, UnpackView(pData), pMessagePacket->Signature);
			pMessage->Signature = pMessagePacket->Signature;

			return pMessage;
		});

		registerConverter(ionet::PacketType::Dbrb_Commit_Message, [](const ionet::Packet& packet) {
			return ToCommitMessage(packet);
		});

		registerConverter(ionet::PacketType::Dbrb_State_Update_Message, [](const ionet::Packet& packet) {
			const auto pMessagePacket = ionet::CoercePacket<StateUpdateMessagePacket>(&packet);
			auto pData = pMessagePacket->payload();
			auto sender = UnpackProcessId(pData);

			ProcessState state;
			if (pMessagePacket->HasAcknowledgeable) {
				const auto* pPrepareMessagePacket = ionet::CoercePacket<PrepareMessagePacket>(reinterpret_cast<const ionet::Packet*>(pData));
				state.Acknowledgeable = *ToPrepareMessage(*pPrepareMessagePacket);
				pData += pPrepareMessagePacket->Size;
			}

			if (pMessagePacket->HasConflicting) {
				const auto* pPrepareMessagePacket1 = ionet::CoercePacket<PrepareMessagePacket>(reinterpret_cast<const ionet::Packet*>(pData));
				pData += pPrepareMessagePacket1->Size;
				const auto* pPrepareMessagePacket2 = ionet::CoercePacket<PrepareMessagePacket>(reinterpret_cast<const ionet::Packet*>(pData));
				pData += pPrepareMessagePacket2->Size;
				state.Conflicting = std::make_pair(*ToPrepareMessage(*pPrepareMessagePacket1), *ToPrepareMessage(*pPrepareMessagePacket2));
			}

			if (pMessagePacket->HasStored) {
				const auto* pCommitMessagePacket = ionet::CoercePacket<CommitMessagePacket>(reinterpret_cast<const ionet::Packet*>(pData));
				state.Stored = *ToCommitMessage(*pCommitMessagePacket);
				pData += pCommitMessagePacket->Size;
			}

			auto pMessage = std::make_unique<StateUpdateMessage>(sender, state, UnpackView(pData), UnpackView(pData));
			pMessage->Signature = pMessagePacket->Signature;

			return pMessage;
		});

		registerConverter(ionet::PacketType::Dbrb_Deliver_Message, [](const ionet::Packet& packet) {
			const auto pMessagePacket = ionet::CoercePacket<DeliverMessagePacket>(&packet);
			auto pData = pMessagePacket->payload();
			auto sender = UnpackProcessId(pData);
			auto payload = ionet::CreateSharedPacket<ionet::Packet>(pMessagePacket->PayloadSize);
			memcpy(payload.get(), pData, pMessagePacket->PayloadSize);
			pData += pMessagePacket->PayloadSize;

			auto pMessage = std::make_unique<DeliverMessage>(sender, payload, UnpackView(pData));
			pMessage->Signature = pMessagePacket->Signature;

			return pMessage;
		});
	}

	std::shared_ptr<MessagePacket> ReconfigMessage::toNetworkPacket(const crypto::KeyPair* pKeyPair) {
		uint32_t payloadSize = 0u;
		auto packedView = PackView(View, payloadSize);

		auto pPacket = ionet::CreateSharedPacket<ReconfigMessagePacket>(payloadSize);
		pPacket->MembershipChange = MembershipChange;

		auto pData = pPacket->payload();
		PackProcessId(pData, Sender);
		PackProcessId(pData, ProcessId);
		CopyView(pData, packedView);

		MaybeSignMessage(pKeyPair, pPacket.get(), this);

		return pPacket;
	}

	std::shared_ptr<MessagePacket> ReconfigConfirmMessage::toNetworkPacket(const crypto::KeyPair* pKeyPair) {
		uint32_t payloadSize = 0u;
		auto packedView = PackView(View, payloadSize);
		auto pPacket = ionet::CreateSharedPacket<ReconfigConfirmMessagePacket>(payloadSize);

		auto pData = pPacket->payload();
		PackProcessId(pData, Sender);
		CopyView(pData, packedView);

		MaybeSignMessage(pKeyPair, pPacket.get(), this);

		return pPacket;
	}

	std::shared_ptr<MessagePacket> ProposeMessage::toNetworkPacket(const crypto::KeyPair* pKeyPair) {
		uint32_t payloadSize = 0u;
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
		pPacket->ProposedSequenceSize = utils::checked_cast<size_t, uint32_t>(ProposedSequence.data().size());

		auto pData = pPacket->payload();
		PackProcessId(pData, Sender);
		for (const auto& packedView : packedViews)
			CopyView(pData, packedView);

		MaybeSignMessage(pKeyPair, pPacket.get(), this);

		return pPacket;
	}

	std::shared_ptr<MessagePacket> ConvergedMessage::toNetworkPacket(const crypto::KeyPair* pKeyPair) {
		uint32_t payloadSize = 0u;
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
		pPacket->ConvergedSequenceSize = utils::checked_cast<size_t, uint32_t>(ConvergedSequence.data().size());

		auto pData = pPacket->payload();
		PackProcessId(pData, Sender);
		for (const auto& packedView : packedViews)
			CopyView(pData, packedView);

		MaybeSignMessage(pKeyPair, pPacket.get(), this);

		return pPacket;
	}

	std::shared_ptr<MessagePacket> InstallMessage::toNetworkPacket(const crypto::KeyPair* pKeyPair) {
		uint32_t payloadSize = 0u;
		std::vector<const View*> views;
		std::vector<PackedView> packedViews;

		views.reserve(ConvergedSequence.data().size() + 2u);
		packedViews.reserve(views.size());

		views.emplace_back(&LeastRecentView);
		packedViews.emplace_back(PackView(LeastRecentView, payloadSize));

		views.emplace_back(&ReplacedView);
		packedViews.emplace_back(PackView(ReplacedView, payloadSize));

		for (const auto& view : ConvergedSequence.data()) {
			views.emplace_back(&view);
			packedViews.emplace_back(PackView(view, payloadSize));
		}

		auto pPacket = ionet::CreateSharedPacket<InstallMessagePacket>(payloadSize);

		auto pData = pPacket->payload();
		PackProcessId(pData, Sender);
		for (const auto& packedView : packedViews)
			CopyView(pData, packedView);

		MaybeSignMessage(pKeyPair, pPacket.get(), this);

		return pPacket;
	}

	std::shared_ptr<MessagePacket> PrepareMessage::toNetworkPacket(const crypto::KeyPair* pKeyPair) {
		uint32_t payloadSize = Payload->Size;
		auto packedView = PackView(View, payloadSize);
		auto pPacket = ionet::CreateSharedPacket<PrepareMessagePacket>(payloadSize);
		pPacket->PayloadSize = Payload->Size;

		auto pData = pPacket->payload();
		PackProcessId(pData, Sender);
		memcpy(pData, Payload.get(), Payload->Size);
		pData += Payload->Size;

		CopyView(pData, packedView);

		MaybeSignMessage(pKeyPair, pPacket.get(), this);

		return pPacket;
	}

	std::shared_ptr<MessagePacket> AcknowledgedMessage::toNetworkPacket(const crypto::KeyPair* pKeyPair) {
		uint32_t payloadSize = Payload->Size;
		auto packedView = PackView(View, payloadSize);
		auto pPacket = ionet::CreateSharedPacket<AcknowledgedMessagePacket>(payloadSize);
		pPacket->Signature = Signature;
		pPacket->PayloadSize = Payload->Size;

		auto pData = pPacket->payload();
		PackProcessId(pData, Sender);
		memcpy(pData, Payload.get(), Payload->Size);
		pData += Payload->Size;

		CopyView(pData, packedView);

		MaybeSignMessage(pKeyPair, pPacket.get(), this);

		return pPacket;
	}

	std::shared_ptr<MessagePacket> CommitMessage::toNetworkPacket(const crypto::KeyPair* pKeyPair) {
		uint32_t certificateSize = utils::checked_cast<size_t, uint32_t>(Certificate.size());
		uint32_t payloadSize = Payload->Size + utils::checked_cast<size_t, uint32_t>(Signature_Size * certificateSize);
		auto packedCertificateView = PackView(CertificateView, payloadSize);
		auto packedCurrentView = PackView(CurrentView, payloadSize);
		auto pPacket = ionet::CreateSharedPacket<CommitMessagePacket>(payloadSize);
		pPacket->PayloadSize = Payload->Size;
		pPacket->CertificateSize = certificateSize;

		auto pData = pPacket->payload();
		PackProcessId(pData, Sender);
		memcpy(pData, Payload.get(), Payload->Size);
		pData += Payload->Size;

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
		uint32_t payloadSize = 0u;

		std::shared_ptr<ionet::Packet> pAcknowledgeable;
		if (State.Acknowledgeable) {
			pAcknowledgeable = State.Acknowledgeable.value().toNetworkPacket(nullptr);
			payloadSize += pAcknowledgeable->Size;
		}

		std::shared_ptr<ionet::Packet> pConflicting1;
		std::shared_ptr<ionet::Packet> pConflicting2;
		if (State.Conflicting) {
			pConflicting1 = State.Conflicting.value().first.toNetworkPacket(nullptr);
			payloadSize += pConflicting1->Size;
			pConflicting2 = State.Conflicting.value().second.toNetworkPacket(nullptr);
			payloadSize += pConflicting2->Size;
		}

		std::shared_ptr<ionet::Packet> pStored;
		if (State.Stored) {
			pStored = State.Stored.value().toNetworkPacket(nullptr);
			payloadSize += pStored->Size;
		}

		auto view = PackView(View, payloadSize);
		auto pendingChanges = PackView(PendingChanges, payloadSize);

		auto pPacket = ionet::CreateSharedPacket<StateUpdateMessagePacket>(payloadSize);
		pPacket->HasAcknowledgeable = State.Acknowledgeable.has_value();
		pPacket->HasConflicting = State.Conflicting.has_value();
		pPacket->HasStored = State.Stored.has_value();

		auto pData = pPacket->payload();
		PackProcessId(pData, Sender);

		if (pPacket->HasAcknowledgeable) {
			memcpy(pData, pAcknowledgeable.get(), pAcknowledgeable->Size);
			pData += pAcknowledgeable->Size;
		}

		if (pPacket->HasConflicting) {
			memcpy(pData, pConflicting1.get(), pConflicting1->Size);
			pData += pConflicting1->Size;
			memcpy(pData, pConflicting2.get(), pConflicting2->Size);
			pData += pConflicting2->Size;
		}

		if (pPacket->HasStored) {
			memcpy(pData, pStored.get(), pStored->Size);
			pData += pStored->Size;
		}

		CopyView(pData, view);
		CopyView(pData, pendingChanges);

		MaybeSignMessage(pKeyPair, pPacket.get(), this);

		return pPacket;
	}

	std::shared_ptr<MessagePacket> DeliverMessage::toNetworkPacket(const crypto::KeyPair* pKeyPair) {
		uint32_t payloadSize = Payload->Size;
		auto packedView = PackView(View, payloadSize);
		auto pPacket = ionet::CreateSharedPacket<DeliverMessagePacket>(payloadSize);
		pPacket->PayloadSize = Payload->Size;

		auto pData = pPacket->payload();
		PackProcessId(pData, Sender);
		memcpy(pData, Payload.get(), Payload->Size);
		pData += Payload->Size;

		CopyView(pData, packedView);

		MaybeSignMessage(pKeyPair, pPacket.get(), this);

		return pPacket;
	}
}}