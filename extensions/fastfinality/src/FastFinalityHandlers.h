/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "FastFinalityChainPackets.h"
#include "catapult/extensions/ServerHooks.h"
#include "catapult/plugins/PluginManager.h"
#include "catapult/thread/Future.h"
#include "catapult/types.h"
#include <functional>
#include <memory>

namespace catapult { namespace fastfinality {
	class FastFinalityFsm;
}}

namespace catapult { namespace fastfinality {

	/// Validates a push block message.
	bool ValidateBlock(
		FastFinalityFsm& fsm,
		const ionet::Packet& packet,
		extensions::ServiceState& state,
		const model::BlockElementSupplier& lastBlockElementSupplier,
		const std::shared_ptr<thread::IoThreadPool>& pValidatorPool);

	/// Handles a push block message.
	void PushBlock(
		FastFinalityFsm& fsm,
		const plugins::PluginManager& pluginManager,
		const ionet::Packet& packet);

	/// Registers a pull remote node state handler in \a handlers constructing response from \a pFsmWeak
	/// using \a pConfigHolder, \a blockElementGetter and \a lastBlockElementSupplier.
	void RegisterPullRemoteNodeStateHandler(
		const std::weak_ptr<FastFinalityFsm>& pFsmWeak,
		ionet::ServerPacketHandlers& handlers,
		const Key& bootPublicKey,
		const std::function<std::shared_ptr<const model::BlockElement> (const Height&)>& blockElementGetter,
		const model::BlockElementSupplier& lastBlockElementSupplier);
}}