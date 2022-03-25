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
#include <vector>

namespace catapult { namespace state {

	static const Key DrivePaymentQueueKey = { { 1 } };
	static const Key DownloadChannelPaymentQueueKey = { { 2 } };
	static const Key DriveVerificationsTree = { { 3 } };

	// Mixin for storing drive details.
	class QueueMixin {
	public:
		QueueMixin()
		{}

	public:
		const Key& getFirst() const {
			return m_first;
		}
		void setFirst(const Key& first) {
			m_first = first;
		}
		const Key& getLast() const {
			return m_last;
		}
		void setLast(const Key& last) {
			m_last = last;
		}

	private:
		Key m_first;
		Key m_last;
	};

	// Drive entry.
	class QueueEntry : public QueueMixin {
	public:
		// Creates a drive entry around \a key.
		explicit QueueEntry(const Key& key): m_key(key), m_version(1)
		{}

	public:

		void setVersion(VersionType version) {
			m_version = version;
		}

		VersionType version() const {
			return m_version;
		}

		// Gets the drive public key.
		const Key& key() const {
			return m_key;
		}

	private:
		Key m_key;
		VersionType m_version;
	};
}}
