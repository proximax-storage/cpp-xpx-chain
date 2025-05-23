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

#include "EntityType.h"

namespace catapult { namespace model {

#define DEFINE_CASE(RESULT) case utils::to_underlying_type(RESULT)

#define CASE_WELL_KNOWN_ENTITY_TYPE(NAME) DEFINE_CASE(Entity_Type_##NAME): return #NAME

#define CUSTOM_ENTITY_TYPE_DEFINITION 1
#undef DEFINE_ENTITY_TYPE

#define STR(SYMBOL) #SYMBOL

#define DEFINE_ENTITY_TYPE(BASIC_TYPE, FACILITY, DESCRIPTION, CODE) \
	DEFINE_CASE(MakeEntityType((model::BasicEntityType::BASIC_TYPE), (model::FacilityCode::FACILITY), CODE)): \
		return STR(DESCRIPTION)

	namespace {
		const char* ToString(EntityType entityType) {
			switch (utils::to_underlying_type(entityType)) {
			// well known types defined in EntityType.h
			CASE_WELL_KNOWN_ENTITY_TYPE(Nemesis_Block);
			CASE_WELL_KNOWN_ENTITY_TYPE(Block);

			// plugin entity types
			#include "plugins/txes/account_link/src/model/AccountLinkEntityType.h"
			#include "plugins/txes/aggregate/src/model/AggregateEntityType.h"
			#include "plugins/txes/committee/src/model/CommitteeEntityType.h"
			#include "plugins/txes/config/src/model/NetworkConfigEntityType.h"
			#include "plugins/txes/contract/src/model/ContractEntityType.h"
			#include "plugins/txes/dbrb/src/model/DbrbEntityType.h"
			#include "plugins/txes/exchange/src/model/ExchangeEntityType.h"
			#include "plugins/txes/exchange_sda/src/model/SdaExchangeEntityType.h"
			#include "plugins/txes/liquidityprovider/src/model/LiquidityProviderEntityType.h"
			#include "plugins/txes/lock_hash/src/model/HashLockEntityType.h"
			#include "plugins/txes/lock_secret/src/model/SecretLockEntityType.h"
			#include "plugins/txes/metadata/src/model/MetadataEntityType.h"
			#include "plugins/txes/metadata_v2/src/model/MetadataEntityType.h"
			#include "plugins/txes/mosaic/src/model/MosaicEntityType.h"
			#include "plugins/txes/multisig/src/model/MultisigEntityType.h"
			#include "plugins/txes/namespace/src/model/NamespaceEntityType.h"
			#include "plugins/txes/operation/src/model/OperationEntityType.h"
			#include "plugins/txes/property/src/model/PropertyEntityType.h"
			#include "plugins/txes/service/src/model/ServiceEntityType.h"
			#include "plugins/txes/storage/src/model/StorageEntityType.h"
			#include "plugins/txes/streaming/src/model/StreamingEntityType.h"
			#include "plugins/txes/supercontract/src/model/SuperContractEntityType.h"
			#include "plugins/txes/transfer/src/model/TransferEntityType.h"
			#include "plugins/txes/upgrade/src/model/BlockchainUpgradeEntityType.h"
			}

			return nullptr;
		}
	}

	std::ostream& operator<<(std::ostream& out, EntityType entityType) {
		auto pStr = ToString(entityType);
		if (pStr)
			out << pStr;
		else
			out << "EntityType<0x" << utils::HexFormat(utils::to_underlying_type(entityType)) << ">";

		return out;
	}
}}
