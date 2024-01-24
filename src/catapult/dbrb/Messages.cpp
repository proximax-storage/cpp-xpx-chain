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

		struct PrepareMessagePacket : public MessagePacket {
			static constexpr ionet::PacketType Packet_Type = ionet::PacketType::Dbrb_Prepare_Message;
		};

		struct AcknowledgedMessagePacket : public MessagePacket {
			static constexpr ionet::PacketType Packet_Type = ionet::PacketType::Dbrb_Acknowledged_Message;
		};

		struct CommitMessagePacket : public MessagePacket {
			static constexpr ionet::PacketType Packet_Type = ionet::PacketType::Dbrb_Commit_Message;
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

			auto pMessage = std::make_shared<PrepareMessage>(pMessagePacket->Sender, payload, view);
			pMessage->Signature = pMessagePacket->Signature;

			return pMessage;
		}

		auto ToCommitMessage(const ionet::Packet& packet) {
			const auto* pMessagePacket = reinterpret_cast<const CommitMessagePacket*>(&packet);
			auto pBuffer = pMessagePacket->payload();
			auto payloadHash = Read<Hash256>(pBuffer);
			auto certificate = Read<CertificateType>(pBuffer);
			auto view = Read<View>(pBuffer);

			auto pMessage = std::make_shared<CommitMessage>(pMessagePacket->Sender, payloadHash, certificate, view);
			pMessage->Signature = pMessagePacket->Signature;

			return pMessage;
		}
	}

	std::shared_ptr<Message> NetworkPacketConverter::toMessage(const ionet::Packet& packet) const {
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

		registerConverter(ionet::PacketType::Dbrb_Prepare_Message, [](const ionet::Packet& packet) {
			return ToPrepareMessage(packet);
		});

		registerConverter(ionet::PacketType::Dbrb_Acknowledged_Message, [](const ionet::Packet& packet) {
			const auto* pMessagePacket = reinterpret_cast<const AcknowledgedMessagePacket*>(&packet);
			auto pBuffer = pMessagePacket->payload();
			auto payloadHash = Read<Hash256>(pBuffer);
			auto view = Read<View>(pBuffer);
			auto payloadSignature = Read<Signature>(pBuffer);

			auto pMessage = std::make_shared<AcknowledgedMessage>(pMessagePacket->Sender, payloadHash, view, payloadSignature);
			pMessage->Signature = pMessagePacket->Signature;

			return pMessage;
		});

		registerConverter(ionet::PacketType::Dbrb_Commit_Message, [](const ionet::Packet& packet) {
			return ToCommitMessage(packet);
		});

		registerConverter(ionet::PacketType::Dbrb_Deliver_Message, [](const ionet::Packet& packet) {
			const auto* pMessagePacket = reinterpret_cast<const DeliverMessagePacket*>(&packet);
			auto pBuffer = pMessagePacket->payload();
			auto payloadHash = Read<Hash256>(pBuffer);
			auto view = Read<View>(pBuffer);

			auto pMessage = std::make_shared<DeliverMessage>(pMessagePacket->Sender, payloadHash, view);
			pMessage->Signature = pMessagePacket->Signature;

			return pMessage;
		});
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
		auto pPacket = ionet::CreateSharedPacket<AcknowledgedMessagePacket>(Hash256_Size + View.packedSize() + Signature_Size);
		pPacket->Sender = Sender;

		auto pBuffer = pPacket->payload();
		Write(pBuffer, PayloadHash);
		Write(pBuffer, View);
		Write(pBuffer, PayloadSignature);

		MaybeSignMessage(pKeyPair, pPacket.get(), this);

		return pPacket;
	}

	std::shared_ptr<MessagePacket> CommitMessage::toNetworkPacket(const crypto::KeyPair* pKeyPair) {
		auto payloadSize = Hash256_Size + sizeof(uint32_t) + Certificate.size() * (ProcessId_Size + Signature_Size) + View.packedSize();
		auto pPacket = ionet::CreateSharedPacket<CommitMessagePacket>(payloadSize);
		pPacket->Sender = Sender;

		auto pBuffer = pPacket->payload();
		Write(pBuffer, PayloadHash);
		Write(pBuffer, Certificate);
		Write(pBuffer, View);

		MaybeSignMessage(pKeyPair, pPacket.get(), this);

		return pPacket;
	}

	std::shared_ptr<MessagePacket> DeliverMessage::toNetworkPacket(const crypto::KeyPair* pKeyPair) {
		auto pPacket = ionet::CreateSharedPacket<DeliverMessagePacket>(Hash256_Size + View.packedSize());
		pPacket->Sender = Sender;

		auto pBuffer = pPacket->payload();
		Write(pBuffer, PayloadHash);
		Write(pBuffer, View);

		MaybeSignMessage(pKeyPair, pPacket.get(), this);

		return pPacket;
	}
}}