/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include <memory>

namespace catapult {
	namespace storage { class ReplicatorService; }
	namespace ionet { class ServerPacketHandlers; }
}

namespace catapult { namespace storage {

	/// Registers a start download files handler in \a handlers requesting start file download from \a pReplicatorServiceWeak.
	void RegisterStartDownloadFilesHandler(const std::weak_ptr<ReplicatorService>& pReplicatorServiceWeak, ionet::ServerPacketHandlers& handlers);

	/// Registers a stop download files handler in \a handlers requesting stop file download from \a pReplicatorServiceWeak.
	void RegisterStopDownloadFilesHandler(const std::weak_ptr<ReplicatorService>& pReplicatorServiceWeak, ionet::ServerPacketHandlers& handlers);
}}