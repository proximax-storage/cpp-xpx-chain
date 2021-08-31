/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/types.h"
#include "catapult/exceptions.h"
#include "catapult/model/Mosaic.h"
#include "catapult/utils/ArraySet.h"
#include "catapult/utils/IntegerMath.h"

namespace catapult { namespace state {

	// Mixin for storing BLS keys details.
	class BlsKeysMixin {
	public:
		BlsKeysMixin() {}

	public:
		/// Sets \a key.
        void setKey(const Key& key) {
			m_key = key;
        }

        /// Gets stored key.
        const Key& key() const {
            return m_key;
        }

	private:
		Key m_key;
	};

	// BLS keys entry.
	class BlsKeysEntry : public BlsKeysMixin {
	public:
		// Creates a BLS keys entry around \a blsKey.
		explicit BlsKeysEntry(const BLSPublicKey& blsKey) : m_blsKey(blsKey), m_version(1)
		{}

	public:
		// Gets the BLS key of the entry.
		const BLSPublicKey& blsKey() const {
			return m_blsKey;
		}

		void setVersion(VersionType version) {
			m_version = version;
		}

		VersionType version() const {
			return m_version;
		}

	private:
		BLSPublicKey m_blsKey;
		VersionType m_version;
	};
}}
