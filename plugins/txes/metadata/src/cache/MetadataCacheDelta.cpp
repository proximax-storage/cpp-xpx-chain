/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "MetadataCacheDelta.h"

namespace catapult { namespace cache {

	BasicMetadataCacheDelta::BasicMetadataCacheDelta(const MetadataCacheTypes::BaseSetDeltaPointers& metadataSets)
			: MetadataCacheDeltaMixins::Size(*metadataSets.pPrimary)
			, MetadataCacheDeltaMixins::Contains(*metadataSets.pPrimary)
			, MetadataCacheDeltaMixins::PatriciaTreeDelta(*metadataSets.pPrimary, metadataSets.pPatriciaTree)
			, MetadataCacheDeltaMixins::ConstAccessor(*metadataSets.pPrimary)
			, MetadataCacheDeltaMixins::MutableAccessor(*metadataSets.pPrimary)
			, MetadataCacheDeltaMixins::DeltaElements(*metadataSets.pPrimary)
			, m_pMetadataById(metadataSets.pPrimary)
			, m_pMetadataIdsByExpiryHeight(metadataSets.pHeightGrouping)
	{}

	void BasicMetadataCacheDelta::insert(const state::MetadataEntry& metadata) {
		for (const auto& field : metadata.fields()) {
			if (field.RemoveHeight.unwrap() != 0) {
				AddIdentifierWithGroup(*m_pMetadataIdsByExpiryHeight, field.RemoveHeight, metadata.metadataId());
			}
		}
		m_pMetadataById->insert(metadata);
	}

	void BasicMetadataCacheDelta::remove(const Hash256& metadataId) {
		m_pMetadataById->remove(metadataId);
	}

	BasicMetadataCacheDelta::CollectedIds BasicMetadataCacheDelta::prune(Height height) {
		BasicMetadataCacheDelta::CollectedIds collectedIds;
		ForEachIdentifierWithGroup(
				*m_pMetadataById,
				*m_pMetadataIdsByExpiryHeight,
				height,
				[this, height, &collectedIds](auto& metadata) {
			for (auto it = metadata.fields().begin(); it != metadata.fields().end();) {
				if (it->RemoveHeight == height) {
					it = metadata.fields().erase(it);
				} else {
					++it;
				}
			}

			if (metadata.fields().empty()) {
				collectedIds.insert(metadata.metadataId());
				m_pMetadataById->remove(metadata.metadataId());
			}
		});

		return collectedIds;
	}
}}
