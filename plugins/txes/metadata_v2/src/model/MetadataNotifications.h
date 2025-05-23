/**
*** Copyright (c) 2016-2019, Jaguar0625, gimre, BloodyRookie, Tech Bureau, Corp.
*** Copyright (c) 2020-present, Jaguar0625, gimre, BloodyRookie.
*** All rights reserved.
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
#include "MetadataTypes.h"
#include "catapult/model/Notifications.h"

namespace catapult { namespace model {

	// region metadata notification types

/// Defines a metadata notification type with \a DESCRIPTION, \a CODE and \a CHANNEL.
#define DEFINE_METADATA_NOTIFICATION(DESCRIPTION, CODE, CHANNEL) DEFINE_NOTIFICATION_TYPE(CHANNEL, Metadata_v2, DESCRIPTION, CODE)

	/// Metadata value was received with specified sizes.
	DEFINE_METADATA_NOTIFICATION(Sizes, 0x0001, Validator);

	/// Metadata value was received.
	DEFINE_METADATA_NOTIFICATION(Value, 0x0002, All);

	/// Extended metadata value was received.
	DEFINE_METADATA_NOTIFICATION(Value_v2, 0x0003, All);

#undef DEFINE_METADATA_NOTIFICATION

	// endregion

	// region MetadataSizesNotification
	/// Notification of metadata sizes.
	template<VersionType version>
	struct MetadataSizesNotification;

	template<>
	struct MetadataSizesNotification<1> : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Metadata_v2_Sizes_Notification;

	public:
		/// Creates a notification around \a valueSizeDelta and \a valueSize.
		MetadataSizesNotification(int16_t valueSizeDelta, uint16_t valueSize)
				: Notification(Notification_Type, sizeof(MetadataSizesNotification))
				, ValueSizeDelta(valueSizeDelta)
				, ValueSize(valueSize)
		{}

	public:
		/// Change in value size in bytes.
		int16_t ValueSizeDelta;

		/// Value size in bytes.
		uint16_t ValueSize;
	};

	// endregion

	// region MetadataValueNotification

	template<typename TDerivedNotification>
	struct BasicMetadataValueNotification : public Notification {
	public:
		BasicMetadataValueNotification(
				const UnresolvedPartialMetadataKey& partialMetadataKey,
				const model::MetadataTarget& metadataTarget,
				int16_t valueSizeDelta,
				uint16_t valueSize,
				const uint8_t* pValue)
				: Notification(TDerivedNotification::Notification_Type, sizeof(TDerivedNotification))
				, PartialMetadataKey(partialMetadataKey)
				, MetadataTarget(metadataTarget)
				, ValueSizeDelta(valueSizeDelta)
				, ValueSize(valueSize)
				, ValuePtr(pValue)
		{}

	public:
		/// Partial metadata key.
		UnresolvedPartialMetadataKey PartialMetadataKey;

		/// Metadata target.
		model::MetadataTarget MetadataTarget;

		/// Change in value size in bytes.
		int16_t ValueSizeDelta;

		/// Value size in bytes.
		uint16_t ValueSize;

		/// Const pointer to the metadata value.
		const uint8_t* ValuePtr;
	};

	/// Notification of metadata value.
	template<VersionType version>
	struct MetadataValueNotification;

	template<>
	struct MetadataValueNotification<1> : public BasicMetadataValueNotification<MetadataValueNotification<1>> {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Metadata_v2_Value_Notification;

	public:
		/// Creates a notification around \a partialMetadataKey, \a metadataTarget, \a valueSizeDelta, \a valueSize and \a pValue.
		MetadataValueNotification(
				const UnresolvedPartialMetadataKey& partialMetadataKey,
				const model::MetadataTarget& metadataTarget,
				int16_t valueSizeDelta,
				uint16_t valueSize,
				const uint8_t* pValue)
				: BasicMetadataValueNotification(partialMetadataKey, metadataTarget, valueSizeDelta, valueSize, pValue)
		{}
	};

	template<>
	struct MetadataValueNotification<2> : public BasicMetadataValueNotification<MetadataValueNotification<2>> {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Metadata_v2_Value_v2_Notification;

	public:
		/// Creates a notification around \a partialMetadataKey, \a metadataTarget, \a valueSizeDelta, \a valueSize and \a pValue.
		MetadataValueNotification(
				const UnresolvedPartialMetadataKey& partialMetadataKey,
				const model::MetadataTarget& metadataTarget,
				int16_t valueSizeDelta,
				uint16_t valueSize,
				const uint8_t* pValue,
				bool isValueImmutable)
			: BasicMetadataValueNotification(partialMetadataKey, metadataTarget, valueSizeDelta, valueSize, pValue)
			, IsValueImmutable(isValueImmutable)
		{}

	public:

		/// State of immutability.
		bool IsValueImmutable;
	};

	// endregion
}}
