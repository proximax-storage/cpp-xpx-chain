/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/
#include "plugins/services/globalstore/src/state/BaseConverters.h"
#include "plugins/services/globalstore/src/cache/GlobalStoreCache.h"
#include "src/cache/AccountRestrictionCache.h"
#include "plugins/txes/property/src/cache/PropertyCache.h"
#include "Observers.h"

namespace catapult { namespace observers {

		namespace {
			void MoveRestrictions(const state::AccountProperties& originalProperties, state::AccountRestrictions& restrictions)
			{
				for (const auto& restriction : originalProperties) {
					switch (restriction.second.descriptor().propertyType()) {
						case model::PropertyType::Address:
						{
							auto& addressIncoming = restrictions.restriction(model::AccountRestrictionFlags::Address);
							auto& addressOutgoing = restrictions.restriction(model::AccountRestrictionFlags::Address | model::AccountRestrictionFlags::Outgoing);
							for (const auto& value : restriction.second.values()) {
								model::AccountRestrictionModification valueModification {model::AccountRestrictionModificationAction::Add, value};
								if (restriction.second.descriptor().operationType() == state::OperationType::Block) {
									addressIncoming.block(valueModification);
									addressOutgoing.block(valueModification);
								} else {
									addressIncoming.allow(valueModification);
									addressOutgoing.allow(valueModification);
								}
							}
							break;
						}
						case model::PropertyType::MosaicId:
						{
							auto& mosaicIncoming = restrictions.restriction(model::AccountRestrictionFlags::MosaicId);
							for (const auto& value : restriction.second.values()) {
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
							for (const auto& value : restriction.second.values()) {
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
				for (const auto& restriction : originalRestrictions) {
					switch (restriction.second.descriptor().restrictionFlags()) {
						case model::AccountRestrictionFlags::Address:
						{
							auto& addressAny = properties.property(model::PropertyType::Address);
							for (const auto& value : restriction.second.values()) {
								model::RawPropertyModification valueModification {model::PropertyModificationType::Add, value};
								if (restriction.second.descriptor().operationType() == state::AccountRestrictionOperationType::Block)
									addressAny.block(valueModification);
								else
									addressAny.allow(valueModification);
							}
							break;
						}
						case model::AccountRestrictionFlags::MosaicId:
						{
							auto& mosaicIncoming = properties.property(model::PropertyType::MosaicId);
							for (const auto& value : restriction.second.values()) {
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
							for (const auto& value : restriction.second.values()) {
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

			void PluginSetup(const ObserverContext& context){
				auto& accountRestrictionCache = context.Cache.sub<cache::AccountRestrictionCache>();
				auto& propertyCache = context.Cache.sub<cache::PropertyCache>();
				auto& globalStore = context.Cache.sub<cache::GlobalStoreCache>();
				auto recordIter = globalStore.find(config::AccountRestrictionPluginInstalled_GlobalKey);
				if (NotifyMode::Commit == context.Mode) {
					if (!recordIter.tryGet()) {
						state::GlobalEntry installedRecord(config::AccountRestrictionPluginInstalled_GlobalKey, context.Height.unwrap(), state::PluginInstallConverter());
						globalStore.insert(installedRecord);
						auto pIterableView = propertyCache.tryMakeBroadIterableView();
						auto end = pIterableView->end();
						for(auto iter = pIterableView->begin(); iter != end; ++iter) {
							auto val = *iter;
							state::AccountRestrictions restrictions(val.first);
							MoveRestrictions(val.second, restrictions);
							accountRestrictionCache.insert(restrictions);
							propertyCache.remove(val.first);
						}
					}
				} else {
					if (recordIter.tryGet() && recordIter.get().Get<state::PluginInstallConverter>() == context.Height.unwrap()) {
						globalStore.remove(config::AccountRestrictionPluginInstalled_GlobalKey);
						auto pIterableView = accountRestrictionCache.tryMakeBroadIterableView();
						for(auto iter = pIterableView->begin(); iter != pIterableView->end(); ++iter) {
							auto val = *iter;
							state::AccountProperties restrictions(val.first);
							MoveRestrictions(val.second, restrictions);
							propertyCache.insert(restrictions);
							accountRestrictionCache.remove(val.first);
						}
					}
				}
			}

			void Observe(const model::BlockNotification<1>, const ObserverContext& context) {
				auto newConfig = context.Config.Network.GetPluginConfiguration<config::AccountRestrictionConfiguration>();
				if (!newConfig.Enabled)
					return;

				// This is the initial blockchain configuration and the plugin is enabled
				// or this is the activation height for a new configuration in which the plugin has just now become enabled
				if(context.HasChange(model::StateChangeFlags::Blockchain_Init) || (context.HasChange(model::StateChangeFlags::Network_Config_Upgraded)
					&& !context.Config.PreviousConfiguration->Network.GetPluginConfiguration<config::AccountRestrictionConfiguration>().Enabled)) {
					PluginSetup(context);
				}
			}
		}

		using Notification = model::BlockNotification<1>;
		DEFINE_OBSERVER(PropertyMigration, Notification, Observe);
}}
