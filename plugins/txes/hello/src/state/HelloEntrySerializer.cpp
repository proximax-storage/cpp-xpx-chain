//
// Created by ruell on 08/10/2019.
//

#include "HelloEntrySerializer.h"
#include "catapult/io/PodIoUtils.h"
#include "catapult/exceptions.h"

namespace catapult { namespace state {

        void HelloEntrySerializer::Save(const HelloEntry& entry, io::OutputStream& output) {
            // write version
            io::Write32(output, 1);

            io::Write(output, entry.key());
            io::Write16(output, entry.messageCount());
        }

        HelloEntry HelloEntrySerializer::Load(io::InputStream& input) {
            // read version
            VersionType version = io::Read32(input);
            if (version > 1)
            CATAPULT_THROW_RUNTIME_ERROR_1("invalid version of HelloEntry", version);

            Key key;
            input.read(key);
            auto messageCount = io::Read16(input);

            return state::HelloEntry(key, messageCount);
        }
    }}
