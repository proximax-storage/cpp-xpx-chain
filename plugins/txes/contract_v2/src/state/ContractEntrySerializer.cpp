/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "ContractEntrySerializer.h"
#include "catapult/io/PodIoUtils.h"
#include "catapult/exceptions.h"

namespace catapult { namespace state {

    void ContractEntrySerializer::Save(const ContractEntry& entry, io::OutputStream& output) {
        // write version
		io::Write32(output, 1);

        io::Write(output, entry.key());
        io::Write(output, entry.owner());
        io::Write16(output, (uint16_t) entry.automatedExecutionFileName().size());
        io::Write16(output, (uint16_t) entry.automatedExecutionFunctionName().size());
        io::Write(output, entry.automatedExecutionCallPayment());
        io::Write(output, entry.automatedDownloadCallPayment());
        io::Write32(output, entry.automatedExecutionsNumber());
        io::Write(output, entry.assignee());
    }

    ContractEntry ContractEntrySerializer::Load(io::InputStream& input) {
        // read version
		VersionType version = io::Read32(input);
		if (version > 1)
			CATAPULT_THROW_RUNTIME_ERROR_1("invalid version of ContractEntry", version);

        Key key;
		input.read(key);
		state::ContractEntry entry(key);

        Key owner;
		input.read(owner);
		entry.setOwner(owner);

        auto automatedExecutionFileNameSize = io::Read16(input);
        std::vector<uint8_t> automatedExecutionFileNameBytes(automatedExecutionFileNameSize);
        io::Read(input, automatedExecutionFileNameBytes);
		std::string automatedExecutionFileName(automatedExecutionFileNameBytes.begin(), automatedExecutionFileNameBytes.end());
        
        auto automatedExecutionFunctionNameSize = io::Read16(input);
        std::vector<uint8_t> automatedExecutionFunctionNameBytes(automatedExecutionFunctionNameSize);
        io::Read(input, automatedExecutionFunctionNameBytes);
		std::string automatedExecutionFunctionName(automatedExecutionFunctionNameBytes.begin(), automatedExecutionFunctionNameBytes.end());

        auto automatedExecutionCallPayment = io::Read64(input);
        auto automatedDownloadCallPayment = io::Read64(input);
        auto automatedExecutionsNumber = io::Read32(input);

        Key assignee;
		input.read(assignee);
		entry.setAssignee(assignee);
    }
}}