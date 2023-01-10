/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Messages.h"
#include "catapult/crypto/Signer.h"
#include "catapult/utils/Casting.h"

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
			auto pBuffer = pMessagePacket->payload();
			auto payload = Read<Payload>(pBuffer);
			auto view = Read<View>(pBuffer);

			auto pMessage = std::make_unique<PrepareMessage>(pMessagePacket->Sender, payload, view);
			pMessage->Signature = pMessagePacket->Signature;

			return pMessage;
		}

		auto ToCommitMessage(const ionet::Packet& packet) {
			const auto* pMessagePacket = reinterpret_cast<const CommitMessagePacket*>(&packet);
			auto pBuffer = pMessagePacket->payload();
			auto initiator = Read<ProcessId>(pBuffer);
			auto payload = Read<Payload>(pBuffer);
			auto certificate = Read<CertificateType>(pBuffer);
			auto certificateView = Read<View>(pBuffer);
			auto currentView = Read<View>(pBuffer);

			auto pMessage = std::make_unique<CommitMessage>(pMessagePacket->Sender, initiator, payload, certificate, certificateView, currentView);
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
			auto pBuffer = pMessagePacket->payload();
			auto processId = Read<ProcessId>(pBuffer);
			auto change = Read<MembershipChange>(pBuffer);
			auto view = Read<View>(pBuffer);

			auto pMessage = std::make_unique<ReconfigMessage>(pMessagePacket->Sender, processId, change, view);
			pMessage->Signature = pMessagePacket->Signature;

			return pMessage;
		});

		registerConverter(ionet::PacketType::Dbrb_Reconfig_Confirm_Message, [](const ionet::Packet& packet) {
			const auto* pMessagePacket = reinterpret_cast<const ReconfigConfirmMessagePacket*>(&packet);
			auto pBuffer = pMessagePacket->payload();
			auto view = Read<View>(pBuffer);

			auto pMessage = std::make_unique<ReconfigConfirmMessage>(pMessagePacket->Sender, view);
			pMessage->Signature = pMessagePacket->Signature;

			return pMessage;
		});

		registerConverter(ionet::PacketType::Dbrb_Propose_Message, [](const ionet::Packet& packet) {
			const auto* pMessagePacket = reinterpret_cast<const ProposeMessagePacket*>(&packet);
			auto pBuffer = pMessagePacket->payload();
			auto proposedSequence = Read<Sequence>(pBuffer);
			auto replacedView = Read<View>(pBuffer);

			auto pMessage = std::make_unique<ProposeMessage>(pMessagePacket->Sender, proposedSequence, replacedView);
			pMessage->Signature = pMessagePacket->Signature;

			return pMessage;
		});

		registerConverter(ionet::PacketType::Dbrb_Converged_Message, [](const ionet::Packet& packet) {
			const auto* pMessagePacket = reinterpret_cast<const ConvergedMessagePacket*>(&packet);
			auto pBuffer = pMessagePacket->payload();
			auto convergedSequence = Read<Sequence>(pBuffer);
			auto replacedView = Read<View>(pBuffer);

			auto pMessage = std::make_unique<ConvergedMessage>(pMessagePacket->Sender, convergedSequence, replacedView);
			pMessage->Signature = pMessagePacket->Signature;

			return pMessage;
		});

		registerConverter(ionet::PacketType::Dbrb_Install_Message, [](const ionet::Packet& packet) {
			const auto* pMessagePacket = reinterpret_cast<const InstallMessagePacket*>(&packet);
			auto pBuffer = pMessagePacket->payload();
			auto sequence = Read<Sequence>(pBuffer);
			auto signatures = Read<CertificateType>(pBuffer);

			auto pMessage = std::make_unique<InstallMessage>(pMessagePacket->Sender, sequence, signatures);
			pMessage->Signature = pMessagePacket->Signature;

			return pMessage;
		});

		registerConverter(ionet::PacketType::Dbrb_Prepare_Message, [](const ionet::Packet& packet) {
			return ToPrepareMessage(packet);
		});

		registerConverter(ionet::PacketType::Dbrb_Acknowledged_Message, [](const ionet::Packet& packet) {
			const auto* pMessagePacket = reinterpret_cast<const AcknowledgedMessagePacket*>(&packet);
			auto pBuffer = pMessagePacket->payload();
			auto initiator = Read<ProcessId>(pBuffer);
			auto payload = Read<Payload>(pBuffer);
			auto view = Read<View>(pBuffer);
			auto payloadSignature = Read<Signature>(pBuffer);

			auto pMessage = std::make_unique<AcknowledgedMessage>(pMessagePacket->Sender, initiator, payload, view, payloadSignature);
			pMessage->Signature = pMessagePacket->Signature;

			return pMessage;
		});

		registerConverter(ionet::PacketType::Dbrb_Commit_Message, [](const ionet::Packet& packet) {
			return ToCommitMessage(packet);
		});

		registerConverter(ionet::PacketType::Dbrb_State_Update_Message, [](const ionet::Packet& packet) {
			const auto* pMessagePacket = reinterpret_cast<const StateUpdateMessagePacket*>(&packet);
			auto pBuffer = pMessagePacket->payload();

			ProcessState state;
			if (*pBuffer++) {
				const auto* pPrepareMessagePacket = reinterpret_cast<const PrepareMessagePacket*>(pBuffer);
				state.Acknowledgeable = *ToPrepareMessage(*pPrepareMessagePacket);
				pBuffer += pPrepareMessagePacket->Size;
			}

			if (*pBuffer++) {
				const auto* pPrepareMessagePacket = reinterpret_cast<const PrepareMessagePacket*>(pBuffer);
				pBuffer += pPrepareMessagePacket->Size;
				state.Conflicting = *ToPrepareMessage(*pPrepareMessagePacket);
			}

			if (*pBuffer++) {
				const auto* pCommitMessagePacket = reinterpret_cast<const CommitMessagePacket*>(pBuffer);
				state.Stored = *ToCommitMessage(*pCommitMessagePacket);
				pBuffer += pCommitMessagePacket->Size;
			}
			auto view = Read<View>(pBuffer);
			auto pendingChanges = Read<View>(pBuffer);

			auto pMessage = std::make_unique<StateUpdateMessage>(pMessagePacket->Sender, state, view, pendingChanges);
			pMessage->Signature = pMessagePacket->Signature;

			return pMessage;
		});

		registerConverter(ionet::PacketType::Dbrb_Deliver_Message, [](const ionet::Packet& packet) {
			const auto* pMessagePacket = reinterpret_cast<const DeliverMessagePacket*>(&packet);
			auto pBuffer = pMessagePacket->payload();
			auto initiator = Read<ProcessId>(pBuffer);
			auto payload = Read<Payload>(pBuffer);
			auto view = Read<View>(pBuffer);

			auto pMessage = std::make_unique<DeliverMessage>(pMessagePacket->Sender, initiator, payload, view);
			pMessage->Signature = pMessagePacket->Signature;

			return pMessage;
		});
	}

	std::shared_ptr<MessagePacket> ReconfigMessage::toNetworkPacket(const crypto::KeyPair* pKeyPair) {
		auto pPacket = ionet::CreateSharedPacket<ReconfigMessagePacket>(ProcessId_Size + 1u + View.packedSize());
		pPacket->Sender = Sender;

		auto pBuffer = pPacket->payload();
		Write(pBuffer, ProcessId);
		Write(pBuffer, utils::to_underlying_type(MembershipChange));
		Write(pBuffer, View);

		MaybeSignMessage(pKeyPair, pPacket.get(), this);

		return pPacket;
	}

	std::shared_ptr<MessagePacket> ReconfigConfirmMessage::toNetworkPacket(const crypto::KeyPair* pKeyPair) {
		auto pPacket = ionet::CreateSharedPacket<ReconfigConfirmMessagePacket>(View.packedSize());
		pPacket->Sender = Sender;

		auto pBuffer = pPacket->payload();
		Write(pBuffer, View);

		MaybeSignMessage(pKeyPair, pPacket.get(), this);

		return pPacket;
	}

	std::shared_ptr<MessagePacket> ProposeMessage::toNetworkPacket(const crypto::KeyPair* pKeyPair) {
		auto pPacket = ionet::CreateSharedPacket<ProposeMessagePacket>(ProposedSequence.packedSize() + ReplacedView.packedSize());
		pPacket->Sender = Sender;

		auto pBuffer = pPacket->payload();
		Write(pBuffer, ProposedSequence);
		Write(pBuffer, ReplacedView);

		MaybeSignMessage(pKeyPair, pPacket.get(), this);

		return pPacket;
	}

	std::shared_ptr<MessagePacket> ConvergedMessage::toNetworkPacket(const crypto::KeyPair* pKeyPair) {
		auto pPacket = ionet::CreateSharedPacket<ConvergedMessagePacket>(ConvergedSequence.packedSize() + ReplacedView.packedSize());
		pPacket->Sender = Sender;

		auto pBuffer = pPacket->payload();
		Write(pBuffer, ConvergedSequence);
		Write(pBuffer, ReplacedView);

		MaybeSignMessage(pKeyPair, pPacket.get(), this);

		return pPacket;
	}

	std::shared_ptr<MessagePacket> InstallMessage::toNetworkPacket(const crypto::KeyPair* pKeyPair) {
		auto pPacket = ionet::CreateSharedPacket<InstallMessagePacket>(Sequence.packedSize() + sizeof(uint32_t) + ConvergedSignatures.size() * (ProcessId_Size + Signature_Size));
		pPacket->Sender = Sender;

		auto pBuffer = pPacket->payload();
		Write(pBuffer, Sequence);
		Write(pBuffer, ConvergedSignatures);

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
		auto pPacket = ionet::CreateSharedPacket<PrepareMessagePacket>(Payload->Size + View.packedSize());
		pPacket->Sender = Sender;

		auto pBuffer = pPacket->payload();
		Write(pBuffer, Payload);
		Write(pBuffer, View);

		MaybeSignMessage(pKeyPair, pPacket.get(), this);

		return pPacket;
	}

	std::shared_ptr<MessagePacket> AcknowledgedMessage::toNetworkPacket(const crypto::KeyPair* pKeyPair) {
		auto pPacket = ionet::CreateSharedPacket<AcknowledgedMessagePacket>(ProcessId_Size + Payload->Size + View.packedSize() + Signature_Size);
		pPacket->Sender = Sender;

		auto pBuffer = pPacket->payload();
		Write(pBuffer, Initiator);
		Write(pBuffer, Payload);
		Write(pBuffer, View);
		Write(pBuffer, PayloadSignature);

		MaybeSignMessage(pKeyPair, pPacket.get(), this);

		return pPacket;
	}

	std::shared_ptr<MessagePacket> CommitMessage::toNetworkPacket(const crypto::KeyPair* pKeyPair) {
		auto payloadSize = ProcessId_Size + Payload->Size + sizeof(uint32_t) + Certificate.size() * (ProcessId_Size + Signature_Size) + CertificateView.packedSize() + CurrentView.packedSize();
		auto pPacket = ionet::CreateSharedPacket<CommitMessagePacket>(payloadSize);
		pPacket->Sender = Sender;

		auto pBuffer = pPacket->payload();
		Write(pBuffer, Initiator);
		Write(pBuffer, Payload);
		Write(pBuffer, Certificate);
		Write(pBuffer, CertificateView);
		Write(pBuffer, CurrentView);

		MaybeSignMessage(pKeyPair, pPacket.get(), this);

		return pPacket;
	}

	std::shared_ptr<MessagePacket> StateUpdateMessage::toNetworkPacket(const crypto::KeyPair* pKeyPair) {
		uint32_t payloadSize = View.packedSize() + PendingChanges.packedSize();

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

		auto pPacket = ionet::CreateSharedPacket<StateUpdateMessagePacket>(payloadSize);
		pPacket->Sender = Sender;

		auto pBuffer = pPacket->payload();

		*pBuffer++ = State.Acknowledgeable.has_value();
		if (State.Acknowledgeable) {
			memcpy(pBuffer, pAcknowledgeable.get(), pAcknowledgeable->Size);
			pBuffer += pAcknowledgeable->Size;
		}

		*pBuffer++ = State.Conflicting.has_value();
		if (State.Conflicting) {
			memcpy(pBuffer, pConflicting.get(), pConflicting->Size);
			pBuffer += pConflicting->Size;
		}

		*pBuffer++ = State.Stored.has_value();
		if (State.Stored) {
			memcpy(pBuffer, pStored.get(), pStored->Size);
			pBuffer += pStored->Size;
		}

		Write(pBuffer, View);
		Write(pBuffer, PendingChanges);

		MaybeSignMessage(pKeyPair, pPacket.get(), this);

		return pPacket;
	}

	std::shared_ptr<MessagePacket> DeliverMessage::toNetworkPacket(const crypto::KeyPair* pKeyPair) {
		auto pPacket = ionet::CreateSharedPacket<DeliverMessagePacket>(ProcessId_Size + Payload->Size + View.packedSize());
		pPacket->Sender = Sender;

		auto pBuffer = pPacket->payload();
		Write(pBuffer, Initiator);
		Write(pBuffer, Payload);
		Write(pBuffer, View);

		MaybeSignMessage(pKeyPair, pPacket.get(), this);

		return pPacket;
	}
}}