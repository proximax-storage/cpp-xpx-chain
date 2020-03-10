/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "src/model/OperationTypes.h"
#include "plugins/txes/lock_shared/src/state/LockInfo.h"
#include <set>
#include <vector>

namespace catapult { namespace state {

	/// An operation info.
	struct OperationEntry : public LockInfo {
	public:
		/// Creates a default operation entry.
		OperationEntry()
			: LockInfo()
			, Result(model::Operation_Result_None)
		{}

		/// Creates a operation entry around \a operationToken.
		OperationEntry(const Hash256& operationToken)
			: LockInfo()
			, OperationToken(operationToken)
			, Result(model::Operation_Result_None)
		{}

	public:
		/// Operation token.
		Hash256 OperationToken;

		/// Executors.
		std::set<Key> Executors;

		/// Transaction hashes.
		std::vector<Hash256> TransactionHashes;

		/// Operation result.
		model::OperationResult Result;
	};
}}
