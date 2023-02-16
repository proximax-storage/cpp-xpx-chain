/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "SuperContractEntrySerializer.h"
#include "catapult/io/PodIoUtils.h"
#include "catapult/utils/Casting.h"

namespace catapult { namespace state {

    namespace {
        void SaveAutomaticExecutionsInfo(const AutomaticExecutionsInfo& automaticExecutionsInfo, io::OutputStream& output) {
            io::Write16(output, utils::checked_cast<size_t, uint16_t>(automaticExecutionsInfo.AutomaticExecutionFileName.size()));
            auto pAutomaticExecutionFileName = reinterpret_cast<const uint8_t*>(automaticExecutionsInfo.AutomaticExecutionFileName.c_str());
            io::Write(output, utils::RawBuffer(pAutomaticExecutionFileName, automaticExecutionsInfo.AutomaticExecutionFileName.size()));
            io::Write16(output, utils::checked_cast<size_t, uint16_t>(automaticExecutionsInfo.AutomaticExecutionsFunctionName.size()));
            auto pAutomaticExecutionsFunctionName = reinterpret_cast<const uint8_t*>(automaticExecutionsInfo.AutomaticExecutionsFunctionName.c_str());
            io::Write(output, utils::RawBuffer(pAutomaticExecutionsFunctionName, automaticExecutionsInfo.AutomaticExecutionsFunctionName.size()));
            io::Write(output, automaticExecutionsInfo.AutomaticExecutionsNextBlockToCheck);
            io::Write(output, automaticExecutionsInfo.AutomaticExecutionCallPayment);
            io::Write(output, automaticExecutionsInfo.AutomaticDownloadCallPayment);
            io::Write32(output, automaticExecutionsInfo.AutomatedExecutionsNumber);
            io::Write8(output, automaticExecutionsInfo.AutomaticExecutionsPrepaidSince.has_value());
            if (automaticExecutionsInfo.AutomaticExecutionsPrepaidSince.has_value()) {
                io::Write(output, *automaticExecutionsInfo.AutomaticExecutionsPrepaidSince);
            }
        }

        void SaveServicePayments(const std::vector<ServicePayment>& servicePayments, io::OutputStream& output) {
            io::Write16(output, utils::checked_cast<size_t, uint16_t>(servicePayments.size()));
            for (const auto& servicePayment : servicePayments) {
                io::Write(output, servicePayment.MosaicId);
                io::Write(output, servicePayment.Amount);
            }
        }

        void SaveContractCalls(const std::deque<ContractCall>& contractCalls, io::OutputStream& output) {
            io::Write16(output, utils::checked_cast<size_t, uint16_t>(contractCalls.size()));
            for (const auto& contractCall : contractCalls) {
                io::Write(output, contractCall.CallId);
                io::Write(output, contractCall.Caller);
                io::Write16(output, utils::checked_cast<size_t, uint16_t>(contractCall.FileName.size()));
                auto pFileName = reinterpret_cast<const uint8_t*>(contractCall.FileName.c_str());
                io::Write(output, utils::RawBuffer(pFileName, contractCall.FileName.size()));
                io::Write16(output, utils::checked_cast<size_t, uint16_t>(contractCall.FunctionName.size()));
                auto pFunctionName = reinterpret_cast<const uint8_t*>(contractCall.FunctionName.c_str());
                io::Write(output, utils::RawBuffer(pFunctionName, contractCall.FunctionName.size()));
                io::Write16(output, utils::checked_cast<size_t, uint16_t>(contractCall.ActualArguments.size()));
                auto pActualArguments = reinterpret_cast<const uint8_t*>(contractCall.ActualArguments.c_str());
                io::Write(output, utils::RawBuffer(pActualArguments, contractCall.ActualArguments.size()));
                io::Write(output, contractCall.ExecutionCallPayment);
                io::Write(output, contractCall.DownloadCallPayment);
                SaveServicePayments(contractCall.ServicePayments, output);
                io::Write(output, contractCall.BlockHeight);  
            }
        }

        void SaveProofOfExecution(const ProofOfExecution& poEx, io::OutputStream& output) {
            io::Write64(output, poEx.StartBatchId);
            io::Write(output, poEx.T.toBytes());
            io::Write(output, poEx.R);
        }

        void SaveExecutorsInfo(const std::map<Key, ExecutorInfo>& executorsInfo, io::OutputStream& output) {
            io::Write16(output, utils::checked_cast<size_t, uint16_t>(executorsInfo.size()));
            for (const auto& info : executorsInfo) {
				io::Write(output, info.first);
				io::Write64(output, info.second.NextBatchToApprove);
				SaveProofOfExecution(info.second.PoEx, output);
			}
        }

        void SaveCompletedCalls(const std::vector<CompletedCall>& completedCalls, io::OutputStream& output) {
            io::Write16(output, utils::checked_cast<size_t, uint16_t>(completedCalls.size()));
            for (const auto& completedCall : completedCalls) {
                io::Write(output, completedCall.CallId);
                io::Write(output, completedCall.Caller);
                std::memcpy(&output, &completedCall.Status, sizeof(int16_t));
                io::Write(output, completedCall.ExecutionWork);
                io::Write(output, completedCall.DownloadWork);
            }
        }

        void SaveBatches(const std::vector<Batch>& batches, io::OutputStream& output) {
            io::Write16(output, utils::checked_cast<size_t, uint16_t>(batches.size()));
            for (const auto& batch : batches) {
                io::Write8(output, batch.Success);
                io::Write(output, batch.PoExVerificationInformation.toBytes());
                SaveCompletedCalls(batch.CompletedCalls, output);
            }
        }
    }

    void SuperContractEntrySerializer::Save(const SuperContractEntry& entry, io::OutputStream& output) {
        // write version
		io::Write32(output, 1);

        io::Write(output, entry.key());
        io::Write(output, entry.driveKey());
        io::Write(output, entry.executionPaymentKey());
        io::Write(output, entry.assignee());
        SaveAutomaticExecutionsInfo(entry.automaticExecutionsInfo(), output);
        SaveContractCalls(entry.requestedCalls(), output);
        SaveExecutorsInfo(entry.executorsInfo(), output);
        SaveBatches(entry.batches(), output);

        io::Write16(output, entry.releasedTransactions().size());
        for (const auto& releasedTransaction : entry.releasedTransactions()) {
            io::Write(output, releasedTransaction);
        }
    }

    namespace {
        void LoadAutomaticExecutionsInfo(AutomaticExecutionsInfo& automaticExecutionsInfo, io::InputStream& input) {
            auto automaticExecutionFileNameSize = io::Read16(input);
            std::vector<uint8_t> automaticExecutionFileNameBytes(automaticExecutionFileNameSize);
            io::Read(input, automaticExecutionFileNameBytes);
            std::string automaticExecutionFileName(automaticExecutionFileNameBytes.begin(), automaticExecutionFileNameBytes.end());           
            auto automaticExecutionsFunctionNameSize = io::Read16(input);
            std::vector<uint8_t> automaticExecutionsFunctionNameBytes(automaticExecutionsFunctionNameSize);
            io::Read(input, automaticExecutionsFunctionNameBytes);
            std::string automaticExecutionsFunctionName(automaticExecutionsFunctionNameBytes.begin(), automaticExecutionsFunctionNameBytes.end());

            automaticExecutionsInfo.AutomaticExecutionFileName = automaticExecutionFileName;
            automaticExecutionsInfo.AutomaticExecutionsFunctionName = automaticExecutionsFunctionName;
            automaticExecutionsInfo.AutomaticExecutionsNextBlockToCheck = io::Read<Height>(input);
            automaticExecutionsInfo.AutomaticExecutionCallPayment = io::Read<Amount>(input);
            automaticExecutionsInfo.AutomaticDownloadCallPayment = io::Read<Amount>(input);
            automaticExecutionsInfo.AutomatedExecutionsNumber = io::Read32(input);

            bool hasAutomaticExecutionsPrepaidSince = io::Read8(input);
            if (hasAutomaticExecutionsPrepaidSince) {
                automaticExecutionsInfo.AutomaticExecutionsPrepaidSince = io::Read<Height>(input);
            }
        }

        void LoadServicePayments(std::vector<ServicePayment>& servicePayments, io::InputStream& input) {
            auto servicePaymentsCount = io::Read16(input);
            while (servicePaymentsCount--) {
                ServicePayment servicePayment;
                io::Read(input, servicePayment.MosaicId);
                io::Read(input, servicePayment.Amount);

                servicePayments.emplace_back(servicePayment);
            }
        }

        void LoadContractCalls(std::deque<ContractCall>& contractCalls, io::InputStream& input) {
            auto contractCallsCount = io::Read16(input);
            while (contractCallsCount--) {
                ContractCall contractCall;
                io::Read(input, contractCall.CallId);
                io::Read(input, contractCall.Caller);

                auto fileNameSize = io::Read16(input);
                std::vector<uint8_t> fileNameBytes(fileNameSize);
                io::Read(input, fileNameBytes);
                std::string fileName(fileNameBytes.begin(), fileNameBytes.end());
                auto functionNameSize = io::Read16(input);
                std::vector<uint8_t> functionNameBytes(functionNameSize);
                io::Read(input, functionNameBytes);
                std::string functionName(functionNameBytes.begin(), functionNameBytes.end());
                auto actualArgumentsSize = io::Read16(input);
                std::vector<uint8_t> actualArgumentsBytes(actualArgumentsSize);
                io::Read(input, actualArgumentsBytes);
                std::string actualArguments(actualArgumentsBytes.begin(), actualArgumentsBytes.end());

                contractCall.FileName = fileName;
                contractCall.FunctionName = functionName;
                contractCall.ActualArguments = actualArguments;

                io::Read(input, contractCall.ExecutionCallPayment);
                io::Read(input, contractCall.DownloadCallPayment);
                LoadServicePayments(contractCall.ServicePayments, input);
                io::Read(input, contractCall.BlockHeight);

                contractCalls.emplace_back(contractCall);
			}
        }

        void LoadProofOfExecution(ProofOfExecution& poEx, io::InputStream& input) {
            poEx.StartBatchId = io::Read64(input);
            std::array<uint8_t, 32> tBuffer;
            io::Read(input, tBuffer);
            poEx.T.fromBytes(tBuffer);
            io::Read(input, poEx.R);
        }

        void LoadExecutorsInfo(std::map<Key, ExecutorInfo>& executorsInfo, io::InputStream& input) {
            auto executorsInfoCount = io::Read16(input);
            while (executorsInfoCount--) {
                Key key;
                input.read(key);
                ExecutorInfo info;
                info.NextBatchToApprove = io::Read64(input);
                LoadProofOfExecution(info.PoEx, input);
                executorsInfo.emplace(key, info);
            }
        }

        void LoadCompletedCalls(std::vector<CompletedCall>& completedCalls, io::InputStream& input) {
            auto completedCallsCount = io::Read16(input);
            while (completedCallsCount--) {
                CompletedCall completedCall;
                io::Read(input, completedCall.CallId);
                io::Read(input, completedCall.Caller);
                std::memcpy(&input, &completedCall.Status, sizeof(int16_t));
                io::Read(input, completedCall.ExecutionWork);
                io::Read(input, completedCall.DownloadWork);

                completedCalls.emplace_back(completedCall);
            }
        }

        void LoadBatches(std::vector<Batch>& batches, io::InputStream& input) {
            auto batchesCount = io::Read16(input);
            while (batchesCount--) {
                Batch batch;
                batch.Success = io::Read8(input);
                std::array<uint8_t, 32> tBuffer;
                io::Read(input, tBuffer);
                batch.PoExVerificationInformation.fromBytes(tBuffer);
                LoadCompletedCalls(batch.CompletedCalls, input);
                batches.emplace_back(batch);
            }
        }
    }

    SuperContractEntry SuperContractEntrySerializer::Load(io::InputStream& input) {
        // read version
		VersionType version = io::Read32(input);
		if (version > 1)
			CATAPULT_THROW_RUNTIME_ERROR_1("invalid version of SuperContractEntry", version);

        Key key;
		input.read(key);
		state::SuperContractEntry entry(key);

        Key driveKey;
        input.read(driveKey);
        entry.setDriveKey(driveKey);

        Key executionPaymentKey;
		input.read(executionPaymentKey);
        entry.setExecutionPaymentKey(executionPaymentKey);

        Key assignee;
		input.read(assignee);
        entry.setAssignee(assignee);

        LoadAutomaticExecutionsInfo(entry.automaticExecutionsInfo(), input);
        LoadContractCalls(entry.requestedCalls(), input);
        LoadExecutorsInfo(entry.executorsInfo(), input);
        LoadBatches(entry.batches(), input);

        auto count = io::Read16(input);
        while (count--) {
            Hash256 releasedTransaction;
            io::Read(input, releasedTransaction);
            entry.releasedTransactions().emplace(releasedTransaction);
        }
    }
}}