/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "SuperContractEntityType.h"
#include "catapult/model/Mosaic.h"
#include "catapult/model/Transaction.h"

namespace catapult::model {

#pragma pack(push, 1)

    /// Binary layout for a deploy transaction body.
	template<typename THeader>
	struct DeployContractTransactionBody : public THeader {
        private:
		using TransactionType = DeployContractTransactionBody<THeader>;

        public:
		    DEFINE_TRANSACTION_CONSTANTS(Entity_Type_DeployContractTransaction, 1)
        
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
            uint8_t ServicePaymentsCount;
            
            // endregion

            /// Name of the called .wasm file for automatic executions in bytes
            uint16_t AutomaticExecutionsFileNameSize;

            /// Name of the called function for automatic executions in bytes
            uint16_t AutomaticExecutionsFunctionNameSize;

            /// Limit of the SC units for one automatic Supercontract Execution
            Amount AutomaticExecutionCallPayment;

            /// Limit of the SM units for one automatic Supercontract Execution
            Amount AutomaticDownloadCallPayment;

            /// The number of prepaid automatic executions. Can be increased via special transaction.
            uint32_t AutomaticExecutionsNumber;

            /// The Public Key to which the money is transferred in case of the Supercontract Closure.
            Key Assignee;

            /// FileName to save stream in.
    		DEFINE_TRANSACTION_VARIABLE_DATA_ACCESSORS(FileName, uint8_t)

            /// FunctionName to save stream in.
    		DEFINE_TRANSACTION_VARIABLE_DATA_ACCESSORS(FunctionName, uint8_t)

            /// ActualArguments to save stream in.
    		DEFINE_TRANSACTION_VARIABLE_DATA_ACCESSORS(ActualArguments, uint8_t)

            // followed by ServicePayments data if ServicePaymentsCount != 0
		    DEFINE_TRANSACTION_VARIABLE_DATA_ACCESSORS(ServicePayments, UnresolvedMosaic)

            /// AutomaticExecutionsFileName to save stream in.
    		DEFINE_TRANSACTION_VARIABLE_DATA_ACCESSORS(AutomaticExecutionsFileName, uint8_t)

            /// AutomaticExecutionsFileName to save stream in.
    		DEFINE_TRANSACTION_VARIABLE_DATA_ACCESSORS(AutomaticExecutionsFunctionName, uint8_t)

        private:
            template<typename T>
            static auto* FileNamePtrT(T& transaction) {
    			auto* pPayloadStart = THeader::PayloadStart(transaction);
    			return transaction.FileNameSize && pPayloadStart ? pPayloadStart : nullptr;
			}

            template<typename T>
            static auto* FunctionNamePtrT(T& transaction) {
                return transaction.FunctionNameSize
							   ? THeader::PayloadStart(transaction)
										 + transaction.FileNameSize
							   : nullptr;
            }

			template<typename T>
			static auto* ActualArgumentsPtrT(T& transaction) {
    			auto* pPayloadStart = THeader::PayloadStart(transaction);
				return transaction.ActualArgumentsSize && pPayloadStart
							   ? pPayloadStart
										 + transaction.FileNameSize
										 + transaction.FunctionNameSize
							   : nullptr;
			}

			template<typename T>
            static auto* ServicePaymentsPtrT(T& transaction) {
				auto* pPayloadStart = THeader::PayloadStart(transaction);
				return transaction.ServicePaymentsCount && pPayloadStart
							   ? pPayloadStart
										 + transaction.FileNameSize
										 + transaction.FunctionNameSize
										 + transaction.ActualArgumentsSize
							   : nullptr;
			}

            template<typename T>
            static auto* AutomaticExecutionsFileNamePtrT(T& transaction) {
				auto* pPayloadStart = THeader::PayloadStart(transaction);
				return transaction.AutomaticExecutionsFileNameSize && pPayloadStart
							   ? pPayloadStart
										 + transaction.FileNameSize
										 + transaction.FunctionNameSize
										 + transaction.ActualArgumentsSize
										 + transaction.ServicePaymentsCount * sizeof(UnresolvedMosaic)
							   : nullptr;
			}
            
            template<typename T>
            static auto* AutomaticExecutionsFunctionNamePtrT(T& transaction) {
				auto* pPayloadStart = THeader::PayloadStart(transaction);
				return transaction.AutomaticExecutionsFunctionNameSize && pPayloadStart
							   ? pPayloadStart
										 + transaction.FileNameSize
										 + transaction.FunctionNameSize
										 + transaction.ActualArgumentsSize
										 + transaction.ServicePaymentsCount * sizeof(UnresolvedMosaic)
										 + transaction.AutomaticExecutionsFileNameSize
							   : nullptr;
			}

        public:
            // Calculates the real size of a deploy \a transaction.
            static constexpr uint64_t CalculateRealSize(const TransactionType& transaction) noexcept {
			    return sizeof(TransactionType) + transaction.FileNameSize + transaction.FunctionNameSize + transaction.ActualArgumentsSize + transaction.ServicePaymentsCount * sizeof(UnresolvedMosaic) + transaction.AutomaticExecutionsFileNameSize + transaction.AutomaticExecutionsFunctionNameSize;
		    }
    };

    DEFINE_EMBEDDABLE_TRANSACTION(DeployContract)

#pragma pack(pop)
}