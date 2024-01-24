/**
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
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

		/// Quorum manager.
		dbrb::QuorumManager QuorumManager;

		/// Whether any commit message was received.
		bool CommitMessageReceived = false;

		/// Time when corresponding Prepare message was received.
		Timestamp Begin;
	};
}}