/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "external/statichash/compile_time_sha256.hpp"
#include "catapult/utils/Casting.h"
#include "catapult/io/PodIoUtils.h"
#include "catapult/io/Stream.h"
#include "catapult/crypto/Hashes.h"

namespace catapult { namespace state {


	static constexpr auto GenerateNewGlobalKeyFromLiteral(const char* value){
		auto hashed_value = statichash::SHA256(value);
		std::array<uint8_t, 8*4> result{};
#ifdef COMPILE_SYSTEM_LITTLE_ENDIAN

		for(auto i = 0; i < 8; i++)
		{
			result[i*4] = (hashed_value[i] & 0x000000ff);
			result[i*4+1] = (hashed_value[i]  & 0x0000ff00) >> 8;
			result[i*4+2] = (hashed_value[i]  & 0x00ff0000) >> 16;
			result[i*4+3] = (hashed_value[i]  & 0xff000000) >> 24;
		}

#else
		for(auto i = 0; i < 8; i++) {
			result[i*4+3] = (hashed_value[i] & 0x000000ff);
			result[i*4+2] = (hashed_value[i]  & 0x0000ff00) >> 8;
			result[i*4+1] = (hashed_value[i]  & 0x00ff0000) >> 16;
			result[i*4] = (hashed_value[i]  & 0xff000000) >> 24;
		}
#endif

		return Hash256(result);
	}

	static Hash256 GenerateNewGlobalKey(const std::string& value){
		Hash256 result;
		crypto::Sha256({ reinterpret_cast<const unsigned char*>(value.data()), value.size()}, result);
		return result;
	}
}}
