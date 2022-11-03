/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma  once
#include "DbrbUtils.h"
#include "Messages.h"
#include "catapult/functions.h"
#include "catapult/net/PacketWriters.h"

namespace catapult { namespace fastfinality {

		/// Struct with all necessary fields needed for joining procedure.
		struct JoinState {
			/// Whether the view discovery is active. View discovery halts once a quorum of
			/// confirmation messages has been collected for any of the views.
			bool ViewDiscoveryActive = false;

			/// Maps views to numbers of received confirmations for those views.
			std::map<View, unsigned> ViewConfirmations;

			/// Increments (or initializes) a counter of received confirms for a given \c view.
			/// Returns whether the view discovery is active after the counter is updated.
			bool addConfirm(View view) {
//				if (std::find(ViewConfirmations.begin(), ViewConfirmations.end(), view) == ViewConfirmations.end())
//					ViewConfirmations.emplace(view, 1u);
//				else
//					ViewConfirmations.at(view) += 1u;

				if (ViewConfirmations.at(view) >= view.quorumSize())
					ViewDiscoveryActive = false;

				return ViewDiscoveryActive;
			};
		};


		/// Class representing DBRB process.
		class DbrbProcess {
		private:
			/// Process identifier.
			ProcessId m_id;

			/// Information about joining procedure.
			JoinState m_joinState;

			/// Most recent view known to the process.
			View m_currentView;

			/// List of pending changes (i.e., join or leave).
			std::vector<std::pair<ProcessId, MembershipChanges>> m_pendingChanges;

			/// Map that maps views to proposed sequences to replace those views.
			std::map<View, Sequence> m_proposedSequences;

			/// Map that maps views to the last converged sequence to replace those views.
			std::map<View, Sequence> m_lastConvergedSequences;

			/// Map that maps views to the sets of sequences that can be proposed.
			std::map<View, std::set<Sequence>> m_formatSequences;

			/// Message certificate for current payload.
			Certificate m_certificate;

			/// View in which message certificate was collected.
			View m_certificateView;

			/// List of payloads allowed to be acknowledged. If empty, any payload can be acknowledged.
			std::vector<Payload> m_acknowledgeAllowed;

			/// Value of the payload.
			Payload m_payload;

			/// Whether value of the payload is relevant.
			bool m_payloadIsStored;

			/// Whether value of the payload is delivered.
			bool m_payloadIsDelivered;

			/// Whether process is allowed to leave the system.
			bool m_canLeave;

			std::shared_ptr<net::PacketWriters> m_pWriters;

		public:
			/// Request to join the system.
			void join();

			/// Request to leave the system.
			void leave();

			/// Broadcast arbitrary \c payload into the system.
			void broadcast(const Message&);

			void processMessage(const ionet::Packet&);

		private:
			using DisseminateCallback = consumer<ionet::SocketOperationCode, const ionet::Packet*>;

			void disseminate(const Message& message, const DisseminateCallback& callback);
			void send(const Message& message, const ionet::Node& recipient, const ionet::PacketIo::WriteCallback& callback);

			// Request members of the current view forcibly remove failing participant.
			void forceLeave(const ionet::Node& node);

			void onReconfigMessageReceived(const ReconfigMessage&, const ionet::Node& sender);
			void onReconfigConfirmMessageReceived(const ReconfigConfirmMessage&);
			void onProposeMessageReceived(const ProposeMessage&);
			void onConvergedMessageReceived(const ConvergedMessage&);
			void onInstallMessageReceived(const InstallMessage&);

			void onPrepareMessageReceived(const PrepareMessage& message);
			void onStateUpdateMessageReceived(const StateUpdateMessage&);
			void onAcknowledgedMessageReceived(const AcknowledgedMessage&);
			void onCommitMessageReceived(const CommitMessage&);
			void onDeliverMessageReceived(const DeliverMessage&);

			/// Called when a new (most recent) view of the system is discovered.
			void onViewDiscovered(View&&);

			void onJoinComplete();
			void onLeaveComplete();
		};

}}