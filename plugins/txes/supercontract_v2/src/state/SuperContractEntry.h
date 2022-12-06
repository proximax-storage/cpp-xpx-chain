/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include <catapult/model/Mosaic.h>
#include <map>
#include <optional>
#include <queue>

namespace catapult::state {

struct ContractCall {
	Hash256 CallId;
	Key Caller;
	std::string FileName;
	std::string FunctionName;
	std::string ActualArguments;
	Amount ExecutionCallPayment;
	Amount DownloadCallPayment;
	std::vector<model::UnresolvedMosaic> ServicePayments;
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
	uint64_t BatchId;
	crypto::CurvePoint T;
	crypto::Scalar R;
};

struct Batch{
	crypto::CurvePoint PoExVerificationInformation;
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

		std::queue<ContractCall>& requestedCalls() {
			return m_requestedCalls;
		}

		const std::queue<ContractCall>& requestedCalls() const {
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

		std::map<Key, ProofOfExecution>& proofs() {
			return m_proofs;
		}

		const std::map<Key, ProofOfExecution>& proofs() const {
			return m_proofs;
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

	private:
		Key m_driveKey;
		Key m_executionPaymentKey;
		Key m_assignee;
		AutomaticExecutionsInfo m_automaticExecutionsInfo;
		std::queue<ContractCall> m_requestedCalls;
		std::map<Key, ProofOfExecution> m_proofs;
		std::vector<Batch> m_batches;
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