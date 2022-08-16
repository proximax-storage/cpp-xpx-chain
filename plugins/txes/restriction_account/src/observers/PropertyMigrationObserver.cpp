/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/
#include "src/cache/AccountRestrictionCache.h"
#include "plugins/txes/property/src/cache/PropertyCache.h"
#include "Observers.h"

#include "catapult/model/Address.h"



namespace catapult { namespace observers {

		namespace {
			void MoveRestrictions(const state::AccountProperties& originalProperties, state::AccountRestrictions& restrictions)
			{
				for(const auto& restriction : originalProperties)
				{
					switch(restriction.second.descriptor().propertyType())
					{
						case model::PropertyType::Address:
						{
							auto& addressIncoming = restrictions.restriction(model::AccountRestrictionFlags::Address);
							auto& addressOutgoing = restrictions.restriction(model::AccountRestrictionFlags::Address | model::AccountRestrictionFlags::Outgoing);
							for(const auto& value : restriction.second.values())
							{
								model::AccountRestrictionModification valueModification {model::AccountRestrictionModificationAction::Add, value};
								if(restriction.second.descriptor().operationType() == state::OperationType::Block)
								{
									addressIncoming.block(valueModification);
									addressOutgoing.block(valueModification);
								}
								else
								{
									addressIncoming.allow(valueModification);
									addressOutgoing.allow(valueModification);
								}
							}
							break;
						}
						case model::PropertyType::MosaicId:
						{
							auto& mosaicIncoming = restrictions.restriction(model::AccountRestrictionFlags::MosaicId);
							for(const auto& value : restriction.second.values())
							{
								model::AccountRestrictionModification valueModification {model::AccountRestrictionModificationAction::Add, value};
								if(restriction.second.descriptor().operationType() == state::OperationType::Block)
									mosaicIncoming.block(valueModification);
								else
									mosaicIncoming.allow(valueModification);

							}
							break;
						}
						case model::PropertyType::TransactionType:
						{
							auto& operationIncoming = restrictions.restriction(model::AccountRestrictionFlags::TransactionType | model::AccountRestrictionFlags::Outgoing);
							for(const auto& value : restriction.second.values())
							{
								model::AccountRestrictionModification valueModification {model::AccountRestrictionModificationAction::Add, value};
								if(restriction.second.descriptor().operationType() == state::OperationType::Block)
									operationIncoming.block(valueModification);
								else
									operationIncoming.allow(valueModification);
							}
							break;
						}
					}
				}
			}
			void MoveRestrictions(const state::AccountRestrictions& originalRestrictions, state::AccountProperties& properties)
			{
				for(const auto& restriction : originalRestrictions)
				{
					switch(restriction.second.descriptor().restrictionFlags())
					{
						case model::AccountRestrictionFlags::Address:
						{
							auto& addressAny = properties.property(model::PropertyType::Address);
							for(const auto& value : restriction.second.values())
							{
								model::RawPropertyModification valueModification {model::PropertyModificationType::Add, value};
								if(restriction.second.descriptor().operationType() == state::AccountRestrictionOperationType::Block)
								{
									addressAny.block(valueModification);
								}
								else
								{
									addressAny.allow(valueModification);
								}
							}
							break;
						}
						case model::AccountRestrictionFlags::MosaicId:
						{
							auto& mosaicIncoming = properties.property(model::PropertyType::MosaicId);
							for(const auto& value : restriction.second.values())
							{
								model::RawPropertyModification valueModification{model::PropertyModificationType::Add, value};
								if(restriction.second.descriptor().operationType() == state::AccountRestrictionOperationType::Block)
									mosaicIncoming.block(valueModification);
								else
									mosaicIncoming.allow(valueModification);

							}
							break;
						}
						case model::AccountRestrictionFlags::TransactionType:
						{
							auto& operationIncoming = properties.property(model::PropertyType::TransactionType);
							for(const auto& value : restriction.second.values())
							{
								model::RawPropertyModification valueModification{model::PropertyModificationType::Add, value};
								if(restriction.second.descriptor().operationType() == state::AccountRestrictionOperationType::Block)
									operationIncoming.block(valueModification);
								else
									operationIncoming.allow(valueModification);
							}
							break;
						}
					}
				}
			}
		}
		using Notification = model::BlockNotification<1>;
		DEFINE_OBSERVER(PropertyMigration, Notification, [](const Notification& notification, const ObserverContext& context) {
			if(context.Height != context.Config.ActivationHeight || context.Config.PreviousConfiguration == nullptr)
				return;

			// Check if plugin is enabled in new configuration and disabled in old configuration
			auto newConfig = context.Config.Network.GetPluginConfiguration<config::AccountRestrictionConfiguration>();
			auto oldConfig = context.Config.PreviousConfiguration->Network.GetPluginConfiguration<config::AccountRestrictionConfiguration>();
			if(newConfig.Enabled && !oldConfig.Enabled) {
				auto& accountRestrictionCache = context.Cache.sub<cache::AccountRestrictionCache>();
				auto& propertyCache = context.Cache.sub<cache::PropertyCache>();

				if (NotifyMode::Commit == context.Mode) {
					auto view = propertyCache.tryMakeBroadIterableView();
					for(const auto& val : *view) {
						state::AccountRestrictions restrictions(val.first);
						MoveRestrictions(val.second, restrictions);
						accountRestrictionCache.insert(restrictions);
						propertyCache.remove(val.first);
					}
				} else {
					auto view = accountRestrictionCache.tryMakeBroadIterableView();
					for(const auto& val : *view) {
						state::AccountProperties restrictions(val.first);
						MoveRestrictions(val.second, restrictions);
						propertyCache.insert(restrictions);
						accountRestrictionCache.remove(val.first);
					}
				}
			}
		});
}}
