/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/utils/Casting.h"
#include "catapult/io/PodIoUtils.h"
#include "catapult/io/Stream.h"

namespace catapult { namespace state {

	struct InflationMapConverter {
		using ValueType = std::map<Height, Amount>;
		static constexpr bool SupportRef = false;

		static std::vector<uint8_t> Convert(const ValueType& value) {
			std::vector<uint8_t> result(value.size()*8*2);
			auto offset = 0;
			for(auto& kv : value) {
				*reinterpret_cast<uint64_t*>(result.data()+offset) = kv.first.unwrap();
				offset += 8;
				*reinterpret_cast<uint64_t*>(result.data()+offset) = kv.second.unwrap();
				offset += 8;
			}
			return result;
		}

		static ValueType Convert(const std::vector<uint8_t>& value) {
			ValueType result;
			auto offset = 0;
			while(offset != value.size()) {
				result[Height(*reinterpret_cast<const uint64_t*>(value.data()+offset))] = Amount(*reinterpret_cast<const uint64_t*>(value.data()+offset+8));
				offset += 16;
			}
			return result;
		}
	};
}}
