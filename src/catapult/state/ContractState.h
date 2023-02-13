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
#include <catapult/config/BlockchainConfiguration.h>
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

	struct Payment {
		UnresolvedMosaicId PaymentUnresolvedMosaicId;
		Amount PaymentAmount;
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
		std::vector<Payment> Payments;
	};

	struct ExecutorProofOfExecutionInfo {
		uint64_t StartBatchId = 0;
		crypto::CurvePoint T;
		crypto::Scalar R;
	};

	struct ExecutorStateInfo {
		uint64_t NextBatchToApprove = 0;
		ExecutorProofOfExecutionInfo PoEx;
	};

	struct ContractInfo {
		Key DriveKey;
		std::map<Key, ExecutorStateInfo> Executors;
		std::map<uint64_t, crypto::CurvePoint> RecentBatches;
		Hash256 DeploymentBaseModificationId;
		std::string AutomaticExecutionsFileName;
		std::string AutomaticExecutionsFunctionName;
		Amount AutomaticExecutionCallPayment;
		Amount AutomaticDownloadCallPayment;
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

		void setBlockProvider(std::function<std::shared_ptr<const model::BlockElement> (Height height)>&& blockProvider) {
			if (m_blockProvider) {
				CATAPULT_THROW_RUNTIME_ERROR("block provider already set");
			}

			m_blockProvider = std::move(blockProvider);
		}

	public:

		virtual bool contractExists(const Key& contractKey) const = 0;

		virtual std::shared_ptr<const model::BlockElement> getBlock(Height height) const = 0;

		virtual std::optional<Height> getAutomaticExecutionsEnabledSince(
				const Key& contractKey,
				const Height& actualHeight,
				const config::BlockchainConfiguration config) const = 0;

		virtual Height getAutomaticExecutionsNextBlockToCheck(const Key& contractKey) const = 0;

		virtual Hash256 getDriveState(const Key& contractKey) const = 0;

		virtual std::set<Key> getContracts(const Key& executorKey) const = 0;

		virtual std::map<Key, ExecutorStateInfo> getExecutors(const Key& contractKey) const = 0;

		virtual ContractInfo getContractInfo(
				const Key& contractKey,
				const Height& actualHeight,
				const config::BlockchainConfiguration config) const = 0;

		protected:

		cache::CatapultCache* m_pCache = nullptr;

		std::function<std::shared_ptr<const model::BlockElement> (Height height)> m_blockProvider;

	};
}}
