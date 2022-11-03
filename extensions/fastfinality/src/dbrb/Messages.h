#pragma once
#include "DbrbUtils.h"


namespace catapult { namespace fastfinality {

	/// Base class for all messages send via DBRB protocol.
	struct Message {
	public:
		virtual std::shared_ptr<const ionet::Packet> packet() const = 0;

	public:
		/// Sender of the message.
		ProcessId Sender;

		// TODO: Include fields to enable signature/certificate verification
	};

	// Messages related to JOIN and LEAVE operations

	struct ReconfigMessage : Message {
		/// Process ID that is a part of change.
		fastfinality::ProcessId ProcessId; // TODO: Check if necessary (perhaps Sender is enough?)

		/// Type of ProcessId's membership change.
		MembershipChanges MembershipChange;

		/// Current view of the system from the perspective of ProcessID.
		/// It shouldn't include the change from this message.
		fastfinality::View View;

		std::shared_ptr<const ionet::Packet> packet() const override {
			return std::make_shared<ionet::Packet>();
		}
	};

	struct ReconfigConfirmMessage : Message {
		/// View that receives the confirmation.
		fastfinality::View View;

		std::shared_ptr<const ionet::Packet> packet() const override {
			return std::make_shared<ionet::Packet>();
		}
	};

	struct ProposeMessage : Message {
		/// Proposed sequence to replace the view.
		Sequence ProposedSequence;

		/// View to be replaced with the proposed sequence.
		View ReplacedView;

		std::shared_ptr<const ionet::Packet> packet() const override {
			return std::make_shared<ionet::Packet>();
		}
	};

	struct ConvergedMessage : Message {
		/// Sequence that is converged on to replace the view.
		Sequence ConvergedSequence;

		/// View that is replaced with the converged sequence.
		View ReplacedView;
	};

	struct InstallMessage : Message {
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
		fastfinality::Payload Payload;
		fastfinality::View View;
	};

	struct StateUpdateMessage : Message {
		/// State of the process. Consists of Acknowledged, Conflicting and Stored fields.
		ProcessState State;

		/// List of pending changes (i.e., join or leave).
		std::vector<std::pair<ProcessId, MembershipChanges>> PendingChanges;
	};

	struct AcknowledgedMessage : Message {};

	struct CommitMessage : Message {};

	struct DeliverMessage : Message {};
}}