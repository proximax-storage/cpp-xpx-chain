/**
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/types.h"
#include "catapult/exceptions.h"

namespace catapult { namespace state {

	// Mixin for storing DBRB message hash.
	class MessageHashMixin {
	public:
		MessageHashMixin()
				: m_hash()
		{}

	public:
		/// Sets stored message \a hash.
		void setHash(const Hash256& hash) {
			m_hash = hash;
		}

		/// Gets stored message hash.
		const Hash256& hash() const {
			return m_hash;
		}

	private:
		Hash256 m_hash;
	};

	// Message hash entry.
	class MessageHashEntry : public MessageHashMixin {
	public:
		// Creates a message hash entry around \a key.
		explicit MessageHashEntry(const uint8_t& key) : m_key(key), m_version(1)
		{}

	public:
		const uint8_t& key() const {
			return m_key;
		}

		void setVersion(VersionType version) {
			m_version = version;
		}

		VersionType version() const {
			return m_version;
		}

	private:
		uint8_t m_key;
		VersionType m_version;
	};
}}
