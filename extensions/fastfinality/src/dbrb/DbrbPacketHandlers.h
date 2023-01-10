/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/model/NetworkInfo.h"
#include <memory>

namespace catapult {
	namespace dbrb { class DbrbProcess; }
	namespace ionet { class ServerPacketHandlers; }
}

namespace catapult { namespace dbrb {

	/// Registers a push DBRB node handler in \a handlers.
	void RegisterPushNodesHandler(
		const std::weak_ptr<DbrbProcess>& pDbrbProcessWeak,
		model::NetworkIdentifier networkIdentifier,
		ionet::ServerPacketHandlers& handlers);

	/// Registers a pull DBRB nodes handler in \a handlers constructing response from \a pNodeRetreiverWeak.
	void RegisterPullNodesHandler(
		const std::weak_ptr<DbrbProcess>& pDbrbProcessWeak,
		ionet::ServerPacketHandlers& handlers);
}}