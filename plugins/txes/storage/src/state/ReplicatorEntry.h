/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/types.h"
#include "catapult/exceptions.h"

namespace catapult { namespace state {

	// Mixin for storing replicator details.
	class ReplicatorMixin {
	public:
		ReplicatorMixin() = default;

	public:
		/// Sets the capacity of the replicator.
		void setCapacity(const Amount& capacity) {
			m_capacity = capacity;
		}

		/// Gets the capacity of the replicator.
		const Amount& capacity() const {
			return m_capacity;
		}

	private:
		Amount m_capacity;
	};

	// Replicator entry.
	class ReplicatorEntry : public ReplicatorMixin {
	public:
		// Creates a replicator entry around \a key.
		explicit ReplicatorEntry(const Key& key) : m_key(key), m_version(1)
		{}

	public:
		// Gets the public key of the replicator.
		const Key& key() const {
			return m_key;
		}

		void setVersion(VersionType version) {
			m_version = version;
		}

		VersionType version() const {
			return m_version;
		}

	private:
		Key m_key;
		VersionType m_version;
	};
}}
