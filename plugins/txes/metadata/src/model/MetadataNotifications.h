/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "src/model/MetadataTypes.h"
#include "plugins/txes/namespace/src/types.h"
#include "catapult/model/Notifications.h"

namespace catapult { namespace model {

	// region metadata notification types

/// Defines a metadata notification type with \a DESCRIPTION, \a CODE and \a CHANNEL.
#define DEFINE_METADATA_NOTIFICATION(DESCRIPTION, CODE, CHANNEL) DEFINE_NOTIFICATION_TYPE(CHANNEL, Metadata, DESCRIPTION, CODE)

	/// Metadata type.
	DEFINE_METADATA_NOTIFICATION(Type_v1, 0x0001, Validator);

	/// Metadata type.
	DEFINE_METADATA_NOTIFICATION(Field_Modification_v1, 0x0002, Validator);

	/// Metadata modifications.
	DEFINE_METADATA_NOTIFICATION(Modifications_v1, 0x0003, Validator);

	/// Address metadata modification.
	DEFINE_METADATA_NOTIFICATION(Address_Modification_v1, 0x0010, Observer);

	/// Mosaic metadata modification.
	DEFINE_METADATA_NOTIFICATION(Mosaic_Modification_v1, 0x0011, Observer);

	/// Namespace metadata modification.
	DEFINE_METADATA_NOTIFICATION(Namespace_Modification_v1, 0x0012, Observer);

	/// Address metadata modifications.
	DEFINE_METADATA_NOTIFICATION(Address_Modifications, 0x0020, Validator);

	/// Mosaic metadata modifications.
	DEFINE_METADATA_NOTIFICATION(Mosaic_Modifications, 0x0021, Validator);

	/// Namespace metadata modifications.
	DEFINE_METADATA_NOTIFICATION(Namespace_Modifications, 0x0022, Validator);

#undef DEFINE_METADATA_NOTIFICATION

	// endregion

	/// Notification of a metadata type.
	template<VersionType version>
	struct MetadataTypeNotification;

	template<>
	struct MetadataTypeNotification<1> : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Metadata_Type_v1_Notification;

	public:
		/// Creates a notification around \a metadataType.
		explicit MetadataTypeNotification(model::MetadataType metadataType)
				: Notification(Notification_Type, sizeof(MetadataTypeNotification<1>))
				, MetadataType(metadataType)
		{}

	public:
		/// Metadata type.
		model::MetadataType MetadataType;
	};

	/// Notification of a metadata modifications.
	template<VersionType version>
	struct MetadataModificationsNotification;

	template<>
	struct MetadataModificationsNotification<1> : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Metadata_Modifications_v1_Notification;
		using MetadataModifications = std::vector<const model::MetadataModification*>;

	public:
		/// Creates a notification around \a metadataType.
		explicit MetadataModificationsNotification(const Hash256& metadataId, const MetadataModifications& modifications)
				: Notification(Notification_Type, sizeof(MetadataModificationsNotification<1>))
				, MetadataId(metadataId)
				, Modifications(modifications)
		{}

	public:
		/// Metadata id.
		Hash256 MetadataId;

		/// Metadata modifications.
		MetadataModifications Modifications;
	};

	/// Notification of a metadata field modification.
	template<VersionType version>
	struct ModifyMetadataFieldNotification;

	template<>
	struct ModifyMetadataFieldNotification<1> : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Metadata_Field_Modification_v1_Notification;

	public:
		/// Creates a notification around \a modificationType, \a key and \a value.
		explicit ModifyMetadataFieldNotification(
				model::MetadataModificationType metadataModificationType,
				const uint8_t& keySize,
				const char* keyPtr,
				const uint16_t& valueSize,
				const char* valuePtr)
				: ModifyMetadataFieldNotification(
				Notification_Type, sizeof(ModifyMetadataFieldNotification<1>),
				metadataModificationType,
				keySize, keyPtr,
				valueSize, valuePtr)
		{}

	protected:
		/// Creates a notification around \a modificationType, \a key and \a value.
		explicit ModifyMetadataFieldNotification(
					NotificationType type, size_t size,
					model::MetadataModificationType metadataModificationType,
					const uint8_t& keySize,
					const char* keyPtr,
					const uint16_t& valueSize,
					const char* valuePtr)
				: Notification(type, size)
				, ModificationType(metadataModificationType)
				, KeySize(keySize)
				, KeyPtr(keyPtr)
				, ValueSize(valueSize)
				, ValuePtr(valuePtr)
		{}

	public:
		/// Metadata's modification type.
		model::MetadataModificationType ModificationType;

		/// Key size.
		uint8_t KeySize;

		/// Key pointer.
		const char* KeyPtr;

		/// Value size.
		uint16_t ValueSize;

		/// Key pointer.
		const char* ValuePtr;
	};

	/// Notification of a metadata value modification.
	template<typename TMetadataId, NotificationType Metadata_Notification_Type, VersionType version>
	struct ModifyMetadataValueNotification;

	template<typename TMetadataId, NotificationType Metadata_Notification_Type>
	struct ModifyMetadataValueNotification<TMetadataId, Metadata_Notification_Type, 1> : public ModifyMetadataFieldNotification<1> {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Metadata_Notification_Type;

	public:
		/// Creates a notification around \a metadataId and \a metadataType, \a key and \a value.
		explicit ModifyMetadataValueNotification(
				const TMetadataId& metadataId,
				model::MetadataType metadataType,
				model::MetadataModificationType metadataModificationType,
				const uint8_t& keySize,
				const char* keyPtr,
				const uint16_t& valueSize,
				const char* valuePtr)
				: ModifyMetadataFieldNotification(
						Notification_Type, sizeof(ModifyMetadataValueNotification),
						metadataModificationType,
						keySize, keyPtr,
						valueSize, valuePtr)
				, MetadataId(metadataId)
				, MetadataType(metadataType)
		{}

	public:
		/// Metadata's id.
		TMetadataId MetadataId;

		/// Metadata's type.
		model::MetadataType MetadataType;
	};

	using ModifyAddressMetadataValueNotification_v1 =
		ModifyMetadataValueNotification<UnresolvedAddress, Metadata_Address_Modification_v1_Notification, 1>;
	using ModifyMosaicMetadataValueNotification_v1 =
		ModifyMetadataValueNotification<UnresolvedMosaicId, Metadata_Mosaic_Modification_v1_Notification, 1>;
	using ModifyNamespaceMetadataValueNotification_v1 =
		ModifyMetadataValueNotification<NamespaceId, Metadata_Namespace_Modification_v1_Notification, 1>;

	/// Notification of a metadata modification.
	template<typename TMetadataId, NotificationType Metadata_Notification_Type>
	struct ModifyMetadataNotification : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Metadata_Notification_Type;

	public:
		/// Creates a notification around \a metadataId.
		explicit ModifyMetadataNotification(
				const Key& signer,
				const TMetadataId& metadataId)
				: Notification(Notification_Type, sizeof(ModifyMetadataNotification))
				, Signer(signer)
				, MetadataId(metadataId)
		{}

	public:
		/// Metadata's signer.
		Key Signer;

		/// Metadata's id.
		TMetadataId MetadataId;
	};

	using ModifyAddressMetadataNotification = ModifyMetadataNotification<UnresolvedAddress, Metadata_Address_Modifications_Notification>;
	using ModifyMosaicMetadataNotification = ModifyMetadataNotification<UnresolvedMosaicId, Metadata_Mosaic_Modifications_Notification>;
	using ModifyNamespaceMetadataNotification = ModifyMetadataNotification<NamespaceId, Metadata_Namespace_Modifications_Notification>;
}}
