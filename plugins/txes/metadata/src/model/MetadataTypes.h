/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/utils/BitwiseEnum.h"
#include "catapult/types.h"
#include "catapult/model/Transaction.h"
#include "plugins/txes/namespace/src/types.h"
#include <stdint.h>

namespace catapult { namespace model {

#pragma pack(push, 1)

	/// Metadata types.
	enum class MetadataType : uint8_t {
		/// Metadata type is an address.
		Address = 0x01,

		/// Metadata type is a mosaic id.
		MosaicId = 0x02,

		/// Metadata type is a namespace id.
		NamespaceId = 0x03,
	};

	MAKE_BITWISE_ENUM(MetadataType)

	/// Metadata modification type.
	enum class MetadataModificationType : uint8_t {
		/// Add metadata value.
		Add,

		/// Remove metadata value.
		Del
	};

	/// Binary layout for a metadata modification.
	struct MetadataModification {
	public:
		/// Size of modification
		uint32_t Size;

		/// Modification type.
		MetadataModificationType ModificationType;

		/// Key size in bytes.
		uint8_t KeySize;

		/// Value size in bytes.
		uint16_t ValueSize;

	private:
		const uint8_t* PayloadStart() const {
			return reinterpret_cast<const uint8_t*>(this) + sizeof(MetadataModification);
		}

		uint8_t* PayloadStart() {
			return reinterpret_cast<uint8_t*>(this) + sizeof(MetadataModification);
		}

		template<typename T>
		static auto* KeyPtrT(T& modification) {
			return modification.KeySize ? modification.PayloadStart() : nullptr;
		}

		template<typename T>
		static auto* ValuePtrT(T& modification) {
			auto* pPayloadStart = modification.PayloadStart();
			return modification.ValueSize && pPayloadStart ? pPayloadStart + modification.KeySize : nullptr;
		}

	public:
		// followed by key data if KeySize != 0
		DEFINE_TRANSACTION_VARIABLE_DATA_ACCESSORS(Key, char)

		// followed by value data if ValueSize != 0
		DEFINE_TRANSACTION_VARIABLE_DATA_ACCESSORS(Value, char)

		bool IsSizeValid() const {
			return Size == (sizeof(MetadataModification) + KeySize + ValueSize);
		}
	};

#pragma pack(pop)
}}
