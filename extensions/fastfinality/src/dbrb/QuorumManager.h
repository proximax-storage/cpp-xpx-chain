/**
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/dbrb/Messages.h"

namespace catapult { namespace dbrb {

	/// Struct that encapsulates all necessary quorum counters and their update methods.
	struct QuorumManager {
		/// Maps views to sets of pairs of respective process IDs and payload hashes received from Acknowledged messages.
		std::map<View, std::set<std::pair<ProcessId, Hash256>>> AcknowledgedPayloads;

		/// Maps views to sets of process IDs ready for delivery.
		std::map<View, std::set<ProcessId>> DeliveredProcesses;

		/// Overloaded methods for updating respective counters.
		/// Returns whether the quorum has just been collected on this update.
		bool update(const AcknowledgedMessage&);
		bool update(const DeliverMessage&);
	};
}}