/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "DbrbUtils.h"

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

	/// Base class for all messages sent via DBRB protocol.
	struct Message {
	public:
		Message() = delete;
		Message(ProcessId sender, ionet::PacketType type): Sender(std::move(sender)), Type(type) {}
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

	// Messages related to BROADCAST operation

	struct PrepareMessage : Message {
	public:
		PrepareMessage() = delete;
		PrepareMessage(const ProcessId& sender, Payload payload, View view)
			: Message(sender, ionet::PacketType::Dbrb_Prepare_Message)
			, Payload(std::move(payload))
			, View(std::move(view))
		{}

	public:
		std::shared_ptr<MessagePacket> toNetworkPacket(const crypto::KeyPair* pKeyPair) override;

	public:
		/// Message to be broadcasted.
		dbrb::Payload Payload;

		/// Current view of the system from the perspective of Sender.
		dbrb::View View;
	};

	struct AcknowledgedMessage : Message {
	public:
		AcknowledgedMessage() = delete;
		explicit AcknowledgedMessage(ProcessId  sender, const Hash256& payloadHash, View view, catapult::Signature payloadSignature)
			: Message(sender, ionet::PacketType::Dbrb_Acknowledged_Message)
			, PayloadHash(payloadHash)
			, View(std::move(view))
			, PayloadSignature(std::move(payloadSignature))
		{}

	public:
		std::shared_ptr<MessagePacket> toNetworkPacket(const crypto::KeyPair* pKeyPair) override;

	public:
		/// Hash of the payload.
		Hash256 PayloadHash;

		/// Current view of the system from the perspective of Sender.
		dbrb::View View;

		/// Signature formed by Sender.
		catapult::Signature PayloadSignature;
	};

	struct CommitMessage : Message {
	public:
		CommitMessage() = delete;
		explicit CommitMessage(const ProcessId& sender, const Hash256& payloadHash, CertificateType certificate, View certificateView, View currentView)
			: Message(sender, ionet::PacketType::Dbrb_Commit_Message)
			, PayloadHash(payloadHash)
			, Certificate(std::move(certificate))
			, CertificateView(std::move(certificateView))
			, CurrentView(std::move(currentView))
		{}

	public:
		std::shared_ptr<MessagePacket> toNetworkPacket(const crypto::KeyPair* pKeyPair) override;

	public:
		/// Hash of the payload.
		Hash256 PayloadHash;

		/// Message certificate for supplied payload.
		CertificateType Certificate;

		/// View associated with supplied certificate.
		View CertificateView;

		/// Current view of the system from the perspective of Sender.
		View CurrentView;
	};

	struct DeliverMessage : Message {
	public:
		DeliverMessage() = delete;
		explicit DeliverMessage(const ProcessId& sender, const Hash256& payloadHash, View view)
			: Message(sender, ionet::PacketType::Dbrb_Deliver_Message)
			, PayloadHash(payloadHash)
			, View(std::move(view))
		{}

	public:
		std::shared_ptr<MessagePacket> toNetworkPacket(const crypto::KeyPair* pKeyPair) override;

	public:
		/// Hash of the payload.
		Hash256 PayloadHash;

		/// Current view of the system from the perspective of Sender.
		dbrb::View View;
	};
}}