#pragma once
#include "DbrbUtils.h"


namespace catapult { namespace fastfinality {

		/// Base class for all messages send via DBRB protocol.
		struct Message {
			/// Sender of the message.
			ProcessId Sender;

			// TODO: Include fields to enable signature/certificate verification
		};


		// Messages related to JOIN and LEAVE operations

		struct ReconfigMessage : Message {
			/// Process ID that is a part of change.
			ProcessId ProcessId; // TODO: Check if necessary (perhaps Sender is enough?)

			/// Type of ProcessId's membership change.
			MembershipChanges MembershipChange;

			/// Current view of the system from the perspective of ProcessID.
			/// It shouldn't include the change from this message.
			View View;
		};

		struct ReconfigConfirmMessage : Message {
			/// View that receives the confirmation.
			View View;
		};

		struct ProposeMessage : Message {
			/// Proposed sequence to replace the view.
			Sequence ProposedSequence;

			/// View to be replaced with the proposed sequence.
			View ReplacedView;
		};

		struct ConvergedMessage : Message {
			/// Sequence that is converged on to replace the view.
			Sequence ConvergedSequence;

			/// View that is replaced with the converged sequence.
			View ReplacedView;
		};

		struct InstallMessage : Message {
			/// Least recent view.
			View LeastRecentView;

			/// Sequence to be installed.
			Sequence InstalledSequence;

			/// View to be installed.
			View InstalledView;

			// TODO: Include a quorum of signed Converged messages which ensures its authenticity
		};


		// Messages related to BROADCAST operation

		struct PrepareMessage : Message {
			Payload Payload;
			View View;
		};

		struct StateUpdateMessage : Message {};

		struct AcknowledgedMessage : Message {};

		struct CommitMessage : Message {};

		struct DeliverMessage : Message {};

}}