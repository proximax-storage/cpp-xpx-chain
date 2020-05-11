/**
*** Copyright (c) 2016-present,
*** Jaguar0625, gimre, BloodyRookie, Tech Bureau, Corp. All rights reserved.
***
*** This file is part of Catapult.
***
*** Catapult is free software: you can redistribute it and/or modify
*** it under the terms of the GNU Lesser General Public License as published by
*** the Free Software Foundation, either version 3 of the License, or
*** (at your option) any later version.
***
*** Catapult is distributed in the hope that it will be useful,
*** but WITHOUT ANY WARRANTY; without even the implied warranty of
*** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*** GNU Lesser General Public License for more details.
***
*** You should have received a copy of the GNU Lesser General Public License
*** along with Catapult. If not, see <http://www.gnu.org/licenses/>.
**/

#pragma once
#include "utils/ByteArray.h"
#include "utils/ClampedBaseValue.h"
#include "utils/RawBuffer.h"
#include <array>
#include <limits>
#include <unordered_set>

namespace catapult {
	
	// region Unresolved Data container
	enum class UnresolvedCommonType : uint8_t {
		Default,
		MosaicLevy
	};
	
	struct UnresolvedData {
		virtual ~UnresolvedData(){}
	};
	
	template<typename TUnresolvedInternalType, typename TData>
	struct UnresolvedContainerBase {
		
		constexpr UnresolvedContainerBase()
			: Type(TUnresolvedInternalType::Default)
			, DataPtr(nullptr) {
		}
		
		constexpr explicit UnresolvedContainerBase (const TUnresolvedInternalType& type, const TData* pData)
			: Type(type)
			, DataPtr(pData)
		{}
	
	public:
		TUnresolvedInternalType Type;
		const TData* DataPtr;
	};
	
	template<typename TBaseType, typename TUnresolvedInternalType, typename TData>
	struct UnresolvedValueContainer
		: public TBaseType
			, UnresolvedContainerBase<TUnresolvedInternalType,TData> {
		
		constexpr UnresolvedValueContainer()
			: TBaseType()
			, UnresolvedContainerBase<TUnresolvedInternalType, TData>() {
		}
		
		constexpr UnresolvedValueContainer(const TBaseType& value)
			: TBaseType(value)
			, UnresolvedContainerBase<TUnresolvedInternalType, TData>()
		{}
		
		constexpr explicit UnresolvedValueContainer(uint64_t value)
			: TBaseType(value)
			, UnresolvedContainerBase<TUnresolvedInternalType, TData>()
		{}
		
		/// Creates unresolved container from \a value, \a type and \a data.
		constexpr explicit UnresolvedValueContainer(uint64_t value, const TUnresolvedInternalType& type, const TData* pData = nullptr)
			: TBaseType(value)
			, UnresolvedContainerBase<TUnresolvedInternalType, TData>(type, pData)
		{}
		
		/// Creates unresolved container by copy constructor, \a type and \a data.
		constexpr explicit UnresolvedValueContainer(const TBaseType& value, const TUnresolvedInternalType& type, const TData* pData = nullptr)
			: TBaseType(value)
			, UnresolvedContainerBase<TUnresolvedInternalType, TData>(type, pData)
		{}
	};
	
	template<size_t N, typename TTag, typename TUnresolvedInternalType = UnresolvedCommonType, typename TData = UnresolvedData>
	struct UnresolvedValueArrayContainer
		: public utils::ByteArray<N, TTag>
			, UnresolvedContainerBase<TUnresolvedInternalType,TData> {
		
		constexpr UnresolvedValueArrayContainer()
			: utils::ByteArray<N, TTag>()
			, UnresolvedContainerBase<TUnresolvedInternalType, TData>() {
		}
		
		/// Creates a byte array around \a array.
		constexpr UnresolvedValueArrayContainer(const std::array<uint8_t, N>& array)
			: utils::ByteArray<N, TTag>(array)
			, UnresolvedContainerBase<TUnresolvedInternalType, TData>()
		{}
		
		constexpr explicit UnresolvedValueArrayContainer(utils::ByteArray<N, TTag> value, const TUnresolvedInternalType& type, const TData* pData = nullptr)
			: utils::ByteArray<N, TTag>(value)
			, UnresolvedContainerBase<TUnresolvedInternalType, TData>(type, pData)
		{}
		
		constexpr explicit UnresolvedValueArrayContainer(const std::array<uint8_t, N>& array, const TUnresolvedInternalType& type, const TData* pData = nullptr)
			: utils::ByteArray<N, TTag>(array)
			, UnresolvedContainerBase<TUnresolvedInternalType, TData>(type, pData)
		{}
	};
	
	// end region Unresolved Data container
	
	// region byte arrays (ex address)

	constexpr size_t Signature_Size = 64;
	constexpr size_t Key_Size = 32;
	constexpr size_t Hash512_Size = 64;
	constexpr size_t Hash256_Size = 32;
	constexpr size_t Hash160_Size = 20;

	struct Signature_tag {};
	using Signature = utils::ByteArray<Signature_Size, Signature_tag>;

	struct Key_tag {};
	using Key = utils::ByteArray<Key_Size, Key_tag>;

	struct Hash512_tag { static constexpr auto Byte_Size = 64; };
	using Hash512 = utils::ByteArray<Hash512_Size, Hash512_tag>;

	struct Hash256_tag { static constexpr auto Byte_Size = 32; };
	using Hash256 = utils::ByteArray<Hash256_Size, Hash256_tag>;

	struct Hash160_tag {};
	using Hash160 = utils::ByteArray<Hash160_Size, Hash160_tag>;

	struct GenerationHash_tag { static constexpr auto Byte_Size = 32; };
	using GenerationHash = utils::ByteArray<Hash256_Size, GenerationHash_tag>;

	// endregion

	// region byte arrays (address)

	constexpr size_t Address_Decoded_Size = 25;
	constexpr size_t Address_Encoded_Size = 40;

	struct Address_tag {};
	using Address = utils::ByteArray<Address_Decoded_Size, Address_tag>;

	struct UnresolvedAddress_tag {};
	using UnresolvedAddress = utils::ByteArray<Address_Decoded_Size, UnresolvedAddress_tag>;
	
	struct UnresolvedLevyAddress_tag {};
	using UnresolvedLevyAddress = UnresolvedValueArrayContainer<Address_Decoded_Size, UnresolvedLevyAddress_tag>;
	
	// endregion

	// region base values

	struct Timestamp_tag {};
	using Timestamp = utils::BaseValue<uint64_t, Timestamp_tag>;
	
	struct Amount_tag {};
	using Amount = utils::BaseValue<uint64_t, Amount_tag>;
	
	enum class UnresolvedAmountType : uint8_t {
		Default,
		DriveDeposit,
		FileDeposit,
		FileUpload,
		MosaicLevy
	};
	
	struct UnresolvedAmountData : public UnresolvedData{
		virtual ~UnresolvedAmountData(){}
	};
	
	using UnresolvedAmount = UnresolvedValueContainer<Amount, UnresolvedAmountType, UnresolvedAmountData>;
	
	struct MosaicId_tag {};
	using MosaicId = utils::BaseValue<uint64_t, MosaicId_tag>;

	struct UnresolvedMosaicId_tag {};
	using UnresolvedMosaicId = utils::BaseValue<uint64_t, UnresolvedMosaicId_tag>;
	
	struct UnresolvedLevyMosaicId_tag {};
	using UnresolvedLevyMosaicId =  utils::BaseValue<uint64_t, UnresolvedLevyMosaicId_tag>;
	
	struct Height_tag {};
	using Height = utils::BaseValue<uint64_t, Height_tag>;

	struct BlockDuration_tag {};
	using BlockDuration = utils::BaseValue<uint64_t, BlockDuration_tag>;

	struct BlockFeeMultiplier_tag {};
	using BlockFeeMultiplier = utils::BaseValue<uint32_t, BlockFeeMultiplier_tag>;

	struct Difficulty_tag {
	public:
		static constexpr uint64_t Default_Value = 0;
		static constexpr uint64_t Min_Value = 0;
		static constexpr uint64_t Max_Value = std::numeric_limits<uint64_t>::max();
	};
	using Difficulty = utils::ClampedBaseValue<uint64_t, Difficulty_tag>;

	struct Importance_tag {};
	using Importance = utils::BaseValue<uint64_t, Importance_tag>;

	struct Reputation_tag {};
	using Reputation = utils::BaseValue<uint64_t, Reputation_tag>;

	// endregion

	using utils::RawBuffer;
	using utils::MutableRawBuffer;
	using utils::RawString;
	using utils::MutableRawString;

	/// Returns the size of the specified array.
	template<typename T, size_t N>
	constexpr size_t CountOf(T const (&)[N]) noexcept {
		return N;
	}

	// Network type (8 bit) + entity type (24 bit).
	using VersionType = uint32_t;

	/// Version set.
	using VersionSet = std::unordered_set<VersionType>;

	struct BlockchainVersion_tag {};
	using BlockchainVersion = utils::BaseValue<uint64_t, BlockchainVersion_tag>;

	struct VmVersion_tag {};
	using VmVersion = utils::BaseValue<uint64_t, VmVersion_tag>;
}
