/**
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "DbrbTree.h"
#include "QuorumManager.h"

namespace catapult { namespace dbrb {

	/// Struct representing data about Payload that is being broadcasted.
	struct BroadcastData {
		/// Payload that is being broadcasted.
		dbrb::Payload Payload;

		/// Map that maps views and process IDs to signatures received from respective Acknowledged messages.
		std::map<std::pair<View, ProcessId>, Signature> Signatures;

		/// Map that maps process IDs to signatures received from them.
		/// Filled when Acknowledged quorum is collected.
		CertificateType Certificate;

		/// View associated with the broadcast operation.
		View BroadcastView;

		/// View consisting of bootstrap processes associated with the broadcast operation.
		View BootstrapView;

		/// Quorum manager.
		dbrb::QuorumManager QuorumManager;

		/// Whether any commit message was received.
		bool CommitMessageReceived = false;

		/// Time when corresponding Prepare message was sent or first received.
		Timestamp Begin;
	};

	struct DeliverCertificate {
		bool Requested;
		bool QuorumCollected;
		CertificateType Certificate;
	};

	/// Struct representing data about Payload that is being broadcasted.
	struct ShardBroadcastData {
		/// Payload that is being broadcasted.
		dbrb::Payload Payload;

		/// Acknowledge certificate collected in sub tree where this node is root.
		CertificateType AcknowledgeCertificate;

		/// Deliver certificate collected in the entire tree not including the sub tree where this node is root.
		CertificateType ParentShardDeliverCertificate;

		/// Children that requested deliver certificate by sending commit message.
		std::map<ProcessId, DeliverCertificate> ParentShardDeliverCertificateRecipients;

		/// Deliver certificate collected in sub tree where this node is root.
		CertificateType ChildShardDeliverCertificate;

		/// Parent and siblings requested deliver certificate by sending commit message.
		std::map<ProcessId, DeliverCertificate> ChildShardDeliverCertificateRecipients;

		/// Whether any commit message has been sent.
		bool CommitMessageSent = false;

		/// Whether the payload has been acknowledged.
		bool Acknowledged = false;

		/// Whether the payload has been delivered.
		bool Delivered = false;

		/// Time when corresponding Prepare message was sent or first received.
		Timestamp Begin;

		/// The process that initiated the broadcast operation.
		ProcessId Broadcaster;

		/// View associated with the broadcast operation.
		View BroadcastView;

		/// Tree built from the view associated with the broadcast operation.
		DbrbTreeView Tree;

		/// The sub tree where this node is root.
		View SubTreeView;

		/// The double shard which the current process is part of.
		DbrbDoubleShard Shard;

		/// The quorum size of the entire network.
		size_t NetworkQuorumSize;

		/// The quorum size of the parent shard.
		size_t ParentShardQuorumSize;

		/// The quorum size of the child shard.
		size_t ChildShardQuorumSize;
	};
}}