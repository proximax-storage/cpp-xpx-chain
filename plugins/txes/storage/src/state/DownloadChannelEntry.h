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

        /// Sets \a drive.
        void setDrive(const Key& drive) {
            m_drive = drive;
        }

        /// Gets drive.
        const Key& drive() const {
            return m_drive;
        }

        /// Sets \a transactionFee of download channel.
        void setTransactionFee(const Amount& transactionFee) {
            m_transactionFee = transactionFee;
        }

        /// Gets transaction fee.
        const Amount& transactionFee() const {
            return m_transactionFee;
        }

        /// Sets \a storageUnits of download channel.
        void setStorageUnits(const Amount& storageUnits) {
            m_storageUnits = storageUnits;
        }

        /// Gets storage units.
        const Amount& storageUnits() const {
            return m_storageUnits;
        }

	private:
		Key m_consumer;
		Key m_drive;
		Amount m_transactionFee;
		Amount m_storageUnits;
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
