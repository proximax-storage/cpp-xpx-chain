/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once

#include "catapult/model/Notifications.h"
#include "catapult/crypto/CurvePoint.h"
#include "catapult/model/SupercontractModel.h"
#include "Mosaic.h"

#include <utility>

namespace catapult::model {

	DEFINE_NOTIFICATION_TYPE(All, SuperContract_v2, Deploy_Supercontract_v1, 0x0001);
	DEFINE_NOTIFICATION_TYPE(All, SuperContract_v2, Manual_Call_v1, 0x0002);
	DEFINE_NOTIFICATION_TYPE(All, SuperContract_v2, Automatic_Executions_Replenishment_v1, 0x0003);
	DEFINE_NOTIFICATION_TYPE(All, SuperContract_v2, Successful_Batch_Execution_v1, 0x0005);
	DEFINE_NOTIFICATION_TYPE(All, SuperContract_v2, Unsuccessful_Batch_Execution_v1, 0x0006);
	DEFINE_NOTIFICATION_TYPE(All, SuperContract_v2, Batch_Execution_Single_v1, 0x0007);


	template<VersionType version>
	struct DeploySupercontractNotification;

	template<>
	struct DeploySupercontractNotification<1> : public Notification {
	public:
		static constexpr auto Notification_Type = SuperContract_v2_Deploy_Supercontract_v1_Notification;

	public:
		explicit DeploySupercontractNotification(
				const Key& contractKey,
				const Key& driveKey,
				const Key& executionPaymentKey,
				const Key& assignee,
				std::string automaticExecutionFileName,
				std::string automaticExecutionsFunctionName,
				Amount automaticExecutionCallPayment,
				Amount automaticDownloadCallPayment)
				: Notification(Notification_Type, sizeof(DeploySupercontractNotification<1>))
				, ContractKey(contractKey)
				, DriveKey(driveKey)
			    , ExecutionPaymentKey(executionPaymentKey)
				, Assignee(assignee)
				, AutomaticExecutionFileName(std::move(automaticExecutionFileName))
				, AutomaticExecutionsFunctionName(std::move(automaticExecutionsFunctionName))
				, AutomaticExecutionCallPayment(automaticExecutionCallPayment)
				, AutomaticDownloadCallPayment(automaticDownloadCallPayment)
				{}

	public:
		Key ContractKey;
		Key DriveKey;
		Key ExecutionPaymentKey;
		Key Assignee;
		std::string AutomaticExecutionFileName;
		std::string AutomaticExecutionsFunctionName;
		Amount AutomaticExecutionCallPayment;
		Amount AutomaticDownloadCallPayment;
	};

	template<VersionType version>
	struct ManualCallNotification;

	template<>
	struct ManualCallNotification<1> : public Notification {
	public:
		static constexpr auto Notification_Type = SuperContract_v2_Manual_Call_v1_Notification;

	public:
		explicit ManualCallNotification(
				const Key& contractKey,
				const Hash256& callId,
				const Key& caller,
				std::string fileName,
				std::string functionName,
				std::string actualArguments,
				Amount executionCallPayment,
				Amount downloadCallPayment,
				std::vector<UnresolvedMosaic> servicePayments)
				: Notification(Notification_Type, sizeof(ManualCallNotification<1>))
				, CallId(callId)
				, ContractKey(contractKey)
				, Caller(caller)
				, FileName(std::move(fileName))
				, FunctionName(std::move(functionName))
				, ActualArguments(std::move(actualArguments))
				, ExecutionCallPayment(executionCallPayment)
				, DownloadCallPayment(downloadCallPayment)
				, ServicePayments(std::move(servicePayments))
				{}

	public:
		Hash256 CallId;
		Key ContractKey;
		Key Caller;
		std::string FileName;
		std::string FunctionName;
		std::string ActualArguments;
		Amount ExecutionCallPayment;
		Amount DownloadCallPayment;
		std::vector<UnresolvedMosaic> ServicePayments;
	};

	template<VersionType version>
	struct AutomaticExecutionsReplenishmentNotification;

	template<>
	struct AutomaticExecutionsReplenishmentNotification<1> : public Notification {
	public:
		static constexpr auto Notification_Type = SuperContract_v2_Automatic_Executions_Replenishment_v1_Notification;

	public:
		explicit AutomaticExecutionsReplenishmentNotification(
				const Key& contractKey,
				uint32_t number)
				: Notification(Notification_Type, sizeof(AutomaticExecutionsReplenishmentNotification<1>))
				, ContractKey(contractKey)
				, Number(number)
				{}

	public:
		Key ContractKey;
		uint32_t Number;
	};

	struct ExecutorWork : public UnresolvedAmountData {
	public:
		ExecutorWork(const Key& contractKey, Amount amountPerExecutor)
		: ContractKey(contractKey)
		, AmountPerExecutor(amountPerExecutor)
		{}

	public:
		Key ContractKey;
		Amount AmountPerExecutor;
	};

	struct AutomaticExecutorWork : public UnresolvedAmountData {
	public:
		AutomaticExecutorWork(const Key& contractKey, uint64_t numberOfExecutions)
		: ContractKey(contractKey)
		, NumberOfExecutions(numberOfExecutions)
		{}

	public:
		Key ContractKey;
		uint64_t NumberOfExecutions;
	};

	template<VersionType version>
	struct SuccessfulBatchExecutionNotification;

	template<>
	struct SuccessfulBatchExecutionNotification<1> : public Notification {
	public:
		static constexpr auto Notification_Type = SuperContract_v2_Successful_Batch_Execution_v1_Notification;

	public:
		explicit SuccessfulBatchExecutionNotification(
				const Key& contractKey,
				uint64_t batchId,
				bool updateStorageState,
				const Hash256& storageHash,
				uint64_t usedSizeBytes,
				uint64_t metaFilesSizeBytes,
				const crypto::CurvePoint& verificationInformation,
				const std::set<Key>& cosigners)
			: Notification(Notification_Type, sizeof(SuccessfulBatchExecutionNotification<1>))
			, ContractKey(contractKey)
			, BatchId(batchId)
			, UpdateStorageState(updateStorageState)
			, StorageHash(storageHash)
			, UsedSizeBytes(usedSizeBytes)
			, MetaFilesSizeBytes(metaFilesSizeBytes)
			, VerificationInformation(verificationInformation)
			, Cosigners(cosigners)
		{}

	public:
		Key ContractKey;
		uint64_t BatchId;
		bool UpdateStorageState;
		Hash256 StorageHash;
		uint64_t UsedSizeBytes;
		uint64_t MetaFilesSizeBytes;
		crypto::CurvePoint VerificationInformation;
		std::set<Key> Cosigners;
	};

	template<VersionType version>
	struct UnsuccessfulBatchExecutionNotification;

	template<>
	struct UnsuccessfulBatchExecutionNotification<1> : public Notification {
	public:
		static constexpr auto Notification_Type = SuperContract_v2_Unsuccessful_Batch_Execution_v1_Notification;

	public:
		explicit UnsuccessfulBatchExecutionNotification(
				const Key& contractKey,
				uint64_t batchId,
				const std::set<Key>& cosigners)
				: Notification(Notification_Type, sizeof(UnsuccessfulBatchExecutionNotification<1>))
				, ContractKey(contractKey)
				, BatchId(batchId)
				, Cosigners(cosigners)
				{}

	public:
		Key ContractKey;
		uint64_t BatchId;
		std::set<Key> Cosigners;
	};

	template<VersionType version>
	struct BatchExecutionSingleNotification;

	template<>
	struct BatchExecutionSingleNotification<1> : public Notification {
	public:
		static constexpr auto Notification_Type = SuperContract_v2_Unsuccessful_Batch_Execution_v1_Notification;

	public:
		explicit BatchExecutionSingleNotification(
				const Key& contractKey,
				uint64_t batchId)
				: Notification(Notification_Type, sizeof(BatchExecutionSingleNotification<1>))
				, ContractKey(contractKey)
				, BatchId(batchId)
				{}

	public:
		Key ContractKey;
		uint64_t BatchId;
	};
}