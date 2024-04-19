/**
*** Copyright 2024 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/types.h"
#include "catapult/exceptions.h"
#include "CommonEntities.h"
#include "catapult/state/StorageState.h"
#include "catapult/utils/ArraySet.h"

namespace catapult { namespace state {

	// Boot key / replicator entry.
	class BootKeyReplicatorEntry {
	public:
		// Creates a replicator entry around \a nodeBootKey and \a replicatorKey.
		BootKeyReplicatorEntry(const Key& nodeBootKey, const Key& replicatorKey)
			: m_version(1)
			, m_nodeBootKey(nodeBootKey)
			, m_replicatorKey(replicatorKey)
		{}

	public:
		void setVersion(VersionType version) {
			m_version = version;
		}

		VersionType version() const {
			return m_version;
		}

		// Gets the boot public key of the node where replicator is running on.
		const Key& nodeBootKey() const {
			return m_nodeBootKey;
		}

		// Sets the public key of the replicator.
		void setReplicatorKey(const Key& replicatorKey) {
			m_replicatorKey = replicatorKey;
		}

		// Gets the public key of the replicator.
		const Key& replicatorKey() const {
			return m_replicatorKey;
		}

	private:
		VersionType m_version;
		Key m_nodeBootKey;
		Key m_replicatorKey;
	};
}}
