/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/types.h"
#include "catapult/exceptions.h"

namespace catapult { namespace state {

    // Cache data entry.
	template<typename TCacheEntry>
    class CacheDataEntry {
    public:
        explicit CacheDataEntry(VersionType version) : m_version(version) {
            if (m_version > TCacheEntry::MaxVersion) {
                std::string name{typeid(this).name()};
                CATAPULT_THROW_INVALID_ARGUMENT_2("invalid version of ", name, m_version);
            }
        }

    public:
        constexpr VersionType getVersion() const {
            return m_version;
        }

    protected:
        VersionType m_version;
    };
}}
