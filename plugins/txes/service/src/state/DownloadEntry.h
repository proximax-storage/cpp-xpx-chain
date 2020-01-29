/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "src/model/ServiceTypes.h"
#include "plugins/txes/lock_shared/src/state/LockInfo.h"
#include <set>

namespace catapult { namespace state {

	/// A download entry.
	struct DownloadEntry {
	public:
		/// Creates a default download entry.
		DownloadEntry()
		{}

		/// Creates a default download entry around \a operationToken.
		DownloadEntry(const Hash256& operationToken)
			: OperationToken(operationToken)
		{}

	public:
		/// Operation token.
		Hash256 OperationToken;

		/// Drive key.
		Key DriveKey;

		/// File recipient key.
		Key FileRecipient;

		/// Height at which download ends.
		catapult::Height Height;

		/// Hashes of files to download.
		std::set<Hash256> Files;

	public:
		/// Returns status of download entry.
		LockStatus status() const {
			return Files.empty() ? LockStatus::Used : LockStatus::Unused;
		}

		/// Returns \c true if download entry is active at \a height.
		constexpr bool isActive(catapult::Height height) const {
			return height < Height && !Files.empty();
		}
	};
}}
