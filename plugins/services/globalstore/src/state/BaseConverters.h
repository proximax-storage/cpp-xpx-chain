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

	template<typename TBaseValue>
	struct BaseTypeConverter {
		using ValueType = TBaseValue;
		static constexpr bool SupportRef = true;

		static TBaseValue Convert(const std::vector<uint8_t>& value) {
			return *reinterpret_cast<const TBaseValue*>(value.data());
		}

		static TBaseValue& Convert(uint8_t* value) {
			return *reinterpret_cast<TBaseValue*>(value);
		}

		static std::vector<uint8_t> Convert(const TBaseValue& value) {
			std::vector<uint8_t> result(sizeof(TBaseValue));
			memcpy(result.data(), &value, sizeof(TBaseValue));
			return result;
		}
		static std::vector<uint8_t> Convert(TBaseValue&& value) {
			std::vector<uint8_t> result(sizeof(TBaseValue));
			memcpy(result.data(), &value, sizeof(TBaseValue));
			return result;
		}
	};

	using Uint64Converter = BaseTypeConverter<uint64_t>;
	using PluginInstallConverter = Uint64Converter;

	template<typename TBaseValue>
	struct ByteArrayConverter {
		using ValueType = TBaseValue;
		static constexpr bool SupportRef = false;

		static TBaseValue Convert(const std::vector<uint8_t>& value) {
			TBaseValue result;
			memcpy(result.data(), value.data(), TBaseValue::Size);
			return result;
		}

		static std::vector<uint8_t> Convert(const TBaseValue& value) {
			std::vector<uint8_t> result(sizeof(TBaseValue));
			memcpy(result.data(), value.data(), TBaseValue::Size);
			return result;
		}
	};
}}
