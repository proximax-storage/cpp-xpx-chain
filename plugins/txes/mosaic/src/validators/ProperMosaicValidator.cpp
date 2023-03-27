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

#include "Validators.h"
#include "ActiveMosaicView.h"
#include "catapult/validators/ValidatorContext.h"

namespace catapult { namespace validators {


	DEFINE_STATEFUL_VALIDATOR_WITH_TYPE(ProperMosaicV1, model::MosaicRequiredNotification<1>, [](const auto& notification, const ValidatorContext& context) {
	  auto view = ActiveMosaicView(context.Cache);

	  auto mosaicId = notification.MosaicId;
	  if (model::MosaicRequiredNotification<1>::MosaicType::Unresolved == notification.ProvidedMosaicType)
		  mosaicId = context.Resolvers.resolve(notification.UnresolvedMosaicId);

	  ActiveMosaicView::FindIterator mosaicIter;
	  return view.tryGet(mosaicId, context.Height, notification.Signer, mosaicIter);
	});

	DEFINE_STATEFUL_VALIDATOR_WITH_TYPE(ProperMosaicV2, model::MosaicRequiredNotification<2>, [](const auto& notification, const ValidatorContext& context) {
		auto view = ActiveMosaicView(context.Cache);
		auto mosaicId = notification.MosaicId;
		if (model::MosaicRequiredNotification<2>::MosaicType::Unresolved == notification.ProvidedMosaicType)
			mosaicId = context.Resolvers.resolve(notification.UnresolvedMosaicId);

		ActiveMosaicView::FindIterator mosaicIter;
		if(HasFlag(model::MosaicRequirementAction::VerifyOwner, notification.Action)) {
			auto result = view.tryGet(mosaicId, context.Height, notification.Signer, mosaicIter);
			if (IsValidationResultFailure(result))
				return result;
		}
		else {
			auto result = view.tryGet(mosaicId, context.Height, mosaicIter);
			if (IsValidationResultFailure(result))
				return result;
		}
		if (0 != notification.PropertyFlagMask) {
			const auto& properties = mosaicIter.get().definition().properties();
			for (auto i = 1u; i < utils::to_underlying_type(model::MosaicFlags::All); i <<= 1) {
				if( 0 != (notification.PropertyFlagMask & i))
				{
					if(HasFlag(model::MosaicRequirementAction::Set, notification.Action) && !properties.is(static_cast<model::MosaicFlags>(i)))
						return Failure_Mosaic_Required_Property_Flag_Unset;
					else if(HasFlag(model::MosaicRequirementAction::Unset, notification.Action) && properties.is(static_cast<model::MosaicFlags>(i)))
						return Failure_Mosaic_Required_Property_Flag_Unset;
				}
			}
		}
		return ValidationResult::Success;
	});
}}
