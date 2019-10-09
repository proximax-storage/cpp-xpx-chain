/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once

namespace catapult { namespace model {

#pragma pack(push, 1)

    /// Binary layout for a file.
    struct File {
    public:
        /// Hash of file.
        Hash256 FileHash;
    };

    /// Binary layout for a remove action.
    struct RemoveAction : public File {
    };

    /// Binary layout for an add action.
    struct AddAction : public File {
    public:
        /// Size of file.
        uint64_t FileSize;
    };

#pragma pack(pop)
}}