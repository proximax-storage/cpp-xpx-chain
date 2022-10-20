/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "DeployTransaction.h"
#include "catapult/model/Notifications.h"
#include "catapult/utils/MemoryUtils.h"

namespace catapult { namespace model {

    // region contract notification types

    /// Defines a contract notification type.
	DEFINE_NOTIFICATION_TYPE(All, Contract_v2, Deploy_v1, 0x0001);

	// endregion

    /// Notification of a deploy.
	template<VersionType version>
    struct DeployNotification;

    template<>
	struct DeployNotification<1> : public Notification {
        public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Contract_v2_Deploy_v1_Notification;

        public:
		/// Creates a notification around \a driveKey, \a startExecuteTransactionParams and \a pModifications.
		explicit DeployNotification(
            const Key& owner,
			const Key& driveKey,
			const std::string& fileName,
            const std::string& functionName,
            const std::string& actualArguments,
            const Amount& executionCallPayment,
            const Amount& downloadCallPayment,
            const uint8_t servicePaymentCount,
            const UnresolvedMosaic* servicePaymentPtr,
            bool singleApprovement,
			const std::string& automatedExecutionFileName,
			const std::string& automatedExecutionFunctionName,
			const Amount& automatedExecutionCallPayment,
			const Amount& automatedDownloadCallPayment,
			const uint32_t& automatedExecutionsNumber,
			const Key& assignee)
            : Notification(Notification_Type, sizeof(DeployNotification<1>))
            , Owner(owner)
            , DriveKey(driveKey)
            , FileName(fileName)
            , FunctionName(functionName)
            , ActualArguments(actualArguments)
            , ExecutionCallPayment(executionCallPayment)
            , DownloadCallPayment(downloadCallPayment)
            , ServicePaymentCount(servicePaymentCount)
            , ServicePaymentPtr(servicePaymentPtr)
            , SingleApprovement(singleApprovement)
            , AutomatedExecutionFileName(automatedExecutionFileName)
            , AutomatedExecutionFunctionName(automatedExecutionFunctionName)
            , AutomatedExecutionCallPayment(automatedExecutionCallPayment)
            , AutomatedDownloadCallPayment(automatedDownloadCallPayment)
            , AutomatedExecutionsNumber(automatedExecutionsNumber)
            , Assignee(assignee)
        {}

        public:
            /// Public key of owner.
            Key Owner;

            /// Drive public key on which the SC is considered to be deployed
            Key DriveKey;

            // region Params for StartExecuteTransaction (except for SuperContractKey).

            /// FileName to save stream in
		    std::string FileName;

            /// FunctionName to save stream in
            std::string FunctionName;

            // ActualArguments to save stream in
            std::string ActualArguments;

            // Number of SC units provided to run the contract for all Executors on the Drive
            Amount ExecutionCallPayment;

            // Number of SM units provided to download data from the Internet for all Executors on the Drive
            Amount DownloadCallPayment;

            /// Number of necessary additional tokens to support specific Supercontract
            uint8_t ServicePaymentCount;

            /// Const pointer to the additional tokens.
            const UnresolvedMosaic* ServicePaymentPtr;

            /// For a single approvement if it is allowed 
            bool SingleApprovement;
            
            // endregion

            /// Length of the Name of the called .wasm file for automated executions
            std::string AutomatedExecutionFileName;

            /// Length of the Name of the called function for automated executions
            std::string AutomatedExecutionFunctionName;

            /// Limit of the SC units for one automated Supercontract Execution
            Amount AutomatedExecutionCallPayment;

            /// Limit of the SM units for one automated Supercontract Execution
            Amount AutomatedDownloadCallPayment;

            /// The number of prepaid automated executions. Can be increased via special transaction.
            uint32_t AutomatedExecutionsNumber;

            /// The Public Key to which the money is transferred in case of the Supercontract Closure.
            Key Assignee;
    };

    // endregion
}}