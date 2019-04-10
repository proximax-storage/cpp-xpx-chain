/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "MetadataField.h"
#include "MetadataUtils.h"
#include "src/model/MetadataTypes.h"
#include <vector>

namespace catapult { namespace state {

	// Metadata entry.
	class MetadataEntry {
	public:
		// Creates a empty metadata.
		explicit MetadataEntry()
		{}

		// Creates a metadata entry around \a metadataId.
		explicit MetadataEntry(const std::vector<uint8_t>& buffer, model::MetadataType type)
				: m_metadataId(GetHash(buffer, type))
				, m_raw(buffer)
				, m_type(type)
		{}

	public:
		// Gets the metadata id.
		const Hash256& metadataId() const {
			return m_metadataId;
		}

	public:
		/// Gets the fields.
		const std::vector<MetadataField>& fields() const {
			return m_fields;
		}

		/// Gets the fields.
		std::vector<MetadataField>& fields() {
			return m_fields;
		}

		/// Gets the type of metadata.
		const model::MetadataType& type() const {
			return m_type;
		}

		/// Gets the raw of metadata.
		const std::vector<uint8_t>& raw() const {
			return m_raw;
		}

	private:
		Hash256 m_metadataId;
		/// Raw is byted version of MosaicId, NamespaceId and etc.
		std::vector<uint8_t> m_raw;
		model::MetadataType m_type;
		std::vector<MetadataField> m_fields;
	};
}}
