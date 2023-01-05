/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/model/Elements.h"
#include "catapult/types.h"
#include "catapult/utils/ArraySet.h"
#include "catapult/utils/NonCopyable.h"
#include <catapult/crypto/CurvePoint.h>
#include <vector>
#include <optional>

namespace catapult { namespace cache { class CatapultCache; } }

namespace catapult { namespace state {

	struct PublishedBatchInfo {
		uint64_t BatchIndex;
		bool BatchSuccess;
		Hash256 DriveState;
		crypto::CurvePoint PoExVerificationInfo;
		std::set<Key> Cosigners;
	};

	struct ManualCallInfo {
		Hash256 CallId;
		std::string FileName;
		std::string FunctionName;
		std::string Arguments;
		Amount ExecutionPayment;
		Amount DownloadPayment;
		Key Caller;
		Height BlockHeight;
	};

	struct ContractInfo {
		Key DriveKey;
		std::set<Key> Executors;
		uint64_t BatchesExecuted = 0U;
		std::string AutomaticExecutionsFileName;
		std::string AutomaticExecutionsFunctionName;
		uint64_t AutomaticExecutionsSCLimit = 0U;
		uint64_t AutomaticExecutionsSMLimit = 0U;
		std::optional<Height> AutomaticExecutionsEnabledSince;
		Height AutomaticExecutionsNextBlockToCheck;
		std::optional<PublishedBatchInfo> LastPublishedBatch;
		std::vector<ManualCallInfo> ManualCalls;
	};

	/// Interface for contract state.
	class ContractState : public utils::NonCopyable {
	public:
		virtual ~ContractState() = default;

	public:
		void setCache(cache::CatapultCache* pCache) {
			m_pCache = pCache;
		}

	public:

		virtual bool isExecutorRegistered(const Key& key) const = 0;

	public:
		virtual Height getChainHeight() = 0;

		virtual bool contractExists(const Key& contractKey) = 0;

		virtual std::shared_ptr<const model::BlockElement> getBlock(Height height) = 0;

		virtual std::optional<Height> getAutomaticExecutionsEnabledSince(const Key& contractKey) = 0;

		virtual Hash256 getDriveState(const Key& contractKey) = 0;

		virtual std::set<Key> getContracts(const Key& executorKey) = 0;

		virtual std::set<Key> getExecutors(const Key& contractKey) = 0;

		virtual ContractInfo getContractInfo(const Key& contractKey) = 0;

	protected:
		cache::CatapultCache* m_pCache;
	};
}}
