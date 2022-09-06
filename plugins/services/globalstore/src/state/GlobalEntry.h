/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/


#pragma once
#include "catapult/types.h"

namespace catapult { namespace state {


	/// Global entry
	class GlobalEntry {
	public:
		template<typename TConverter>
		GlobalEntry(const Hash256& key, const typename TConverter::ValueType& data, TConverter)
		{
			Key = key;
			Data = TConverter::Convert(data);

		}

		GlobalEntry(const Hash256& key, const std::vector<uint8_t>& data);

	public:

		std::vector<uint8_t> Get() const;

		std::vector<uint8_t>& Ref();

		const std::vector<uint8_t>& Ref() const;

		const Hash256& GetKey() const;

		template<typename TConverter>
		typename TConverter::ValueType Get()
		{
			return TConverter::Convert(Data);
		}

		template<typename TConverter>
		void Set(const typename TConverter::ValueType& value)
		{
			Data = TConverter::Convert(Data);
		}

		template<typename TConverter, typename TEnable = std::enable_if_t<TConverter::SupportRef>>
		typename TConverter::ValueType& GetRef()
		{
			return TConverter::Convert(Data.data());
		}
	private:
		Hash256 Key;
		std::vector<uint8_t> Data;
	};
}}
