/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/types.h"
#include <map>
#include <set>

namespace catapult { namespace state {

	using DownloadMap = std::map<Hash256, std::set<Hash256>>;
	using FileRecipientMap = std::map<Key, DownloadMap>;

	// Download entry.
	class DownloadEntry {
	public:
		// Creates a download entry around \a driveKey.
		DownloadEntry(const Key& driveKey)
			: m_driveKey(driveKey)
		{}

	public:
		/// Gets the drive key.
		const Key& driveKey() const {
			return m_driveKey;
		}

		/// Sets the \a driveKey.
		void setDriveKey(const Key& driveKey) {
			m_driveKey = driveKey;
		}

		/// Gets the file recipients.
		const FileRecipientMap& fileRecipients() const {
			return m_fileRecipients;
		}

		/// Gets the file recipients.
		FileRecipientMap& fileRecipients() {
			return m_fileRecipients;
		}

	private:
		Key m_driveKey;
		FileRecipientMap m_fileRecipients;
	};
}}
