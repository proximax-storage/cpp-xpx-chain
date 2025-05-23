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
#include "src/model/PropertyTypes.h"
#include "src/state/PropertyDescriptor.h"
#include "catapult/model/Notifications.h"

namespace catapult { namespace model {

	// region property notification types

/// Defines a property notification type with \a DESCRIPTION, \a CODE and \a CHANNEL.
#define DEFINE_PROPERTY_NOTIFICATION(DESCRIPTION, CODE, CHANNEL) DEFINE_NOTIFICATION_TYPE(CHANNEL, Property, DESCRIPTION, CODE)

	/// Property type.
	DEFINE_PROPERTY_NOTIFICATION(Type_v1, 0x0001, Validator);

	/// Address property modification.
	DEFINE_PROPERTY_NOTIFICATION(Address_Modification_v1, 0x0010, All);

	/// Mosaic property modification.
	DEFINE_PROPERTY_NOTIFICATION(Mosaic_Modification_v1, 0x0011, All);

	/// Transaction type property modification.
	DEFINE_PROPERTY_NOTIFICATION(Transaction_Type_Modification_v1, 0x0012, All);

	/// Address property modifications.
	DEFINE_PROPERTY_NOTIFICATION(Address_Modifications_v1, 0x0020, Validator);

	/// Mosaic property modifications.
	DEFINE_PROPERTY_NOTIFICATION(Mosaic_Modifications_v1, 0x0021, Validator);

	/// Transaction type property modifications.
	DEFINE_PROPERTY_NOTIFICATION(Transaction_Type_Modifications_v1, 0x0022, Validator);

#undef DEFINE_PROPERTY_NOTIFICATION

	// endregion

	/// Notification of a property type.
	template<VersionType version>
	struct PropertyTypeNotification;

	template<>
	struct PropertyTypeNotification<1> : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Property_Type_v1_Notification;

	public:
		/// Creates a notification around \a propertyType.
		explicit PropertyTypeNotification(model::PropertyType propertyType)
				: Notification(Notification_Type, sizeof(PropertyTypeNotification<1>))
				, PropertyType(propertyType)
		{}

	public:
		/// Property type.
		model::PropertyType PropertyType;
	};

	/// Notification of a property value modification.
	template<typename TPropertyValue, NotificationType Property_Notification_Type, VersionType version>
	struct ModifyPropertyValueNotification;

	template<typename TPropertyValue, NotificationType Property_Notification_Type>
	struct ModifyPropertyValueNotification<TPropertyValue, Property_Notification_Type, 1> : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Property_Notification_Type;

	public:
		/// Creates a notification around \a key, \a propertyType and \a modification.
		explicit ModifyPropertyValueNotification(
				const Key& key,
				PropertyType propertyType,
				const PropertyModification<TPropertyValue>& modification)
				: Notification(Notification_Type, sizeof(ModifyPropertyValueNotification<TPropertyValue, Property_Notification_Type, 1>))
				, Key(key)
				, PropertyDescriptor(propertyType)
				, Modification(modification)
		{}

	public:
		/// Account's public key.
		catapult::Key Key;

		/// Property descriptor.
		state::PropertyDescriptor PropertyDescriptor;

		/// Property modification.
		/// \note TPropertyValue is the resolved value.
		PropertyModification<TPropertyValue> Modification;
	};

	using ModifyAddressPropertyValueNotification_v1 =
		ModifyPropertyValueNotification<UnresolvedAddress, Property_Address_Modification_v1_Notification, 1>;
	using ModifyMosaicPropertyValueNotification_v1 =
		ModifyPropertyValueNotification<UnresolvedMosaicId, Property_Mosaic_Modification_v1_Notification, 1>;
	using ModifyTransactionTypePropertyValueNotification_v1 =
		ModifyPropertyValueNotification<EntityType, Property_Transaction_Type_Modification_v1_Notification, 1>;

	/// Notification of a property modification.
	template<typename TPropertyValue, NotificationType Property_Notification_Type, VersionType version>
	struct ModifyPropertyNotification;

	template<typename TPropertyValue, NotificationType Property_Notification_Type>
	struct ModifyPropertyNotification<TPropertyValue, Property_Notification_Type, 1> : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Property_Notification_Type;

	public:
		/// Creates a notification around \a key, \a propertyType, \a modificationsCount and \a pModifications.
		explicit ModifyPropertyNotification(
				const Key& key,
				PropertyType propertyType,
				uint8_t modificationsCount,
				const PropertyModification<TPropertyValue>* pModifications)
				: Notification(Notification_Type, sizeof(ModifyPropertyNotification<TPropertyValue, Property_Notification_Type, 1>))
				, Key(key)
				, PropertyDescriptor(propertyType)
				, ModificationsCount(modificationsCount)
				, ModificationsPtr(pModifications)
		{}

	public:
		/// Account's public key.
		catapult::Key Key;

		/// Property descriptor.
		state::PropertyDescriptor PropertyDescriptor;

		/// Number of modifications.
		uint8_t ModificationsCount;

		/// Const pointer to the first modification.
		const PropertyModification<TPropertyValue>* ModificationsPtr;
	};

	using ModifyAddressPropertyNotification_v1 =
		ModifyPropertyNotification<UnresolvedAddress, Property_Address_Modifications_v1_Notification, 1>;
	using ModifyMosaicPropertyNotification_v1 =
		ModifyPropertyNotification<UnresolvedMosaicId, Property_Mosaic_Modifications_v1_Notification, 1>;
	using ModifyTransactionTypePropertyNotification_v1 =
		ModifyPropertyNotification<EntityType, Property_Transaction_Type_Modifications_v1_Notification, 1>;
}}
