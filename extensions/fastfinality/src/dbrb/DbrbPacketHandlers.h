/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/model/NetworkInfo.h"
#include <memory>

namespace catapult {
	namespace dbrb {
		class DbrbProcess;
		class ShardedDbrbProcess;
	}
	namespace ionet { class ServerPacketHandlers; }
	namespace crypto { class KeyPair; }
	namespace config { class BlockchainConfigurationHolder; }
}

namespace catapult { namespace dbrb {

	/// Registers a push DBRB node handler in \a handlers.
	void RegisterPushNodesHandler(
		const std::weak_ptr<DbrbProcess>& pDbrbProcessWeak,
		model::NetworkIdentifier networkIdentifier,
		const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder,
		ionet::ServerPacketHandlers& handlers);

	/// Registers a push DBRB node handler in \a handlers.
	void RegisterPushNodesHandler(
		const std::weak_ptr<ShardedDbrbProcess>& pDbrbProcessWeak,
		model::NetworkIdentifier networkIdentifier,
		const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder,
		ionet::ServerPacketHandlers& handlers);

	/// Registers a remove DBRB node request handler in \a handlers.
	void RegisterRemoveNodeRequestHandler(
		const std::weak_ptr<DbrbProcess>& pDbrbProcessWeak,
		const crypto::KeyPair& keyPair,
		ionet::ServerPacketHandlers& handlers);

	/// Registers a remove DBRB node request handler in \a handlers.
	void RegisterRemoveNodeRequestHandler(
		const std::weak_ptr<ShardedDbrbProcess>& pDbrbProcessWeak,
		const crypto::KeyPair& keyPair,
		ionet::ServerPacketHandlers& handlers);

	/// Registers a remove DBRB node response handler in \a handlers.
	void RegisterRemoveNodeResponseHandler(
		const std::weak_ptr<DbrbProcess>& pDbrbProcessWeak,
		ionet::ServerPacketHandlers& handlers);

	/// Registers a remove DBRB node response handler in \a handlers.
	void RegisterRemoveNodeResponseHandler(
		const std::weak_ptr<ShardedDbrbProcess>& pDbrbProcessWeak,
		ionet::ServerPacketHandlers& handlers);
}}