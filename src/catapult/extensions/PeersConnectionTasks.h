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
#include "NodeSelector.h"
#include "catapult/config/NodeConfiguration.h"
#include "catapult/ionet/NodeContainer.h"
#include "catapult/thread/Task.h"
#include <vector>

namespace catapult {
	namespace cache { class CatapultCache; }
	namespace extensions { class ServiceState; }
	namespace net {
		class ConnectionContainer;
		class PacketWriters;
	}
}

namespace catapult { namespace extensions {

	// region NodeAger

	// A node ager.
	using NodeAger = consumer<const utils::KeySet&>;

	/// Creates and prepares a node ager that ages all \a serviceId connections in \a nodes given \a config.
	NodeAger CreateNodeAger(
			ionet::ServiceIdentifier serviceId,
			const config::NodeConfiguration::ConnectionsSubConfiguration& config,
			ionet::NodeContainer& nodes);

	// endregion

	// region SelectorSettings

	/// Settings used to initialize a selector task.
	struct SelectorSettings {
	public:
		/// Creates settings around \a state, \a serviceId and \a requiredRole.
		SelectorSettings(
				extensions::ServiceState& state,
				ionet::ServiceIdentifier serviceId,
				ionet::NodeRoles requiredRole);

		/// Creates settings around \a state and \a serviceId.
		SelectorSettings(
				extensions::ServiceState& state,
				ionet::ServiceIdentifier serviceId);

	public:
		/// Container of nodes from which to select.
		ionet::NodeContainer& Nodes;

		/// Service identifier for selection.
		ionet::ServiceIdentifier ServiceId;

		/// Required role for selection (if applicable).
		ionet::NodeRoles RequiredRole;

		/// Connections configuration.
		config::NodeConfiguration::ConnectionsSubConfiguration Config;

		/// Retrieves an account importance given a public key.
		extensions::ImportanceRetriever ImportanceRetriever;
	};

	// endregion

	// region NodeSelector / ConnectPeersTask

	/// A node selector.
	using NodeSelector = supplier<NodeSelectionResult>;

	/// Creates and prepares a node selector given \a settings.
	/// \note All nodes are provisioned for specified service id.
	/// \note The selector is intended to be used in conjunction with CreateConnectPeersTask for managing outgoing connections.
	NodeSelector CreateNodeSelector(const SelectorSettings& settings);

	/// Creates a task for the specified service that connects to nodes with the specified role given \a settings and \a packetWriters.
	thread::Task CreateConnectPeersTask(const SelectorSettings& settings, net::PacketWriters& packetWriters);

	/// Creates a task for the specified service that connects to nodes given \a settings, \a packetWriters and \a selector.
	/// \note \a selector returns add candidates (subset of compatible nodes in \a nodes)
	///        and remove candidates (subset of active connections in \a packetWriters).
	thread::Task CreateConnectPeersTask(const SelectorSettings& settings, net::PacketWriters& packetWriters, const NodeSelector& selector);

	// endregion

	// region RemoveOnlyNodeSelector / AgePeersTask

	/// A remove-only node selector.
	using RemoveOnlyNodeSelector = supplier<utils::KeySet>;

	/// Creates and prepares a remove-only node selector given \a settings.
	/// \note The selector is intended to be used in conjunction with CreateAgePeersTask for managing incoming connections.
	RemoveOnlyNodeSelector CreateRemoveOnlyNodeSelector(const SelectorSettings& settings);

	/// Creates a task for the specified service that ages nodes given \a settings and \a connectionContainer.
	thread::Task CreateAgePeersTask(const SelectorSettings& settings, net::ConnectionContainer& connectionContainer);

	/// Creates a task for the specified service that connects to nodes given \a settings, \a connectionContainer and \a selector.
	/// \note \a selector returns remove candidates (subset of active connections in \a connectionContainer).
	thread::Task CreateAgePeersTask(
			const SelectorSettings& settings,
			net::ConnectionContainer& connectionContainer,
			const RemoveOnlyNodeSelector& selector);

	// endregion
}}
