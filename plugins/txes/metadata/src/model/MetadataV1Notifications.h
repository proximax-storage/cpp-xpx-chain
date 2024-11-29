/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "src/model/MetadataV1Types.h"
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
	DEFINE_METADATA_NOTIFICATION(Address_Modifications_v1, 0x0020, Validator);

	/// Mosaic metadata modifications.
	DEFINE_METADATA_NOTIFICATION(Mosaic_Modifications_v1, 0x0021, Validator);

	/// Namespace metadata modifications.
	DEFINE_METADATA_NOTIFICATION(Namespace_Modifications_v1, 0x0022, Validator);

#undef DEFINE_METADATA_NOTIFICATION

	// endregion

	/// Notification of a metadata type.
	template<VersionType version>
	struct MetadataV1TypeNotification;

	template<>
	struct MetadataV1TypeNotification<1> : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Metadata_Type_v1_Notification;

	public:
		/// Creates a notification around \a metadataType.
		explicit MetadataV1TypeNotification(model::MetadataV1Type metadataType)
				: Notification(Notification_Type, sizeof(MetadataV1TypeNotification<1>))
				, MetadataType(metadataType)
		{}

	public:
		/// Metadata type.
		model::MetadataV1Type MetadataType;
	};

	/// Notification of a metadata modifications.
	template<VersionType version>
	struct MetadataV1ModificationsNotification;

	template<>
	struct MetadataV1ModificationsNotification<1> : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Metadata_Modifications_v1_Notification;
		using MetadataModifications = std::vector<const model::MetadataV1Modification*>;

	public:
		/// Creates a notification around \a metadataType.
		explicit MetadataV1ModificationsNotification(const Hash256& metadataId, const MetadataModifications& modifications)
				: Notification(Notification_Type, sizeof(MetadataV1ModificationsNotification<1>))
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
	struct ModifyMetadataV1FieldNotification;

	template<>
	struct ModifyMetadataV1FieldNotification<1> : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Metadata_Field_Modification_v1_Notification;

	public:
		/// Creates a notification around \a modificationType, \a key and \a value.
		explicit ModifyMetadataV1FieldNotification(
				model::MetadataV1ModificationType metadataModificationType,
				const uint8_t& keySize,
				const char* keyPtr,
				const uint16_t& valueSize,
				const char* valuePtr)
				: ModifyMetadataV1FieldNotification(
				Notification_Type, sizeof(ModifyMetadataV1FieldNotification<1>),
				metadataModificationType,
				keySize, keyPtr,
				valueSize, valuePtr)
		{}

	protected:
		/// Creates a notification around \a modificationType, \a key and \a value.
		explicit ModifyMetadataV1FieldNotification(
					NotificationType type, size_t size,
					model::MetadataV1ModificationType metadataModificationType,
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
		model::MetadataV1ModificationType ModificationType;

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
	struct ModifyMetadataV1ValueNotification;

	template<typename TMetadataId, NotificationType Metadata_Notification_Type>
	struct ModifyMetadataV1ValueNotification<TMetadataId, Metadata_Notification_Type, 1> : public ModifyMetadataV1FieldNotification<1> {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Metadata_Notification_Type;

	public:
		/// Creates a notification around \a metadataId and \a metadataType, \a key and \a value.
		explicit ModifyMetadataV1ValueNotification(
				const TMetadataId& metadataId,
				model::MetadataV1Type metadataType,
				model::MetadataV1ModificationType metadataModificationType,
				const uint8_t& keySize,
				const char* keyPtr,
				const uint16_t& valueSize,
				const char* valuePtr)
				: ModifyMetadataV1FieldNotification(
						Notification_Type, sizeof(ModifyMetadataV1ValueNotification<TMetadataId, Metadata_Notification_Type, 1>),
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
		model::MetadataV1Type MetadataType;
	};

	using ModifyAddressMetadataValueNotification_v1 =
		ModifyMetadataV1ValueNotification<UnresolvedAddress, Metadata_Address_Modification_v1_Notification, 1>;
	using ModifyMosaicMetadataValueNotification_v1 =
		ModifyMetadataV1ValueNotification<UnresolvedMosaicId, Metadata_Mosaic_Modification_v1_Notification, 1>;
	using ModifyNamespaceMetadataValueNotification_v1 =
		ModifyMetadataV1ValueNotification<NamespaceId, Metadata_Namespace_Modification_v1_Notification, 1>;

	/// Notification of a metadata modification.
	template<typename TMetadataId, NotificationType Metadata_Notification_Type, VersionType version>
	struct ModifyMetadataV1Notification;

	template<typename TMetadataId, NotificationType Metadata_Notification_Type>
	struct ModifyMetadataV1Notification<TMetadataId, Metadata_Notification_Type, 1> : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Metadata_Notification_Type;

	public:
		/// Creates a notification around \a metadataId.
		explicit ModifyMetadataV1Notification(
				const Key& signer,
				const TMetadataId& metadataId)
				: Notification(Notification_Type, sizeof(ModifyMetadataV1Notification<TMetadataId, Metadata_Notification_Type, 1>))
				, Signer(signer)
				, MetadataId(metadataId)
		{}

	public:
		/// Metadata's signer.
		Key Signer;

		/// Metadata's id.
		TMetadataId MetadataId;
	};

	using ModifyAddressMetadataNotification_v1 = ModifyMetadataV1Notification<UnresolvedAddress, Metadata_Address_Modifications_v1_Notification, 1>;
	using ModifyMosaicMetadataNotification_v1 = ModifyMetadataV1Notification<UnresolvedMosaicId, Metadata_Mosaic_Modifications_v1_Notification, 1>;
	using ModifyNamespaceMetadataNotification_v1 = ModifyMetadataV1Notification<NamespaceId, Metadata_Namespace_Modifications_v1_Notification, 1>;
}}
