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
#include "MosaicConstants.h"
#include "MosaicProperties.h"
#include "MosaicTypes.h"
#include "MosaicLevy.h"
#include "catapult/model/Notifications.h"

namespace catapult { namespace model {

	// region mosaic notification types

/// Defines a mosaic notification type with \a DESCRIPTION, \a CODE and \a CHANNEL.
#define DEFINE_MOSAIC_NOTIFICATION(DESCRIPTION, CODE, CHANNEL) DEFINE_NOTIFICATION_TYPE(CHANNEL, Mosaic, DESCRIPTION, CODE)

	/// Mosaic properties were provided.
	DEFINE_MOSAIC_NOTIFICATION(Properties_v1, 0x0012, Validator);

	/// Mosaic was defined.
	DEFINE_MOSAIC_NOTIFICATION(Definition_v1, 0x0013, All);

	/// Mosaic nonce and id were provided.
	DEFINE_MOSAIC_NOTIFICATION(Nonce_v1, 0x0014, Validator);

	/// Mosaic supply was changed.
	DEFINE_MOSAIC_NOTIFICATION(Supply_Change_v1, 0x0022, All);

	/// Mosaic rental fee has been sent.
	DEFINE_MOSAIC_NOTIFICATION(Rental_Fee_v1, 0x0030, Observer);
		
	/// Add mosaic levy
	DEFINE_MOSAIC_NOTIFICATION(Add_Levy_v1, 0x0040, All);
		
	/// Update mosaic levy
	DEFINE_MOSAIC_NOTIFICATION(Update_Levy_v1, 0x0041, All);
		
	/// Remove mosaic levy
	DEFINE_MOSAIC_NOTIFICATION(Remove_Levy_v1, 0x0042, All);
	
#undef DEFINE_MOSAIC_NOTIFICATION

	// endregion

	// region definition

	/// Notification of mosaic properties.
	/// \note This is required due to potentially lossy conversion from raw properties to MosaicProperties.
	template<VersionType version>
	struct MosaicPropertiesNotification;

	template<>
	struct MosaicPropertiesNotification<1> : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Mosaic_Properties_v1_Notification;

	public:
		/// Creates a notification around \a propertiesHeader and \a pProperties.
		explicit MosaicPropertiesNotification(const MosaicPropertiesHeader& propertiesHeader, const MosaicProperty* pProperties)
				: Notification(Notification_Type, sizeof(MosaicPropertiesNotification<1>))
				, PropertiesHeader(propertiesHeader)
				, PropertiesPtr(pProperties)
		{}

	public:
		/// Mosaic properties header.
		const MosaicPropertiesHeader& PropertiesHeader;

		/// Const pointer to the optional properties.
		const MosaicProperty* PropertiesPtr;
	};

	/// Notification of a mosaic definition.
	template<VersionType version>
	struct MosaicDefinitionNotification;

	template<>
	struct MosaicDefinitionNotification<1> : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Mosaic_Definition_v1_Notification;

	public:
		/// Creates a notification around \a signer, \a mosaicId and \a properties.
		explicit MosaicDefinitionNotification(const Key& signer, MosaicId mosaicId, const MosaicProperties& properties)
				: Notification(Notification_Type, sizeof(MosaicDefinitionNotification<1>))
				, Signer(signer)
				, MosaicId(mosaicId)
				, Properties(properties)
		{}

	public:
		/// Signer.
		const Key& Signer;

		/// Id of the mosaic.
		catapult::MosaicId MosaicId;

		/// Mosaic properties.
		MosaicProperties Properties;
	};

	/// Notification of a mosaic nonce and id.
	template<VersionType version>
	struct MosaicNonceNotification;

	template<>
	struct MosaicNonceNotification<1> : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Mosaic_Nonce_v1_Notification;

	public:
		/// Creates a notification around \a signer, \a mosaicNonce and \a mosaicId.
		explicit MosaicNonceNotification(const Key& signer, MosaicNonce mosaicNonce, catapult::MosaicId mosaicId)
				: Notification(Notification_Type, sizeof(MosaicNonceNotification<1>))
				, Signer(signer)
				, MosaicNonce(mosaicNonce)
				, MosaicId(mosaicId)
		{}

	public:
		/// Signer.
		const Key& Signer;

		/// Mosaic nonce.
		catapult::MosaicNonce MosaicNonce;

		/// Mosaic id.
		catapult::MosaicId MosaicId;
	};

	// endregion

	// region change

	/// Notification of a mosaic supply change.
	template<VersionType version>
	struct MosaicSupplyChangeNotification;

	template<>
	struct MosaicSupplyChangeNotification<1> : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Mosaic_Supply_Change_v1_Notification;

	public:
		/// Creates a notification around \a signer, \a mosaicId, \a direction and \a delta.
		MosaicSupplyChangeNotification(const Key& signer, UnresolvedMosaicId mosaicId, MosaicSupplyChangeDirection direction, Amount delta)
				: Notification(Notification_Type, sizeof(MosaicSupplyChangeNotification<1>))
				, Signer(signer)
				, MosaicId(mosaicId)
				, Direction(direction)
				, Delta(delta)
		{}

	public:
		/// Signer.
		const Key& Signer;

		/// Id of the affected mosaic.
		UnresolvedMosaicId MosaicId;

		/// Supply change direction.
		MosaicSupplyChangeDirection Direction;

		/// Amount of the change.
		Amount Delta;
	};

	// endregion

	// region rental fee

	/// Notification of a mosaic rental fee.
	template<VersionType version>
	struct MosaicRentalFeeNotification;

	template<>
	struct MosaicRentalFeeNotification<1> : public BasicBalanceNotification<MosaicRentalFeeNotification<1>> {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Mosaic_Rental_Fee_v1_Notification;

	public:
		/// Creates a notification around \a sender, \a recipient, \a mosaicId and \a amount.
		explicit MosaicRentalFeeNotification(
				const Key& sender,
				const UnresolvedAddress& recipient,
				UnresolvedMosaicId mosaicId,
				catapult::Amount amount)
				: BasicBalanceNotification(sender, mosaicId, amount)
				, Recipient(recipient)
		{}

	public:
		/// Recipient.
		UnresolvedAddress Recipient;
	};

	// endregion
		
	// region mosaic modify levy
	template<VersionType version>
	struct MosaicAddLevyNotification;
	
	template<>
	struct MosaicAddLevyNotification<1> :  public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Mosaic_Add_Levy_v1_Notification;
	
	public:
		explicit MosaicAddLevyNotification(catapult::MosaicId mosaicId, MosaicLevy levy, Key signer)
			: Notification(Notification_Type, sizeof(MosaicAddLevyNotification<1>))
			, MosaicId(mosaicId)
			, Levy(levy)
			, Signer(signer)
		{}
	
	public:
		
		/// Id of the mosaic.
		catapult::MosaicId MosaicId;
		
		/// levy container
		MosaicLevy Levy;
		
		/// Signer of the transaction
		Key Signer;
	};
	
	template<VersionType version>
	struct MosaicUpdateLevyNotification;
	template<>
	struct MosaicUpdateLevyNotification<1> :  public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Mosaic_Update_Levy_v1_Notification;
	
	public:
		explicit MosaicUpdateLevyNotification(
			uint32_t updateFlag,
			catapult::MosaicId mosaicId, MosaicLevy levy, Key signer)
			: Notification(Notification_Type, sizeof(MosaicUpdateLevyNotification<1>))
			, UpdateFlag(updateFlag)
			, MosaicId(mosaicId)
			, Levy(levy)
			, Signer(signer)
		{}
	
	public:
		
		/// update type flag
		uint32_t UpdateFlag;
		
		/// Id of the mosaic.
		catapult::MosaicId MosaicId;
		
		/// levy container
		MosaicLevy Levy;
		
		/// Signer of the transaction
		Key Signer;
	};
	// endregion
	
	template<VersionType version>
	struct MosaicRemoveLevyNotification;
	template<>
	struct MosaicRemoveLevyNotification<1> :  public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Mosaic_Remove_Levy_v1_Notification;
	
	public:
		explicit MosaicRemoveLevyNotification(catapult::MosaicId mosaicId, Key signer)
			: Notification(Notification_Type, sizeof(MosaicRemoveLevyNotification<1>))
			, MosaicId(mosaicId)
			, Signer(signer)
		{}
	
	public:
		
		/// Id of the mosaic.
		catapult::MosaicId MosaicId;
		
		/// Signer of the transaction
		Key Signer;
	};
}}
