/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "MetadataV1Field.h"
#include "MetadataV1Utils.h"
#include "plugins/txes/metadata/src/model/MetadataV1Types.h"
#include <vector>

namespace catapult { namespace state {

	// Metadata entry.
	class MetadataV1Entry {
	public:
		// Creates a empty metadata.
		explicit MetadataV1Entry() : m_type(model::MetadataV1Type{0})
		{}

		// Test only constructor, creates a metadata entry around \a metadataId.
		explicit MetadataV1Entry(const Hash256& hash)
				: m_metadataId(hash)
				, m_type(model::MetadataV1Type{0})
		{}

		// Creates a metadata entry around \a metadataId.
		explicit MetadataV1Entry(const std::vector<uint8_t>& buffer, model::MetadataV1Type type)
				: m_metadataId(GetHash(buffer, type))
				, m_raw(buffer)
				, m_type(type)
		{}

	public:
		// Gets the metadata id.
		const Hash256& metadataId() const {
			return m_metadataId;
		}

		/// Gets the fields as const.
		const std::vector<MetadataV1Field>& fields() const {
			return m_fields;
		}

		/// Gets the fields.
		std::vector<MetadataV1Field>& fields() {
			return m_fields;
		}

		/// Gets the type of metadata.
		const model::MetadataV1Type& type() const {
			return m_type;
		}

		/// Gets the metadata as bytes.
		const std::vector<uint8_t>& raw() const {
			return m_raw;
		}

	private:
		Hash256 m_metadataId;
		/// Raw is bytes of MosaicId, NamespaceId and etc.
		std::vector<uint8_t> m_raw;
		model::MetadataV1Type m_type;
		std::vector<MetadataV1Field> m_fields;
	};
}}
