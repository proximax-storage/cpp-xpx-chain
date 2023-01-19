/**
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/types.h"
#include "catapult/exceptions.h"

namespace catapult { namespace state {

	// Message hash entry.
	class MessageHashEntry {
	public:
		// Creates a message hash entry around \a key.
		explicit MessageHashEntry(const uint8_t& key) : m_version(1), m_key(key), m_hash()
		{}

		// Creates a message hash entry around \a key and \a hash.
		explicit MessageHashEntry(const uint8_t& key, const Hash256& hash) : m_version(1), m_key(key), m_hash(hash)
		{}

	public:
		const uint8_t& key() const {
			return m_key;
		}

		void setHash(const Hash256& hash) {
			m_hash = hash;
		}

		const Hash256& hash() const {
			return m_hash;
		}

		void setVersion(VersionType version) {
			m_version = version;
		}

		VersionType version() const {
			return m_version;
		}

	private:
		VersionType m_version;
		uint8_t m_key;
		Hash256 m_hash;
	};
}}
