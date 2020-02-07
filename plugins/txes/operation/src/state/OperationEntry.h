/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "plugins/txes/lock_shared/src/state/LockInfo.h"
#include <set>
#include <vector>

namespace catapult { namespace state {

	/// An operation info.
	struct OperationEntry : public LockInfo {
	public:
		/// Creates a default operation entry.
		OperationEntry() : LockInfo()
		{}

		/// Creates a operation entry around \a operationToken.
		OperationEntry(const Hash256& operationToken)
			: LockInfo()
			, OperationToken(operationToken)
		{}

	public:
		/// Operation token.
		Hash256 OperationToken;

		/// Executors.
		std::set<Key> Executors;

		/// Transaction hashes.
		std::vector<Hash256> TransactionHashes;

		/// Operation result.
		uint16_t Result;
	};
}}
