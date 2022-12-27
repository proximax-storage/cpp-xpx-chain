/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include <utility>

#include "DbrbUtils.h"

namespace catapult { namespace crypto { class KeyPair; }}

namespace catapult { namespace dbrb {

#pragma pack(push, 1)

	struct MessagePacket : public ionet::Packet {
		catapult::Signature Signature;

		/// Returns a non-const pointer to data contained in this packet.
		uint8_t* payload() {
			return Size <= sizeof(Packet) ? nullptr : reinterpret_cast<uint8_t*>(this) + sizeof(Packet) + Signature_Size;
		}

		/// Returns a const pointer to data contained in this packet.
		constexpr const uint8_t* payload() const {
			return Size <= sizeof(Packet) ? nullptr : reinterpret_cast<const uint8_t*>(this) + sizeof(Packet) + Signature_Size;
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

		// TODO: Include fields to enable signature/certificate verification
		catapult::Signature Signature;
	};

	class NetworkPacketConverter {
	public:
		using PacketConverter = std::function<std::unique_ptr<Message> (const ionet::Packet& packet)>;

	public:
		NetworkPacketConverter();

	public:
		/// Registers a \a converter for the specified packet \a type.
		void registerConverter(ionet::PacketType type, const PacketConverter& converter);

		/// Converts \a packet to a message or throws if converter of packet type is not registered.
		std::unique_ptr<Message> toMessage(const ionet::Packet& packet) const;

	private:
		const PacketConverter* findConverter(const ionet::Packet& packet) const;

	private:
		std::vector<PacketConverter> m_converters;
	};

	// Messages related to JOIN and LEAVE operations

	struct ReconfigMessage : Message {
	public:
		ReconfigMessage() = delete;
		ReconfigMessage(const ProcessId& sender, ProcessId processId, const MembershipChanges& membershipChange, View view)
			: Message(sender, ionet::PacketType::Dbrb_Reconfig_Message)
			, ProcessId(std::move(processId))
			, MembershipChange(membershipChange)
			, View(std::move(view))
		{}

	public:
		std::shared_ptr<MessagePacket> toNetworkPacket(const crypto::KeyPair* pKeyPair) override;

	public:
		/// Process ID that is a part of change.
		dbrb::ProcessId ProcessId; // TODO: Check if necessary (perhaps Sender is enough?)

		/// Type of ProcessId's membership change.
		MembershipChanges MembershipChange;

		/// Current view of the system from the perspective of ProcessID.
		/// It shouldn't include the change from this message.
		dbrb::View View;
	};

	struct ReconfigConfirmMessage : Message {
	public:
		ReconfigConfirmMessage() = delete;
		ReconfigConfirmMessage(const ProcessId& sender, View view)
			: Message(sender, ionet::PacketType::Dbrb_Reconfig_Confirm_Message)
			, View(std::move(view))
		{}

	public:
		std::shared_ptr<MessagePacket> toNetworkPacket(const crypto::KeyPair* pKeyPair) override;

	public:
		/// View that receives the confirmation.
		dbrb::View View;
	};

	struct ProposeMessage : Message {
	public:
		ProposeMessage() = delete;
		ProposeMessage(const ProcessId& sender, Sequence proposedSequence, View replacedView)
			: Message(sender, ionet::PacketType::Dbrb_Propose_Message)
			, ProposedSequence(std::move(proposedSequence))
			, ReplacedView(std::move(replacedView))
		{}

	public:
		std::shared_ptr<MessagePacket> toNetworkPacket(const crypto::KeyPair* pKeyPair) override;

	public:
		/// Proposed sequence to replace the view.
		Sequence ProposedSequence;

		/// View to be replaced with the proposed sequence.
		View ReplacedView;
	};

	struct ConvergedMessage : Message {
	public:
		ConvergedMessage() = delete;
		ConvergedMessage(const ProcessId& sender, Sequence convergedSequence, View replacedView)
			: Message(sender, ionet::PacketType::Dbrb_Converged_Message)
			, ConvergedSequence(std::move(convergedSequence))
			, ReplacedView(std::move(replacedView))
		{}

	public:
		std::shared_ptr<MessagePacket> toNetworkPacket(const crypto::KeyPair* pKeyPair) override;

	public:
		/// Sequence that is converged on to replace the view.
		Sequence ConvergedSequence;

		/// View that is replaced with the converged sequence.
		// TODO: This can be prepended to ConvergedSequence, similarly to how it's done in InstallMessage
		View ReplacedView;
	};

	struct InstallMessage : Message {
	public:
		InstallMessage() = delete;
		InstallMessage(const ProcessId& sender, Sequence sequence, std::map<ProcessId, catapult::Signature> signatures)
			: Message(sender, ionet::PacketType::Dbrb_Install_Message)
			, Sequence(std::move(sequence))
			, ConvergedSignatures(std::move(signatures))
		{}

	public:
		std::shared_ptr<MessagePacket> toNetworkPacket(const crypto::KeyPair* pKeyPair) override;
		std::optional<InstallMessageData> tryGetMessageData() const;

	public:
		/// Sequence that is converged on to replace the view, with a single replaced view prepended to it.
		/// Must contain at least 2 elements to be valid.
		dbrb::Sequence Sequence;

		/// Map of processes and their signatures for appropriate Converged messages for Sequence.
		std::map<ProcessId, catapult::Signature> ConvergedSignatures;
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
		explicit AcknowledgedMessage(const ProcessId& sender, ProcessId  initiator, Payload payload, View view, catapult::Signature payloadSignature)
			: Message(sender, ionet::PacketType::Dbrb_Acknowledged_Message)
			, Initiator(std::move(initiator))
			, Payload(std::move(payload))
			, View(std::move(view))
			, PayloadSignature(std::move(payloadSignature))
		{}

	public:
		std::shared_ptr<MessagePacket> toNetworkPacket(const crypto::KeyPair* pKeyPair) override;

	public:
		/// Broadcast initiator.
		ProcessId Initiator;

		/// Acknowledged payload.
		dbrb::Payload Payload;

		/// Current view of the system from the perspective of Sender.
		dbrb::View View;

		/// Signature formed by Sender.
		catapult::Signature PayloadSignature;
	};

	struct CommitMessage : Message {
	public:
		CommitMessage() = delete;
		explicit CommitMessage(const ProcessId& sender, ProcessId  initiator, Payload payload, std::map<ProcessId, catapult::Signature> certificate, View certificateView, View currentView)
			: Message(sender, ionet::PacketType::Dbrb_Commit_Message)
			, Initiator(std::move(initiator))
			, Payload(std::move(payload))
			, Certificate(std::move(certificate))
			, CertificateView(std::move(certificateView))
			, CurrentView(std::move(currentView))
		{}

	public:
		std::shared_ptr<MessagePacket> toNetworkPacket(const crypto::KeyPair* pKeyPair) override;

	public:
		/// Broadcast initiator.
		ProcessId Initiator;

		/// Payload for which Acknowledged quorum was collected.
		dbrb::Payload Payload;

		/// Message certificate for supplied payload.
		std::map<ProcessId, catapult::Signature> Certificate;

		/// View associated with supplied certificate.
		View CertificateView;

		/// Current view of the system from the perspective of Sender.
		View CurrentView;
	};

	struct ProcessState {
		/// Prepare message with a payload that is allowed to be acknowledged, if any.
		std::optional<PrepareMessage> Acknowledgeable;

		/// A conflicting Prepare message, if any.
		std::optional<PrepareMessage> Conflicting;

		/// Stored Commit message, if any.
		std::optional<CommitMessage> Stored;
	};

	struct StateUpdateMessage : Message {
	public:
		StateUpdateMessage() = delete;
		StateUpdateMessage(const ProcessId& sender, ProcessState state, View view, View pendingChanges)
				: Message(sender, ionet::PacketType::Dbrb_State_Update_Message)
				, State(std::move(state))
				, View(std::move(view))
				, PendingChanges(std::move(pendingChanges))
		{}

	public:
		std::shared_ptr<MessagePacket> toNetworkPacket(const crypto::KeyPair* pKeyPair) override;

	public:
		/// State of the process.
		ProcessState State;

		/// View associated with the supplied state.
		dbrb::View View;

		/// List of pending changes (i.e., join or leave).
		dbrb::View PendingChanges;
	};

	struct DeliverMessage : Message {
	public:
		DeliverMessage() = delete;
		explicit DeliverMessage(const ProcessId& sender, ProcessId  initiator, Payload payload, View view)
			: Message(sender, ionet::PacketType::Dbrb_Deliver_Message)
			, Initiator(std::move(initiator))
			, Payload(std::move(payload))
			, View(std::move(view))
		{}

	public:
		std::shared_ptr<MessagePacket> toNetworkPacket(const crypto::KeyPair* pKeyPair) override;

	public:
		/// Broadcast initiator.
		ProcessId Initiator;

		/// Payload to deliver.
		dbrb::Payload Payload;

		/// Current view of the system from the perspective of Sender.
		dbrb::View View;
	};
}}