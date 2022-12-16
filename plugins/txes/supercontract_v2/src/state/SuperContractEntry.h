/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include <map>
#include <set>
#include <optional>
#include <queue>
#include <catapult/types.h>
#include <catapult/crypto/CurvePoint.h>

namespace catapult::state {

struct ServicePayment {
	UnresolvedMosaicId MosaicId;
	catapult::Amount Amount;
};

struct ContractCall {
	Hash256 CallId;
	Key Caller;
	std::string FileName;
	std::string FunctionName;
	std::string ActualArguments;
	Amount ExecutionCallPayment;
	Amount DownloadCallPayment;
	std::vector<ServicePayment> ServicePayments;
};

struct CompletedCall {
	Hash256 CallId;
	// Zero means that it's an automatic execution
	Key Caller;
	// Zero Status Means Success
	uint16_t Status = 0;
	Amount ExecutionWork;
	Amount DownloadWork;
};

struct AutomaticExecutionsInfo {
	Amount m_automatedExecutionCallPayment;
	Amount m_automatedDownloadCallPayment;
	uint32_t m_automatedExecutionsNumber = 0U;
	std::optional<Height> m_automaticExecutionsEnabledSince;
};

enum class DeploymentStatus {
	NOT_STARTED,
	IN_PROGRESS,
	COMPLETED
};

struct ProofOfExecution {
	uint64_t StartBatchId = 0;
	crypto::CurvePoint T;
	crypto::Scalar R;
};

struct ExecutorInfo {
	uint64_t NextBatchToApprove = 0;
	ProofOfExecution PoEx;
};

struct Batch{
	crypto::CurvePoint PoExVerificationInformation;
	std::vector<CompletedCall> CompletedCalls;
};

// Mixin for storing supercontract details.
class SuperContractMixin {
	public:
		SuperContractMixin()
		{}

		const Key& driveKey() const {
			return m_driveKey;
		}

		void setDriveKey(const Key& driveKey) {
			m_driveKey = driveKey;
		}

		const Key& executionPaymentKey() const {
			return m_executionPaymentKey;
		}

		void setExecutionPaymentKey(const Key& executionPaymentKey) {
			m_executionPaymentKey = executionPaymentKey;
		}

		const Key& assignee() const {
			return m_assignee;
		}

		void setAssignee(const Key& assignee) {
			m_assignee = assignee;
		}

		AutomaticExecutionsInfo& automaticExecutionsInfo() {
			return m_automaticExecutionsInfo;
		}

		const AutomaticExecutionsInfo& automaticExecutionsInfo() const {
			return m_automaticExecutionsInfo;
		}

		std::deque<ContractCall>& requestedCalls() {
			return m_requestedCalls;
		}

		const std::deque<ContractCall>& requestedCalls() const {
			return m_requestedCalls;
		}

		DeploymentStatus deploymentStatus() const {
			if (m_batches.empty() && m_requestedCalls.empty()) {
				return DeploymentStatus::NOT_STARTED;
			}
			if (m_batches.empty()) {
				return DeploymentStatus::IN_PROGRESS;
			}
			return DeploymentStatus::COMPLETED;
		}

		std::map<Key, ExecutorInfo>& executorsInfo() {
			return m_executorsInfo;
		}

		const std::map<Key, ExecutorInfo>& executorsInfo() const {
			return m_executorsInfo;
		}

		uint64_t nextBatchId() const {
			return m_batches.size();
		}

		std::vector<Batch>& batches() {
			return m_batches;
		}

		const std::vector<Batch>& batches() const {
			return m_batches;
		}

		std::set<Hash256>& releasedTransactions() {
			return m_releasedTransactions;
		}

		const std::set<Hash256>& releasedTransactions() const {
			return m_releasedTransactions;
		}

	private:
		Key m_driveKey;
		Key m_executionPaymentKey;
		Key m_assignee;
		AutomaticExecutionsInfo m_automaticExecutionsInfo;
		std::deque<ContractCall> m_requestedCalls;
		std::map<Key, ExecutorInfo> m_executorsInfo;
		std::vector<Batch> m_batches;
		std::set<Hash256> m_releasedTransactions;
};

// Supercontract entry.
class SuperContractEntry : public SuperContractMixin {
	public:
		// Creates a super contract entry around \a key.
		explicit SuperContractEntry(const Key& key) : m_key(key)
		{}

	public:
		// Gets the super contract public key.
		const Key& key() const {
			return m_key;
		}

	private:
		Key m_key;
};
}