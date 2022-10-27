/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "SuperContractEntityType.h"
#include "catapult/model/Mosaic.h"
#include "catapult/model/Transaction.h"

namespace catapult { namespace model {

#pragma pack(push, 1)

    /// Binary layout for a deploy transaction body.
	template<typename THeader>
    struct DeployTransactionBody : public THeader {
        private:
		using TransactionType = DeployTransactionBody<THeader>;

        public:
		    DEFINE_TRANSACTION_CONSTANTS(Entity_Type_Deploy, 1)
        
            /// Drive public key on which the SC is considered to be deployed
            Key DriveKey;

            // region Params for StartExecuteTransaction (except for SuperContractKey).

            /// Message of FileName string in bytes
		    uint16_t FileNameSize;

            /// Message of FunctionName string in bytes
            uint16_t FunctionNameSize;

            // Input parameters for this function string in bytes
            uint16_t ActualArgumentsSize;

            // Number of SC units provided to run the contract for all Executors on the Drive
            Amount ExecutionCallPayment;

            // Number of SM units provided to download data from the Internet for all Executors on the Drive
            Amount DownloadCallPayment;

            /// Number of necessary additional tokens to support specific Supercontract
            uint8_t ServicePaymentCount;

            /// For a single approvement if it is allowed 
            bool SingleApprovement;
            
            // endregion

            /// Name of the called .wasm file for automated executions in bytes
            uint16_t AutomatedExecutionFileNameSize;

            /// Name of the called function for automated executions in bytes
            uint16_t AutomatedExecutionFunctionNameSize;

            /// Limit of the SC units for one automated Supercontract Execution
            Amount AutomatedExecutionCallPayment;

            /// Limit of the SM units for one automated Supercontract Execution
            Amount AutomatedDownloadCallPayment;

            /// The number of prepaid automated executions. Can be increased via special transaction.
            uint32_t AutomatedExecutionsNumber;

            /// The Public Key to which the money is transferred in case of the Supercontract Closure.
            Key Assignee;

            /// FileName to save stream in.
    		DEFINE_TRANSACTION_VARIABLE_DATA_ACCESSORS(FileName, uint8_t)

            /// FunctionName to save stream in.
    		DEFINE_TRANSACTION_VARIABLE_DATA_ACCESSORS(FunctionName, uint8_t)

            /// ActualArguments to save stream in.
    		DEFINE_TRANSACTION_VARIABLE_DATA_ACCESSORS(ActualArguments, uint8_t)

            // followed by ServicePayment data if ServicePaymentCount != 0
		    DEFINE_TRANSACTION_VARIABLE_DATA_ACCESSORS(ServicePayment, UnresolvedMosaic)

            /// AutomatedExecutionFileName to save stream in.
    		DEFINE_TRANSACTION_VARIABLE_DATA_ACCESSORS(AutomatedExecutionFileName, uint8_t)

            /// AutomatedExecutionFileName to save stream in.
    		DEFINE_TRANSACTION_VARIABLE_DATA_ACCESSORS(AutomatedExecutionFunctionName, uint8_t)

        private:
            template<typename T>
            static auto* FileNamePtr(T& transaction) {
                return transaction.FileNameSize ? THeader::PayloadStart(transaction) : nullptr;
            }

            template<typename T>
            static auto* FunctionNamePtr(T& transaction) {
                return transaction.FunctionNameSize ? THeader::PayloadStart(transaction) : nullptr;
            }

            template<typename T>
            static auto* ActualArgumentsPtr(T& transaction) {
                return transaction.ActualArgumentsSize ? THeader::PayloadStart(transaction) : nullptr;
            }

            template<typename T>
            static auto* ServicePaymentPtr(T& transaction) {
                auto* pPayloadStart = THeader::PayloadStart(transaction);
                return transaction.ServicePaymentCount && pPayloadStart ? pPayloadStart : nullptr;
            }

            template<typename T>
            static auto* AutomatedExecutionFileNamePtr(T& transaction) {
                return transaction.AutomatedExecutionFileNameSize ? THeader::PayloadStart(transaction) : nullptr;
            }
            
            template<typename T>
            static auto* AutomatedExecutionFunctionNamePtr(T& transaction) {
                return transaction.AutomatedExecutionFunctionNameSize ? THeader::PayloadStart(transaction) : nullptr;
            }

        public:
            // Calculates the real size of a deploy \a transaction.
            static constexpr uint64_t CalculateRealSize(const TransactionType& transaction) noexcept {
			    return sizeof(TransactionType) + transaction.FileNameSize + transaction.FunctionNameSize + transaction.ActualArgumentsSize + transaction.ServicePaymentCount * sizeof(UnresolvedMosaic) + transaction.AutomatedExecutionFileNameSize + transaction.AutomatedExecutionFunctionNameSize;
		    }
    };

    DEFINE_EMBEDDABLE_TRANSACTION(Deploy)

#pragma pack(pop)
}}