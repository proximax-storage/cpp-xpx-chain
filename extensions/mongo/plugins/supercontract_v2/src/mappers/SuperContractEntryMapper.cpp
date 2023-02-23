/**
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "SuperContractEntryMapper.h"
#include "catapult/utils/Casting.h"
#include "mongo/src/mappers/MapperUtils.h"

using namespace catapult::mongo::mappers;

namespace catapult { namespace mongo { namespace plugins {

    // region ToDbModel

    namespace {
        void StreamAutomaticExecutionsInfo(bson_stream::document& builder, const state::AutomaticExecutionsInfo& automaticExecutionsInfo) {
            auto pAutomaticExecutionFileName = reinterpret_cast<const uint8_t*>(automaticExecutionsInfo.AutomaticExecutionFileName.c_str());
            auto pAutomaticExecutionsFunctionName = reinterpret_cast<const uint8_t*>(automaticExecutionsInfo.AutomaticExecutionsFunctionName.c_str());
            builder << "automaticExecutionsInfo" << bson_stream::open_document
                << "automaticExecutionFileName" << ToBinary(pAutomaticExecutionFileName, automaticExecutionsInfo.AutomaticExecutionFileName.size())
                << "automaticExecutionsFunctionName" << ToBinary(pAutomaticExecutionsFunctionName, automaticExecutionsInfo.AutomaticExecutionsFunctionName.size())
                << "automaticExecutionsNextBlockToCheck" << ToInt64(automaticExecutionsInfo.AutomaticExecutionsNextBlockToCheck)
                << "automaticExecutionCallPayment" << ToInt64(automaticExecutionsInfo.AutomaticExecutionCallPayment)
                << "automaticDownloadCallPayment" << ToInt64(automaticExecutionsInfo.AutomaticDownloadCallPayment)
                << "automatedExecutionsNumber" << static_cast<int32_t>(automaticExecutionsInfo.AutomatedExecutionsNumber)
                << "automaticExecutionsPrepaidSinceHasValue" << automaticExecutionsInfo.AutomaticExecutionsPrepaidSince.has_value();
                if (automaticExecutionsInfo.AutomaticExecutionsPrepaidSince.has_value()) {
                    builder << "automaticExecutionsPrepaidSince" << static_cast<int64_t>(automaticExecutionsInfo.AutomaticExecutionsPrepaidSince->unwrap());
                }
            builder << bson_stream::close_document;
        }

        void StreamServicePayments(bson_stream::document& builder, const std::vector<state::ServicePayment>& servicePayments) {
            auto array = builder << "servicePayments" << bson_stream::open_array;
            for (const auto& payment : servicePayments) {
                array
                    << bson_stream::open_document
                    << "mosaicId" << ToInt64(payment.MosaicId)
                    << "amount" << ToInt64(payment.Amount)
                    << bson_stream::close_document;
            }
            array << bson_stream::close_array;
        }

        void StreamContractCalls(bson_stream::document& builder, const std::deque<state::ContractCall>& requestedCalls) {
            auto array = builder << "requestedCalls" << bson_stream::open_array;
            for (const auto& request : requestedCalls) {
                auto pFileName = reinterpret_cast<const uint8_t*>(request.FileName.c_str());
                auto pFunctionName = reinterpret_cast<const uint8_t*>(request.FunctionName.c_str());
                auto pActualArguments = reinterpret_cast<const uint8_t*>(request.ActualArguments.c_str());
                bson_stream::document requestedCallsBuilder;
                requestedCallsBuilder
                    << "callId" << ToBinary(request.CallId)
                    << "caller" << ToBinary(request.Caller)
                    << "fileName" << ToBinary(pFileName, request.FileName.size())
                    << "functionName" << ToBinary(pFunctionName, request.FunctionName.size())
                    << "actualArguments" << ToBinary(pActualArguments, request.ActualArguments.size())
                    << "executionCallPayment" << ToInt64(request.ExecutionCallPayment)
                    << "downloadCallPayment" << ToInt64(request.DownloadCallPayment);
                StreamServicePayments(requestedCallsBuilder, request.ServicePayments);
                requestedCallsBuilder
                    << "blockHeight" << ToInt64(request.BlockHeight);
                array << requestedCallsBuilder;
            }
            array << bson_stream::close_array;
        }

        void StreamProofOfExecution(bson_stream::document& builder, const state::ProofOfExecution& poEx) {
            builder << "poEx" << bson_stream::open_document
                    << "startBatchId" << static_cast<int64_t>(poEx.StartBatchId);
            auto T = poEx.T.toBytes();
            builder << "T" << ToBinary(T.data(), T.size())
                    << "R" << ToBinary(poEx.R) << bson_stream::close_document;
        }

        void StreamExecutorsInfo(bson_stream::document& builder, const std::map<Key, state::ExecutorInfo>& executorsInfo) {
            auto array = builder << "executorsInfo" << bson_stream::open_array;
            for (const auto& info : executorsInfo) {
                bson_stream::document executorInfoBuilder;
                executorInfoBuilder 
                    << "replicatorKey" << ToBinary(info.first)
                    << "nextBatchToApprove" << static_cast<int64_t>(info.second.NextBatchToApprove);
                StreamProofOfExecution(executorInfoBuilder, info.second.PoEx);
                array << executorInfoBuilder;
            }
            array << bson_stream::close_array;
        }

        void StreamCompletedCalls(bson_stream::document& builder, const std::vector<state::CompletedCall>& completedCalls) {
            auto array = builder << "completedCalls" << bson_stream::open_array;
            for (const auto& completed : completedCalls) {
                array << "callId" << ToBinary(completed.CallId)
                    << "caller" << ToBinary(completed.Caller)
                    << "status" << static_cast<int16_t>(completed.Status)
                    << "executionWork" << ToInt64(completed.ExecutionWork)
                    << "downloadWork" << ToInt64(completed.DownloadWork);
            }
            array << bson_stream::close_array;
        }

        void StreamBatches(bson_stream::document& builder, const std::map<uint64_t, state::Batch>& batches) {
            auto array = builder << "batches" << bson_stream::open_array;
            for (const auto& batch : batches) {
                bson_stream::document batchBuilder;
                batchBuilder << "batchId" << static_cast<int64_t>(batch.first)
                            << "success" << batch.second.Success;
                auto poExVerificationInformation = batch.second.PoExVerificationInformation.toBytes();
                batchBuilder << "poExVerificationInformation" << ToBinary(poExVerificationInformation.data(), poExVerificationInformation.size());
                StreamCompletedCalls(batchBuilder, batch.second.CompletedCalls);
            }
            array << bson_stream::close_array;
        }

        void StreamReleasedTransactions(bson_stream::document& builder, const std::multiset<Hash256>& releasedTransactions) {
            auto array = builder << "releasedTransactions" << bson_stream::open_array;
            for (const auto& releasedTransaction : releasedTransactions) {
                array << ToBinary(releasedTransaction);
            }
            array << bson_stream::close_array;
        }
    }

    bsoncxx::document::value ToDbModel(const state::SuperContractEntry& entry, const Address& accountAddress) {
        bson_stream::document builder;
        auto doc = builder
            << "supercontract" << bson_stream::open_document
            << "multisig" << ToBinary(entry.key())
            << "multisigAddress" << ToBinary(accountAddress)
            << "driveKey" << ToBinary(entry.driveKey())
            << "executionPaymentKey" << ToBinary(entry.executionPaymentKey())
            << "assignee" << ToBinary(entry.assignee());

        StreamAutomaticExecutionsInfo(builder, entry.automaticExecutionsInfo());
        StreamContractCalls(builder, entry.requestedCalls());
        StreamExecutorsInfo(builder, entry.executorsInfo());
        StreamBatches(builder, entry.batches());
        StreamReleasedTransactions(builder, entry.releasedTransactions());

        return doc
            << bson_stream::close_document
            << bson_stream::finalize;
    }

    // endregion

    // region ToModel

    namespace {
        void ReadAutomaticExecutionsInfo(state::AutomaticExecutionsInfo& automaticExecutionsInfo, const bsoncxx::document::view& dbAutomaticExecutionsInfo) {
            auto binaryAutomaticExecutionFileName = dbAutomaticExecutionsInfo["automaticExecutionFileName"].get_binary();
            std::string automaticExecutionFileName(reinterpret_cast<const char*>(binaryAutomaticExecutionFileName.bytes, binaryAutomaticExecutionFileName.size));
            auto binaryAutomaticExecutionsFunctionName = dbAutomaticExecutionsInfo["automaticExecutionsFunctionName"].get_binary();
            std::string automaticExecutionsFunctionName(reinterpret_cast<const char*>(binaryAutomaticExecutionsFunctionName.bytes, binaryAutomaticExecutionsFunctionName.size));

            automaticExecutionsInfo.AutomaticExecutionFileName = automaticExecutionFileName;
            automaticExecutionsInfo.AutomaticExecutionsFunctionName = automaticExecutionsFunctionName;
            automaticExecutionsInfo.AutomaticExecutionsNextBlockToCheck =  Height(dbAutomaticExecutionsInfo["automaticExecutionsNextBlockToCheck"].get_int64());
            automaticExecutionsInfo.AutomaticExecutionCallPayment = Amount(static_cast<uint64_t>(dbAutomaticExecutionsInfo["automaticExecutionCallPayment"].get_int64()));
            automaticExecutionsInfo.AutomaticDownloadCallPayment = Amount(static_cast<uint64_t>(dbAutomaticExecutionsInfo["automaticDownloadCallPayment"].get_int64()));
            automaticExecutionsInfo.AutomatedExecutionsNumber = dbAutomaticExecutionsInfo["automatedExecutionsNumber"].get_int32();
            
            auto AutomaticExecutionsPrepaidSinceHasValue = dbAutomaticExecutionsInfo["AutomaticExecutionsPrepaidSince"];
            if (AutomaticExecutionsPrepaidSinceHasValue) {
                automaticExecutionsInfo.AutomaticExecutionsPrepaidSince = Height(AutomaticExecutionsPrepaidSinceHasValue.get_int64());
            }
        }

        auto ReadServicePayments(std::vector<state::ServicePayment>& servicePayments, const bsoncxx::array::view& dbServicePayments) {
            for (const auto& dbServicePayment : dbServicePayments) {
                auto mosaicId = UnresolvedMosaicId{static_cast<uint64_t>(dbServicePayment["mosaicId"].get_int64())};
                auto amount = Amount(static_cast<uint64_t>(dbServicePayment["amount"].get_int64()));
                servicePayments.emplace_back(state::ServicePayment{mosaicId, amount});
            }
        }

        void ReadContractCalls(std::deque<state::ContractCall>& requestedCalls, const bsoncxx::array::view& dbRequestedCalls) {
            for (const auto& dbRequestedCall : dbRequestedCalls) {
                auto doc = dbRequestedCall.get_document().view();
                Hash256 callId;
                DbBinaryToModelArray(callId, doc["callId"].get_binary());
                Key caller;
                DbBinaryToModelArray(caller, doc["caller"].get_binary());
                auto binaryFileName = doc["fileName"].get_binary();
                std::string fileName(reinterpret_cast<const char*>(binaryFileName.bytes, binaryFileName.size));
                auto binaryFunctionName = doc["functionName"].get_binary();
                std::string functionName(reinterpret_cast<const char*>(binaryFunctionName.bytes, binaryFunctionName.size));
                auto binaryActualArguments = doc["actualArguments"].get_binary();
                std::string actualArguments(reinterpret_cast<const char*>(binaryActualArguments.bytes, binaryActualArguments.size));
                auto executionCallPayment = Amount(static_cast<uint64_t>(doc["executionCallPayment"].get_int64()));
                auto downloadCallPayment = Amount(static_cast<uint64_t>(doc["downloadCallPayment"].get_int64()));
                std::vector<state::ServicePayment> servicePayments;
                ReadServicePayments(servicePayments, doc["servicePayments"].get_array().value);
                auto blockHeight = Height(doc["blockHeight"].get_int64());

                requestedCalls.emplace_back(state::ContractCall{callId, caller, fileName, functionName, actualArguments, executionCallPayment, downloadCallPayment, servicePayments, blockHeight});
            }
        }

        void ReadProofOfExecution(state::ProofOfExecution& poEx, const bsoncxx::document::view& dbProofOfExecution) {
            poEx.StartBatchId = static_cast<int64_t>(dbProofOfExecution["startBatchId"].get_int64());
            // Key is used to read the CurvePoint datatype because it's just a wrapper over std array without any specific logic
            Key buffer; 
            DbBinaryToModelArray(buffer, dbProofOfExecution["T"].get_binary());
            poEx.T.fromBytes(buffer.array());
            DbBinaryToModelArray(poEx.R, dbProofOfExecution["R"].get_binary());
        }

        void ReadExecutorsInfo(std::map<Key, state::ExecutorInfo>& executorsInfo, const bsoncxx::array::view& dbExecutorsInfo) {
            for (const auto& info : dbExecutorsInfo) {
                auto doc = info.get_document().view();
                Key replicatorKey;
                DbBinaryToModelArray(replicatorKey, doc["replicatorKey"].get_binary());
                auto nextBatchToApprove = static_cast<uint64_t>(doc["nextBatchToApprove"].get_int64());
                state::ProofOfExecution poEx;
                ReadProofOfExecution(poEx, doc["poEx"].get_value().get_document());
                executorsInfo.emplace(replicatorKey, state::ExecutorInfo{nextBatchToApprove, poEx});
            }
        }

        void ReadCompletedCalls(std::vector<state::CompletedCall>& completedCalls, const bsoncxx::array::view& dbCompletedCalls) {
            for (const auto& completed : dbCompletedCalls) {
                auto doc = completed.get_document().view();
                Hash256 callId;
                DbBinaryToModelArray(callId, doc["callId"].get_binary());
                Key caller;
                DbBinaryToModelArray(caller, doc["caller"].get_binary());
                auto status = doc["status"].get_int32();
                auto executionWork = Amount(static_cast<uint64_t>(doc["executionWork"].get_int64()));
                auto downloadWork = Amount(static_cast<uint64_t>(doc["downloadWork"].get_int64()));
                completedCalls.emplace_back(state::CompletedCall{callId, caller, status, executionWork, downloadWork});
            }
        }

        void ReadBatches(std::map<uint64_t, state::Batch>& batches, const bsoncxx::array::view& dbBatches) {
            for (const auto& batch : dbBatches) {
                auto doc = batch.get_document().view();
                auto batchId = doc["batchId"].get_int64();
                auto success = doc["success"].get_bool();
                Key buffer; 
                DbBinaryToModelArray(buffer, doc["poExVerificationInformation"].get_binary());
                crypto::CurvePoint poExVerificationInformation;   
                poExVerificationInformation.fromBytes(buffer.array());      
                std::vector<state::CompletedCall> completedCalls;
                ReadCompletedCalls(completedCalls, doc["completedCalls"].get_array().value);
                batches.emplace(batchId, state::Batch{success, poExVerificationInformation, completedCalls});
            }
        }

        void ReadReleasedTransactions(std::multiset<Hash256>& releasedTransactions, const bsoncxx::array::view& dbReleasedTransactions) {
            for (const auto& dbReleasedTransaction : dbReleasedTransactions) {            
                Hash256 releasedTransaction;
                DbBinaryToModelArray(releasedTransaction, dbReleasedTransaction.get_binary());
                releasedTransactions.emplace(std::move(releasedTransaction));
            }
        }
    }

    state::SuperContractEntry ToSuperContractEntry(const bsoncxx::document::view& document) {
        auto dbSuperContractEntry = document["supercontract"];
        Key multisig;
        DbBinaryToModelArray(multisig, dbSuperContractEntry["multisig"].get_binary());
        state::SuperContractEntry entry(multisig);

        Key driveKey;
        DbBinaryToModelArray(driveKey, dbSuperContractEntry["driveKey"].get_binary());
        entry.setDriveKey(driveKey);

        Key executionPaymentKey;
        DbBinaryToModelArray(executionPaymentKey, dbSuperContractEntry["executionPaymentKey"].get_binary());
        entry.setExecutionPaymentKey(executionPaymentKey);

        Key assignee;
        DbBinaryToModelArray(assignee, dbSuperContractEntry["assignee"].get_binary());
        entry.setAssignee(assignee);

        ReadAutomaticExecutionsInfo(entry.automaticExecutionsInfo(), dbSuperContractEntry["automaticExecutionsInfo"].get_value().get_document());
        ReadContractCalls(entry.requestedCalls(), dbSuperContractEntry["requestedCalls"].get_array().value);
        ReadExecutorsInfo(entry.executorsInfo(), dbSuperContractEntry["executorsInfo"].get_array().value);
        ReadBatches(entry.batches(), dbSuperContractEntry["batches"].get_array().value);
        ReadReleasedTransactions(entry.releasedTransactions(), dbSuperContractEntry["releasedTransactions"].get_array().value);        
    }

    // endregion
}}}