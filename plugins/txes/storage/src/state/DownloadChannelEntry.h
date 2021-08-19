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
#include <map>

namespace catapult { namespace state {

	// Mixin for storing download channel details.
	class DownloadChannelMixin {
	public:
		DownloadChannelMixin() = default;

	public:
		/// Sets \a consumer of download channel.
		void setConsumer(const Key& consumer) {
			m_consumer = consumer;
		}

		/// Gets consumer of download channel.
		const Key& consumer() const {
			return m_consumer;
		}

        /// Sets \a downloadSize of download channel.
        void setDownloadSize(const uint64_t& downloadSize) {
			m_downloadSize = downloadSize;
        }

		/// Increases download size of the download channel by \a delta.
		void increaseDownloadSize(const uint64_t& delta) {
			m_downloadSize = m_downloadSize + delta;
		}

        /// Gets download size.
        const uint64_t& downloadSize() const {
            return m_downloadSize;
        }

		/// Gets list of public keys.
		const std::vector<Key>& listOfPublicKeys() const {
			return m_listOfPublicKeys;
		}

		/// Gets list of public keys.
		std::vector<Key>& listOfPublicKeys() {
			return m_listOfPublicKeys;
		}

	private:
		Key m_consumer;
		uint64_t m_downloadSize; // In Mbytes
		std::vector<Key> m_listOfPublicKeys;
	};

	// DownloadChannel channel entry.
	class DownloadChannelEntry : public DownloadChannelMixin {
	public:
		// Creates a download channel entry around \a id.
		explicit DownloadChannelEntry(const Hash256& id) : m_id(id), m_version(1)
		{}

	public:
		// Gets the download channel id.
		const Hash256 & id() const {
			return m_id;
		}

		void setVersion(VersionType version) {
			m_version = version;
		}

		VersionType version() const {
			return m_version;
		}

	private:
		Hash256 m_id;
		VersionType m_version;
	};
}}
