/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/


#include "GlobalEntry.h"
#include "catapult/model/EntityType.h"

namespace catapult { namespace state {
		GlobalEntry::GlobalEntry(const Hash256& key, const std::vector<uint8_t>& data)
		{
			Key = key;
			Data = data;

		}
		std::vector<uint8_t> GlobalEntry::Get() const
		{
			return Data;
		}

		std::vector<uint8_t>& GlobalEntry::Ref()
		{
			return Data;
		}

		const std::vector<uint8_t>& GlobalEntry::Ref() const
		{
			return Data;
		}

		const Hash256& GlobalEntry::GetKey() const
		{
			return Key;
		}

}}
