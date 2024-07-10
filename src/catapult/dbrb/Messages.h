/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "View.h"
#include <utility>

namespace catapult { namespace crypto { class KeyPair; }}

namespace catapult { namespace dbrb {

#pragma pack(push, 1)

	struct MessagePacket : public ionet::Packet {
		catapult::Signature Signature;
		ProcessId Sender;

		/// Returns a non-const pointer to data contained in this packet.
		uint8_t* payload() {
			return Size <= sizeof(Packet) ? nullptr : reinterpret_cast<uint8_t*>(this) + sizeof(Packet) + Signature_Size + ProcessId_Size;
		}

		/// Returns a const pointer to data contained in this packet.
		constexpr const uint8_t* payload() const {
			return Size <= sizeof(Packet) ? nullptr : reinterpret_cast<const uint8_t*>(this) + sizeof(Packet) + Signature_Size + ProcessId_Size;
		}

		/// Returns buffers for signing this packet.
		std::vector<RawBuffer> buffers() const {
			auto pBegin = reinterpret_cast<const uint8_t*>(this);
			return Size <= sizeof(MessagePacket) ?
				std::vector<RawBuffer>{} :
				std::vector<RawBuffer>{ { pBegin, sizeof(ionet::Packet) }, { pBegin + sizeof(ionet::Packet) + Signature_Size, Size - sizeof(ionet::Packet) - Signature_Size } };
		}
	};

#pragma pack(pop)

	struct Message;

	class NetworkPacketConverter {
	public:
		using PacketConverter = std::function<std::shared_ptr<Message> (const ionet::Packet& packet)>;

	public:
		NetworkPacketConverter();

	public:
		/// Registers a \a converter for the specified packet \a type.
		void registerConverter(ionet::PacketType type, const PacketConverter& converter);

		/// Converts \a packet to a message or throws if converter of packet type is not registered.
		std::shared_ptr<Message> toMessage(const ionet::Packet& packet) const;

	private:
		const PacketConverter* findConverter(const ionet::Packet& packet) const;

	private:
		std::vector<PacketConverter> m_converters;
	};

	/// Base class for all messages sent via DBRB protocol.
	struct Message {
	public:
		Message() = delete;
		Message(const ProcessId& sender, ionet::PacketType type): Sender(sender), Type(type) {}
		virtual ~Message() = default;

	public:
		/// Creates a network packet representing this message.
		virtual std::shared_ptr<MessagePacket> toNetworkPacket(const crypto::KeyPair* pKeyPair) = 0;

		/// Compares this message with \a rhs.
		bool operator<(const Message& rhs) const {
			return Sender < rhs.Sender;
		}

	public:
		/// Sender of the message.
		ProcessId Sender;

		/// Type of the packet.
		ionet::PacketType Type;

		/// This message signed by sender.
		catapult::Signature Signature;
	};

	/// Base class for all messages sent via DBRB protocol.
	struct BaseMessage : Message {
	public:
		BaseMessage() = delete;
		BaseMessage(const ProcessId& sender, ionet::PacketType type, View view): Message(sender, type), View(std::move(view)) {}

	public:
		/// Creates a network packet representing this message.
		virtual std::shared_ptr<MessagePacket> toNetworkPacket(const crypto::KeyPair* pKeyPair) = 0;

	public:
		/// Current view of the system from the perspective of Sender.
		dbrb::View View;
	};

	struct PrepareMessage : BaseMessage {
	public:
		PrepareMessage() = delete;
		PrepareMessage(const ProcessId& sender, Payload payload, dbrb::View view, dbrb::View bootstrapView)
			: BaseMessage(sender, ionet::PacketType::Dbrb_Prepare_Message, std::move(view))
			, Payload(std::move(payload))
			, BootstrapView(std::move(bootstrapView))
		{}

	public:
		std::shared_ptr<MessagePacket> toNetworkPacket(const crypto::KeyPair* pKeyPair) override;

	public:
		/// Message to be broadcast.
		dbrb::Payload Payload;

		/// Current bootstrap view of the system from the perspective of the sender.
		dbrb::View BootstrapView;
	};

	struct AcknowledgedMessage : BaseMessage {
	public:
		AcknowledgedMessage() = delete;
		explicit AcknowledgedMessage(const ProcessId& sender, const Hash256& payloadHash, dbrb::View view, const catapult::Signature& payloadSignature)
			: BaseMessage(sender, ionet::PacketType::Dbrb_Acknowledged_Message, std::move(view))
			, PayloadHash(payloadHash)
			, PayloadSignature(payloadSignature)
		{}

	public:
		std::shared_ptr<MessagePacket> toNetworkPacket(const crypto::KeyPair* pKeyPair) override;

	public:
		/// Hash of the payload.
		Hash256 PayloadHash;

		/// Signature formed by Sender.
		catapult::Signature PayloadSignature;
	};

	struct CommitMessage : BaseMessage {
	public:
		CommitMessage() = delete;
		explicit CommitMessage(const ProcessId& sender, const Hash256& payloadHash, CertificateType certificate, dbrb::View view)
			: BaseMessage(sender, ionet::PacketType::Dbrb_Commit_Message, std::move(view))
			, PayloadHash(payloadHash)
			, Certificate(std::move(certificate))
		{}

	public:
		std::shared_ptr<MessagePacket> toNetworkPacket(const crypto::KeyPair* pKeyPair) override;

	public:
		/// Hash of the payload.
		Hash256 PayloadHash;

		/// Message certificate for supplied payload.
		CertificateType Certificate;
	};

	struct DeliverMessage : BaseMessage {
	public:
		DeliverMessage() = delete;
		explicit DeliverMessage(const ProcessId& sender, const Hash256& payloadHash, dbrb::View view)
			: BaseMessage(sender, ionet::PacketType::Dbrb_Deliver_Message, std::move(view))
			, PayloadHash(payloadHash)
		{}

	public:
		std::shared_ptr<MessagePacket> toNetworkPacket(const crypto::KeyPair* pKeyPair) override;

	public:
		/// Hash of the payload.
		Hash256 PayloadHash;
	};

	struct ConfirmDeliverMessage : BaseMessage {
	public:
		ConfirmDeliverMessage() = delete;
		explicit ConfirmDeliverMessage(const ProcessId& sender, const Hash256& payloadHash, dbrb::View view)
			: BaseMessage(sender, ionet::PacketType::Dbrb_Confirm_Deliver_Message, std::move(view))
			, PayloadHash(payloadHash)
		{}

	public:
		std::shared_ptr<MessagePacket> toNetworkPacket(const crypto::KeyPair* pKeyPair) override;

	public:
		/// Hash of the payload.
		Hash256 PayloadHash;
	};

	// Messages for sharded DBRB

	struct ShardPrepareMessage : Message {
	public:
		ShardPrepareMessage() = delete;
		ShardPrepareMessage(const ProcessId& sender, Payload payload, DbrbTreeView view, const catapult::Signature& broadcasterSignature)
			: Message(sender, ionet::PacketType::Dbrb_Shard_Prepare_Message)
			, Payload(std::move(payload))
			, View(std::move(view))
			, BroadcasterSignature(broadcasterSignature)
		{}

	public:
		std::shared_ptr<MessagePacket> toNetworkPacket(const crypto::KeyPair* pKeyPair) override;

	public:
		/// Message to be broadcasted.
		dbrb::Payload Payload;

		/// Current view of the system from the perspective of broadcaster.
		DbrbTreeView View;

		/// Payload and view signed by broadcaster.
		catapult::Signature BroadcasterSignature;
	};

	struct ShardBaseMessage : Message {
	public:
		ShardBaseMessage() = delete;
		explicit ShardBaseMessage(const ProcessId& sender, ionet::PacketType type, const Hash256& payloadHash, CertificateType certificate)
			: Message(sender, type)
			, PayloadHash(payloadHash)
			, Certificate(std::move(certificate))
		{}

	public:
		/// Hash of the payload.
		Hash256 PayloadHash;

		/// Message certificate for supplied payload.
		CertificateType Certificate;
	};

	struct ShardAcknowledgedMessage : ShardBaseMessage {
	public:
		ShardAcknowledgedMessage() = delete;
		explicit ShardAcknowledgedMessage(const ProcessId& sender, const Hash256& payloadHash, CertificateType certificate)
			: ShardBaseMessage(sender, ionet::PacketType::Dbrb_Shard_Acknowledged_Message, payloadHash, std::move(certificate))
		{}

	public:
		std::shared_ptr<MessagePacket> toNetworkPacket(const crypto::KeyPair* pKeyPair) override;
	};

	struct ShardCommitMessage : ShardBaseMessage {
	public:
		ShardCommitMessage() = delete;
		explicit ShardCommitMessage(const ProcessId& sender, const Hash256& payloadHash, CertificateType certificate)
			: ShardBaseMessage(sender, ionet::PacketType::Dbrb_Shard_Commit_Message, payloadHash, std::move(certificate))
		{}

	public:
		std::shared_ptr<MessagePacket> toNetworkPacket(const crypto::KeyPair* pKeyPair) override;
	};

	struct ShardDeliverMessage : ShardBaseMessage {
	public:
		ShardDeliverMessage() = delete;
		explicit ShardDeliverMessage(const ProcessId& sender, const Hash256& payloadHash, CertificateType certificate)
			: ShardBaseMessage(sender, ionet::PacketType::Dbrb_Shard_Deliver_Message, payloadHash, std::move(certificate))
		{}

	public:
		std::shared_ptr<MessagePacket> toNetworkPacket(const crypto::KeyPair* pKeyPair) override;
	};
}}