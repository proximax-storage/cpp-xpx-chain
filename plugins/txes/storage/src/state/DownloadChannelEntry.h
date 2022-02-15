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
		DownloadChannelMixin()
			: m_downloadSizeMegabytes(0)
			, m_downloadApprovalCount(0)
		{}

	public:
		/// Sets \a consumer of download channel.
		void setConsumer(const Key& consumer) {
			m_consumer = consumer;
		}

		/// Gets consumer of download channel.
		const Key& consumer() const {
			return m_consumer;
		}

		/// Sets \a drive of download channel.
		void setDrive(const Key& drive) {
			m_drive = drive;
		}

		/// Gets drive of download channel.
		const Key& drive() const {
			return m_drive;
		}

        /// Sets \a downloadSize of download channel.
        void setDownloadSize(const uint64_t& downloadSize) {
			m_downloadSizeMegabytes = downloadSize;
        }

		/// Increases download size of the download channel by \a delta.
		void increaseDownloadSize(const uint64_t& delta) {
			m_downloadSizeMegabytes = m_downloadSizeMegabytes + delta;
		}

        /// Gets download size.
        const uint64_t& downloadSize() const {
            return m_downloadSizeMegabytes;
        }

		/// Gets number of completed download approval transactions.
		const uint16_t& downloadApprovalCount() const {
			return m_downloadApprovalCount;
		}

		/// Sets number of completed download approval transactions.
		void setDownloadApprovalCount(const uint16_t& count) {
			m_downloadApprovalCount = count;
		}

		/// Sets last download approval initiation timestamp.
		void setLastDownloadApprovalInitiated(const catapult::Timestamp& timestamp) {
			m_lastDownloadApprovalInitiated = timestamp;
		}

		/// Increases number of completed download approval transactions by one.
		void incrementDownloadApprovalCount() {
			++m_downloadApprovalCount;
		}

		/// Gets list of public keys.
		const std::vector<Key>& listOfPublicKeys() const {
			return m_listOfPublicKeys;
		}

		/// Gets list of public keys.
		std::vector<Key>& listOfPublicKeys() {
			return m_listOfPublicKeys;
		}

		/// Gets replicators' cumulative payment amounts
		const std::map<Key, Amount>& cumulativePayments() const {
			return m_cumulativePayments;
		}

		/// Gets replicators' cumulative payment amounts
		std::map<Key, Amount>& cumulativePayments() {
			return m_cumulativePayments;
		}

		/// Gets last download approval initiation timestamp.
		const catapult::Timestamp& lastDownloadApprovalInitiated() const {
			return m_lastDownloadApprovalInitiated;
		}

		/// Gets last download approval initiation timestamp.
		catapult::Timestamp& lastDownloadApprovalInitiated() {
			return m_lastDownloadApprovalInitiated;
		}

	private:
		Key m_consumer;
		Key m_drive;
		uint64_t m_downloadSizeMegabytes; // In Mbytes
		uint16_t m_downloadApprovalCount;
		std::vector<Key> m_listOfPublicKeys;
		std::map<Key, Amount> m_cumulativePayments;
		catapult::Timestamp m_lastDownloadApprovalInitiated;
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
