/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Messages.h"
#include "DbrbDefinitions.h"
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

		struct ConfirmDeliverMessagePacket : public MessagePacket {
			static constexpr ionet::PacketType Packet_Type = ionet::PacketType::Dbrb_Confirm_Deliver_Message;
		};

		struct ShardPrepareMessagePacket : public MessagePacket {
			static constexpr ionet::PacketType Packet_Type = ionet::PacketType::Dbrb_Shard_Prepare_Message;
		};

		struct ShardAcknowledgedMessagePacket : public MessagePacket {
			static constexpr ionet::PacketType Packet_Type = ionet::PacketType::Dbrb_Shard_Acknowledged_Message;
		};

		struct ShardCommitMessagePacket : public MessagePacket {
			static constexpr ionet::PacketType Packet_Type = ionet::PacketType::Dbrb_Shard_Commit_Message;
		};

		struct ShardDeliverMessagePacket : public MessagePacket {
			static constexpr ionet::PacketType Packet_Type = ionet::PacketType::Dbrb_Shard_Deliver_Message;
		};

#pragma pack(pop)

		template<typename MessagePacketType, typename MessageType>
		auto ToShardMessage(const ionet::Packet& packet) {
			const auto* pMessagePacket = reinterpret_cast<const MessagePacketType*>(&packet);
			auto pBuffer = pMessagePacket->payload();
			auto payloadHash = Read<Hash256>(pBuffer);
			auto certificate = Read<CertificateType>(pBuffer);

			return std::make_shared<MessageType>(pMessagePacket->Sender, payloadHash, certificate);
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
			const auto* pMessagePacket = reinterpret_cast<const PrepareMessagePacket*>(&packet);
			auto pBuffer = pMessagePacket->payload();
			auto payload = Read<Payload>(pBuffer);
			auto view = Read<View>(pBuffer);
			auto bootstrapView = Read<View>(pBuffer);

			return std::make_shared<PrepareMessage>(pMessagePacket->Sender, payload, view, bootstrapView);
		});

		registerConverter(ionet::PacketType::Dbrb_Acknowledged_Message, [](const ionet::Packet& packet) {
			const auto* pMessagePacket = reinterpret_cast<const AcknowledgedMessagePacket*>(&packet);
			auto pBuffer = pMessagePacket->payload();
			auto payloadHash = Read<Hash256>(pBuffer);
			auto view = Read<View>(pBuffer);
			auto payloadSignature = Read<Signature>(pBuffer);

			return std::make_shared<AcknowledgedMessage>(pMessagePacket->Sender, payloadHash, view, payloadSignature);
		});

		registerConverter(ionet::PacketType::Dbrb_Commit_Message, [](const ionet::Packet& packet) {
			const auto* pMessagePacket = reinterpret_cast<const CommitMessagePacket*>(&packet);
			auto pBuffer = pMessagePacket->payload();
			auto payloadHash = Read<Hash256>(pBuffer);
			auto certificate = Read<CertificateType>(pBuffer);
			auto view = Read<View>(pBuffer);

			return std::make_shared<CommitMessage>(pMessagePacket->Sender, payloadHash, certificate, view);
		});

		registerConverter(ionet::PacketType::Dbrb_Deliver_Message, [](const ionet::Packet& packet) {
			const auto* pMessagePacket = reinterpret_cast<const DeliverMessagePacket*>(&packet);
			auto pBuffer = pMessagePacket->payload();
			auto payloadHash = Read<Hash256>(pBuffer);
			auto view = Read<View>(pBuffer);

			return std::make_shared<DeliverMessage>(pMessagePacket->Sender, payloadHash, view);
		});

		registerConverter(ionet::PacketType::Dbrb_Confirm_Deliver_Message, [](const ionet::Packet& packet) {
			const auto* pMessagePacket = reinterpret_cast<const ConfirmDeliverMessagePacket*>(&packet);
			auto pBuffer = pMessagePacket->payload();
			auto payloadHash = Read<Hash256>(pBuffer);
			auto view = Read<View>(pBuffer);

			return std::make_shared<ConfirmDeliverMessage>(pMessagePacket->Sender, payloadHash, view);
		});

		registerConverter(ionet::PacketType::Dbrb_Shard_Prepare_Message, [](const ionet::Packet& packet) {
			const auto* pMessagePacket = reinterpret_cast<const ShardPrepareMessagePacket*>(&packet);
			auto pBuffer = pMessagePacket->payload();
			auto payload = Read<Payload>(pBuffer);
			auto view = Read<DbrbTreeView>(pBuffer);
			auto broadcasterSignature = Read<Signature>(pBuffer);

			return std::make_shared<ShardPrepareMessage>(pMessagePacket->Sender, payload, view, broadcasterSignature);
		});

		registerConverter(ionet::PacketType::Dbrb_Shard_Acknowledged_Message, [](const ionet::Packet& packet) {
			return ToShardMessage<ShardAcknowledgedMessagePacket, ShardAcknowledgedMessage>(packet);
		});

		registerConverter(ionet::PacketType::Dbrb_Shard_Commit_Message, [](const ionet::Packet& packet) {
			return ToShardMessage<ShardCommitMessagePacket, ShardCommitMessage>(packet);
		});

		registerConverter(ionet::PacketType::Dbrb_Shard_Deliver_Message, [](const ionet::Packet& packet) {
			return ToShardMessage<ShardDeliverMessagePacket, ShardDeliverMessage>(packet);
		});
	}

	std::shared_ptr<MessagePacket> PrepareMessage::toNetworkPacket() {
		auto pPacket = ionet::CreateSharedPacket<PrepareMessagePacket>(Payload->Size + View.packedSize() + BootstrapView.packedSize());
		pPacket->Sender = Sender;

		auto pBuffer = pPacket->payload();
		Write(pBuffer, Payload);
		Write(pBuffer, View);
		Write(pBuffer, BootstrapView);

		return pPacket;
	}

	std::shared_ptr<MessagePacket> AcknowledgedMessage::toNetworkPacket() {
		auto pPacket = ionet::CreateSharedPacket<AcknowledgedMessagePacket>(Hash256_Size + View.packedSize() + Signature_Size);
		pPacket->Sender = Sender;

		auto pBuffer = pPacket->payload();
		Write(pBuffer, PayloadHash);
		Write(pBuffer, View);
		Write(pBuffer, PayloadSignature);

		return pPacket;
	}

	std::shared_ptr<MessagePacket> CommitMessage::toNetworkPacket() {
		auto payloadSize = Hash256_Size + sizeof(uint32_t) + Certificate.size() * (ProcessId_Size + Signature_Size) + View.packedSize();
		auto pPacket = ionet::CreateSharedPacket<CommitMessagePacket>(payloadSize);
		pPacket->Sender = Sender;

		auto pBuffer = pPacket->payload();
		Write(pBuffer, PayloadHash);
		Write(pBuffer, Certificate);
		Write(pBuffer, View);

		return pPacket;
	}

	std::shared_ptr<MessagePacket> DeliverMessage::toNetworkPacket() {
		auto pPacket = ionet::CreateSharedPacket<DeliverMessagePacket>(Hash256_Size + View.packedSize());
		pPacket->Sender = Sender;

		auto pBuffer = pPacket->payload();
		Write(pBuffer, PayloadHash);
		Write(pBuffer, View);

		return pPacket;
	}

	std::shared_ptr<MessagePacket> ConfirmDeliverMessage::toNetworkPacket() {
		auto pPacket = ionet::CreateSharedPacket<ConfirmDeliverMessagePacket>(Hash256_Size + View.packedSize());
		pPacket->Sender = Sender;

		auto pBuffer = pPacket->payload();
		Write(pBuffer, PayloadHash);
		Write(pBuffer, View);

		return pPacket;
	}

	std::shared_ptr<MessagePacket> ShardPrepareMessage::toNetworkPacket() {
		auto pPacket = ionet::CreateSharedPacket<ShardPrepareMessagePacket>(Payload->Size + sizeof(uint32_t) + View.size() * ProcessId_Size + Signature_Size);
		pPacket->Sender = Sender;

		auto pBuffer = pPacket->payload();
		Write(pBuffer, Payload);
		Write(pBuffer, View);
		Write(pBuffer, BroadcasterSignature);

		return pPacket;
	}

	namespace {
		template<typename ShardMessagePacket>
		std::shared_ptr<MessagePacket> ToNetworkPacket(ShardBaseMessage* message) {
			auto pPacket = ionet::CreateSharedPacket<ShardMessagePacket>(Hash256_Size + sizeof(uint32_t) + message->Certificate.size() * (ProcessId_Size + Signature_Size));
			pPacket->Sender = message->Sender;

			auto pBuffer = pPacket->payload();
			Write(pBuffer, message->PayloadHash);
			Write(pBuffer, message->Certificate);

			return pPacket;
		}
	}

	std::shared_ptr<MessagePacket> ShardAcknowledgedMessage::toNetworkPacket() {
		return ToNetworkPacket<ShardAcknowledgedMessagePacket>(this);
	}

	std::shared_ptr<MessagePacket> ShardCommitMessage::toNetworkPacket() {
		return ToNetworkPacket<ShardCommitMessagePacket>(this);
	}

	std::shared_ptr<MessagePacket> ShardDeliverMessage::toNetworkPacket() {
		return ToNetworkPacket<ShardDeliverMessagePacket>(this);
	}
}}