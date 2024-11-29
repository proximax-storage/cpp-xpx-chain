/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include <array>
#include <cstdint>
#include <vector>
#include <algorithm>
#include "catapult/crypto/Hashes.h"
#include "catapult/types.h"
#include "plugins/txes/metadata/src/model/MetadataV1Types.h"

namespace catapult { namespace state {

	/// Converts \a value to a vector.
	template<typename T>
	auto ToVector(T value) {
		std::vector<uint8_t> vec(sizeof(T));
		reinterpret_cast<T&>(*vec.data()) = value;
		return vec;
	}

	/// Converts an array (\a value) to a vector.
	template<size_t N>
	auto ToVector(const std::array<uint8_t, N>& value) {
		return std::vector<uint8_t>(value.cbegin(), value.cend());
	}

	/// Returns hash of MosaicId, namespaceId, Address and etc.
	template<typename T>
	Hash256 GetHash(const T& data, model::MetadataV1Type type) {
		Hash256 result;
		crypto::Sha3_256_Builder sha3;
		sha3.update({
				{ reinterpret_cast<const uint8_t*>(&type), sizeof(uint8_t) },
				data
		});
		sha3.final(result);
		return result;
	}
}}
