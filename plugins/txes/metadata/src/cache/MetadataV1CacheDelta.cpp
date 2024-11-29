/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "MetadataV1CacheDelta.h"

namespace catapult { namespace cache {

	BasicMetadataV1CacheDelta::BasicMetadataV1CacheDelta(const MetadataV1CacheTypes::BaseSetDeltaPointers& metadataSets)
			: MetadataV1CacheDeltaMixins::Size(*metadataSets.pPrimary)
			, MetadataV1CacheDeltaMixins::Contains(*metadataSets.pPrimary)
			, MetadataV1CacheDeltaMixins::PatriciaTreeDelta(*metadataSets.pPrimary, metadataSets.pPatriciaTree)
			, MetadataV1CacheDeltaMixins::ConstAccessor(*metadataSets.pPrimary)
			, MetadataV1CacheDeltaMixins::MutableAccessor(*metadataSets.pPrimary)
			, MetadataV1CacheDeltaMixins::DeltaElements(*metadataSets.pPrimary)
			, MetadataV1CacheDeltaMixins::PrivateAccessor(*metadataSets.pPrimary, *metadataSets.pHeightGrouping)
			, MetadataV1CacheDeltaMixins::BroadIteration(*metadataSets.pPrimary, *metadataSets.pHeightGrouping)
			, m_pMetadataById(metadataSets.pPrimary)
			, m_pMetadataIdsByExpiryHeight(metadataSets.pHeightGrouping)
	{}

	void BasicMetadataV1CacheDelta::insert(const state::MetadataV1Entry& metadata) {
		for (const auto& field : metadata.fields()) {
			if (field.RemoveHeight.unwrap() != 0) {
				AddIdentifierWithGroup(*m_pMetadataIdsByExpiryHeight, field.RemoveHeight, metadata.metadataId());
			}
		}
		m_pMetadataById->insert(metadata);
	}

	void BasicMetadataV1CacheDelta::remove(const Hash256& metadataId) {
		m_pMetadataById->remove(metadataId);
	}

	BasicMetadataV1CacheDelta::CollectedIds BasicMetadataV1CacheDelta::prune(Height height) {
		BasicMetadataV1CacheDelta::CollectedIds collectedIds;
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
