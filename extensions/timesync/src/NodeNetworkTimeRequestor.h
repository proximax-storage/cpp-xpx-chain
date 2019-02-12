/**
*** Copyright (c) 2016-present,
*** Jaguar0625, gimre, BloodyRookie, Tech Bureau, Corp. All rights reserved.
***
*** This file is part of Catapult.
***
*** Catapult is free software: you can redistribute it and/or modify
*** it under the terms of the GNU Lesser General Public License as published by
*** the Free Software Foundation, either version 3 of the License, or
*** (at your option) any later version.
***
*** Catapult is distributed in the hope that it will be useful,
*** but WITHOUT ANY WARRANTY; without even the implied warranty of
*** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*** GNU Lesser General Public License for more details.
***
*** You should have received a copy of the GNU Lesser General Public License
*** along with Catapult. If not, see <http://www.gnu.org/licenses/>.
**/

#pragma once
#include "CommunicationTimestamps.h"
#include "timesync/src/api/RemoteTimeSyncApi.h"
#include "catapult/net/BriefServerRequestor.h"

namespace catapult { namespace timesync {

	/// Node network time request policy.
	class NodeNetworkTimeRequestPolicy {
	public:
		using ResponseType = CommunicationTimestamps;

	public:
		static constexpr const char* FriendlyName() {
			return "network time";
		}

		static thread::future<ResponseType> CreateFuture(ionet::PacketIo& packetIo) {
			return api::CreateRemoteTimeSyncApi(packetIo)->networkTime();
		}
	};

	/// A brief server requestor for requesting node network time information.
	using NodeNetworkTimeRequestor = net::BriefServerRequestor<NodeNetworkTimeRequestPolicy>;

	/// Creates a node network time requestor for a server with a key pair of \a keyPair using \a pPool and configured with \a settings.
	std::shared_ptr<NodeNetworkTimeRequestor> CreateNodeNetworkTimeRequestor(
			const std::shared_ptr<thread::IoServiceThreadPool>& pPool,
			const crypto::KeyPair& keyPair,
			const net::ConnectionSettings& settings);
}}
