/**
*** FOR TRAINING PURPOSES ONLY
**/

#pragma once
#include "HelloEntry.h"
#include "catapult/io/Stream.h"

namespace catapult { namespace state {

        /// Policy for saving and loading catapult upgrade entry data.
        struct HelloEntrySerializer {
            /// Saves \a entry to \a output.
            static void Save(const HelloEntry& entry, io::OutputStream& output);

            /// Loads a single value from \a input.
            static HelloEntry Load(io::InputStream& input);
        };
    }}

