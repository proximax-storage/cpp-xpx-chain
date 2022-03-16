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
		const uint16_t& downloadApprovalCountLeft() const {
			return m_downloadApprovalCount;
		}

		/// Sets number of completed download approval transactions.
		void setDownloadApprovalCountLeft(const uint16_t& count) {
			m_downloadApprovalCount = count;
		}

		/// Increases number of completed download approval transactions by one.
		void decrementDownloadApprovalCount() {
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

		/// Gets replicators of the download shard.
		const utils::SortedKeySet& shardReplicators() const {
			return m_shardReplicators;
		}

		/// Gets replicators of the download shard.
		utils::SortedKeySet& shardReplicators() {
			return m_shardReplicators;
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
		const Timestamp& getLastDownloadApprovalInitiated() const {
			return m_lastDownloadApprovalInitiated;
		}

		/// Gets last download approval initiation timestamp.
		void setLastDownloadApprovalInitiated(const Timestamp& timestamp) {
			m_lastDownloadApprovalInitiated = timestamp;
		}

		const Key& getQueueNext() const {
			return m_paymentsQueueNext;
		}

		void setQueueNext(const Key& paymentsQueueNext) {
			m_paymentsQueueNext = paymentsQueueNext;
		}

		const Key& getQueuePrevious() const {
			return m_paymentsQueuePrevious;
		}

		void setQueuePrevious(const Key& paymentsQueuePrevious) {
			m_paymentsQueuePrevious = paymentsQueuePrevious;
		}

		bool isFinishPublished() const {
			return m_finishPublished;
		}

		void setFinishPublished(bool finishPublished) {
			m_finishPublished = finishPublished;
		}

		bool isCloseInitiated() const {
			return isFinishPublished() or downloadApprovalCountLeft() == 0;
		}

		const std::optional<Hash256>& downloadApprovalInitiationEvent() const {
			return m_downloadApprovalInitiationEvent;
		}

		std::optional<Hash256>& downloadApprovalInitiationEvent() {
			return m_downloadApprovalInitiationEvent;
		}

	private:
		Key m_consumer;
		Key m_drive;
		uint64_t m_downloadSizeMegabytes; // In Mbytes
		uint16_t m_downloadApprovalCount;
		std::vector<Key> m_listOfPublicKeys;
		utils::SortedKeySet m_shardReplicators;
		std::map<Key, Amount> m_cumulativePayments;
		std::optional<Hash256> m_downloadApprovalInitiationEvent;
		catapult::Timestamp m_lastDownloadApprovalInitiated;
		bool m_finishPublished;

		Key m_paymentsQueuePrevious;
		Key m_paymentsQueueNext;
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

		Key entryKey() {
			return m_id.array();
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
