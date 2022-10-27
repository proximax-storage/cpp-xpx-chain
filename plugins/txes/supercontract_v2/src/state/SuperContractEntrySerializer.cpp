/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "SuperContractEntrySerializer.h"
#include "catapult/io/PodIoUtils.h"
#include "catapult/exceptions.h"

namespace catapult { namespace state {

    namespace {
        void SaveServicePayments(io::OutputStream& output, const std::optional<ServicePayments>& servicePayments) {
            bool hasServicePayments = servicePayments.has_value();
            io::Write8(output, hasServicePayments);
            if (hasServicePayments) {
                io::Write16(output, servicePayments->size());
                for (const auto& servicePayment : *servicePayments) {
                    io::Write(output, servicePayment.Token);
                    io::Write(output, servicePayment.Amount);
                }
            }
           
        }

        void SaveStartExecutionInformation(io::OutputStream& output, const StartExecutionInformation& startExecutionInformation) {
            auto pFileName = (const uint8_t*) (startExecutionInformation.FileName.c_str());
			io::Write(output, utils::RawBuffer(pFileName, startExecutionInformation.FileName.size()));
            auto pFunctionName = (const uint8_t*) (startExecutionInformation.FunctionName.c_str());
			io::Write(output, utils::RawBuffer(pFunctionName, startExecutionInformation.FunctionName.size()));
            auto pActualArguments = (const uint8_t*) (startExecutionInformation.ActualArguments.c_str());
			io::Write(output, utils::RawBuffer(pActualArguments, startExecutionInformation.ActualArguments.size()));
            io::Write(output, startExecutionInformation.ExecutionCallPayment);
            io::Write(output, startExecutionInformation.DownloadCallPayment);
            io::Write(output, startExecutionInformation.SynchronizeCallPayment);
            io::Write16(output, startExecutionInformation.ServicePaymentCount);
            SaveServicePayments(output, startExecutionInformation.ServicePayments);
            io::Write8(output, startExecutionInformation.SingleApprovement);
        }

        void LoadServicePayments(io::InputStream& input, std::optional<ServicePayments>& servicePayments) {
            bool hasServicePayments = io::Read8(input);
            if (hasServicePayments) {
                servicePayments = ServicePayments();
                auto numServicePayments = io::Read16(input);
                while (numServicePayments--) {
                    ServicePayment servicePayment;
                    io::Read(input, servicePayment.Token);
                    io::Read(input, servicePayment.Amount);

                    &servicePayments->emplace_back(servicePayment);
                }
            }      
        }

        void LoadStartExecutionInformation(io::InputStream& input, StartExecutionInformation& startExecutionInformation) {
            auto fileNameSize = io::Read16(input);
            std::vector<uint8_t> fileNameBytes(fileNameSize);
            io::Read(input, fileNameBytes);
            std::string fileName(fileNameBytes.begin(), fileNameBytes.end());
            startExecutionInformation.FileName = fileName;
            
            auto functionNameSize = io::Read16(input);
            std::vector<uint8_t> functionNameBytes(functionNameSize);
            io::Read(input, functionNameBytes);
            std::string functionName(functionNameBytes.begin(), functionNameBytes.end());
            startExecutionInformation.FunctionName = functionName;

            auto actualArgumentsSize = io::Read16(input);
            std::vector<uint8_t> actualArgumentsBytes(actualArgumentsSize);
            io::Read(input, actualArgumentsBytes);
            std::string actualArguments(actualArgumentsBytes.begin(), actualArgumentsBytes.end());
            startExecutionInformation.ActualArguments = actualArguments;

            startExecutionInformation.ExecutionCallPayment = io::Read<Amount>(input);
            startExecutionInformation.DownloadCallPayment = io::Read<Amount>(input);
            startExecutionInformation.SynchronizeCallPayment = io::Read<Amount>(input);
            startExecutionInformation.ServicePaymentCount = io::Read16(input);
            LoadServicePayments(input, startExecutionInformation.ServicePayments);
            startExecutionInformation.SingleApprovement = io::Read8(input);
        }
    }

    void SuperContractEntrySerializer::Save(const SuperContractEntry& entry, io::OutputStream& output) {
        // write version
		io::Write32(output, 1);

        io::Write(output, entry.key());
        io::Write(output, entry.owner());

        auto pAutomatedExecutionFileName = (const uint8_t*) (entry.automatedExecutionFileName().c_str());
		io::Write(output, utils::RawBuffer(pAutomatedExecutionFileName, entry.automatedExecutionFileName().size()));
        auto pAutomatedExecutionFunctionName = (const uint8_t*) (entry.automatedExecutionFunctionName().c_str());
		io::Write(output, utils::RawBuffer(pAutomatedExecutionFunctionName, entry.automatedExecutionFunctionName().size()));

        io::Write(output, entry.automatedExecutionCallPayment());
        io::Write(output, entry.automatedDownloadCallPayment());
        io::Write(output, entry.automatedSynchronizeCallPayment());
        io::Write32(output, entry.automatedExecutionsNumber());
        io::Write(output, entry.assignee());
        io::Write16(output, entry.executorCount());
        SaveStartExecutionInformation(output, entry.startExecutionInformation());

        bool hasAutomaticExecutionsEnabledSince = entry.automaticExecutionsEnabledSince().has_value();
        io::Write8(output, hasAutomaticExecutionsEnabledSince);
        if (hasAutomaticExecutionsEnabledSince) {
            io::Write64(output, *entry.automaticExecutionsEnabledSince());
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

        Key owner;
		input.read(owner);
		entry.setOwner(owner);

        auto automatedExecutionFileNameSize = io::Read16(input);
        std::vector<uint8_t> automatedExecutionFileNameBytes(automatedExecutionFileNameSize);
        io::Read(input, automatedExecutionFileNameBytes);
		std::string automatedExecutionFileName(automatedExecutionFileNameBytes.begin(), automatedExecutionFileNameBytes.end());
        entry.setAutomatedExecutionFileName(automatedExecutionFileName);
        
        auto automatedExecutionFunctionNameSize = io::Read16(input);
        std::vector<uint8_t> automatedExecutionFunctionNameBytes(automatedExecutionFunctionNameSize);
        io::Read(input, automatedExecutionFunctionNameBytes);
		std::string automatedExecutionFunctionName(automatedExecutionFunctionNameBytes.begin(), automatedExecutionFunctionNameBytes.end());
        entry.setAutomatedExecutionFunctionName(automatedExecutionFunctionName);

        entry.setAutomatedExecutionCallPayment(Amount(io::Read64(input)));
        entry.setAutomatedDownloadCallPayment(Amount(io::Read64(input)));
        entry.setAutomatedSynchronizeCallPayment(Amount(io::Read64(input)));

        Key assignee;
		input.read(assignee);
		entry.setAssignee(assignee);

        entry.setExecutorCount(io::Read16(input));
        LoadStartExecutionInformation(input, entry.startExecutionInformation());

        bool hasAutomaticExecutionsEnabledSince = io::Read8(input);
        if (hasAutomaticExecutionsEnabledSince) {
            *entry.automaticExecutionsEnabledSince() = io::Read64(input);
        }
    }
}}