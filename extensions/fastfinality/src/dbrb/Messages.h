#pragma once
#include <utility>

#include "DbrbUtils.h"


namespace catapult { namespace dbrb {

	/// Base class for all messages send via DBRB protocol.
	struct Message {
	public:
		Message() = delete;
		Message(const ProcessId& sender, ionet::PacketType type): Sender(sender), Type(type) {}
		virtual ~Message() = default;

	public:
		/// Creates a network packet representing this message.
		virtual std::shared_ptr<ionet::Packet> toNetworkPacket() const = 0;

		/// Compares this node with \a rhs.
		bool operator<(const Message& rhs) const {
			return Sender < rhs.Sender;
		}

	public:
		/// Sender of the message.
		ProcessId Sender;

		/// Type of the packet.
		ionet::PacketType Type;

		// TODO: Include fields to enable signature/certificate verification
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
		ReconfigMessage(const ProcessId& sender, const ProcessId& processId, const MembershipChanges& membershipChange, View view)
			: Message(sender, ionet::PacketType::Dbrb_Reconfig_Message)
			, ProcessId(processId)
			, MembershipChange(membershipChange)
			, View(std::move(view))
		{}

	public:
		std::shared_ptr<ionet::Packet> toNetworkPacket() const override;

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
		std::shared_ptr<ionet::Packet> toNetworkPacket() const override;

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
		std::shared_ptr<ionet::Packet> toNetworkPacket() const override;

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
		std::shared_ptr<ionet::Packet> toNetworkPacket() const override;

	public:
		/// Sequence that is converged on to replace the view.
		Sequence ConvergedSequence;

		/// View that is replaced with the converged sequence.
		View ReplacedView;
	};

	struct InstallMessage : Message {
	public:
		InstallMessage() = delete;
		InstallMessage(const ProcessId& sender, View leastRecentView, Sequence convergedSequence, View replacedView)
			: Message(sender, ionet::PacketType::Dbrb_Install_Message)
			, LeastRecentView(std::move(leastRecentView))
			, ConvergedSequence(std::move(convergedSequence))
			, ReplacedView(std::move(replacedView))
		{}

	public:
		std::shared_ptr<ionet::Packet> toNetworkPacket() const override;

	public:
		/// Least recent view.
		View LeastRecentView;	// TODO: Unnecessary, it's just *InstalledSequence.maybeLeastRecent()

		/// Sequence that is converged on to replace the view.
		Sequence ConvergedSequence;

		/// View to be replaced.
		View ReplacedView;

		// TODO: Include a quorum of signed Converged messages which ensures its authenticity
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
		std::shared_ptr<ionet::Packet> toNetworkPacket() const override;

	public:
		/// Message to be broadcasted.
		dbrb::Payload Payload;

		/// Current view of the system from the perspective of Sender.
		dbrb::View View;
	};

	struct StateUpdateMessage : Message {
	public:
		StateUpdateMessage() = delete;
		StateUpdateMessage(const ProcessId& sender, ProcessState state, View pendingChanges)
			: Message(sender, ionet::PacketType::Dbrb_State_Update_Message)
			, State(std::move(state))
			, PendingChanges(std::move(pendingChanges))
		{}

	public:
		std::shared_ptr<ionet::Packet> toNetworkPacket() const override;

	public:
		/// State of the process.
		ProcessState State;

		/// List of pending changes (i.e., join or leave).
		View PendingChanges;
	};

	struct AcknowledgedMessage : Message {
	public:
		AcknowledgedMessage() = delete;
		explicit AcknowledgedMessage(const ProcessId& sender, Payload payload, View view, Signature signature)
			: Message(sender, ionet::PacketType::Dbrb_Acknowledged_Message)
			, Payload(std::move(payload))
			, View(std::move(view))
			, Signature(std::move(signature))
		{}

	public:
		std::shared_ptr<ionet::Packet> toNetworkPacket() const override;

	public:
		/// Acknowledged payload.
		dbrb::Payload Payload;

		/// Current view of the system from the perspective of Sender.
		dbrb::View View;

		/// Signature formed by Sender.
		catapult::Signature Signature;
	};

	struct CommitMessage : Message {
	public:
		CommitMessage() = delete;
		explicit CommitMessage(const ProcessId& sender, Payload payload, std::set<Signature> certificate, View certificateView, View currentView)
			: Message(sender, ionet::PacketType::Dbrb_Commit_Message)
			, Payload(std::move(payload))
			, Certificate(std::move(certificate))
			, CertificateView(std::move(certificateView))
			, CurrentView(std::move(currentView))
		{}

	public:
		std::shared_ptr<ionet::Packet> toNetworkPacket() const override;

	public:
		/// Payload for which Acknowledged quorum was collected.
		dbrb::Payload Payload;

		/// Message certificate for supplied payload.
		std::map<ProcessId, Signature> Certificate;

		/// View associated with supplied certificate.
		View CertificateView;

		/// Current view of the system from the perspective of Sender.
		View CurrentView;
	};

	struct DeliverMessage : Message {
	public:
		DeliverMessage() = delete;
		explicit DeliverMessage(const ProcessId& sender, Payload payload, View view)
			: Message(sender, ionet::PacketType::Dbrb_Deliver_Message)
			, Payload(std::move(payload))
			, View(std::move(view))
		{}

	public:
		std::shared_ptr<ionet::Packet> toNetworkPacket() const override;

	public:
		/// Payload to deliver.
		dbrb::Payload Payload;

		/// Current view of the system from the perspective of Sender.
		dbrb::View View;
	};
}}