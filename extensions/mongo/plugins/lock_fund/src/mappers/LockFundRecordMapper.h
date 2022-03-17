/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/model/Mosaic.h"
#include "mongo/src/mappers/MapperInclude.h"
#include "mongo/src/mappers/MapperUtils.h"
#include "plugins/txes/lock_fund/src/state/LockFundRecordGroup.h"

namespace catapult { namespace mongo { namespace plugins {

	namespace detail {

		template<typename TParam, typename TReturnValue>
		TReturnValue ToMongoId(TParam key);

		template<>
		int64_t ToMongoId<Height, int64_t>(Height key)
		{
			return mappers::ToInt64(key);
		}
		template<>
		bsoncxx::types::b_binary ToMongoId<Key, bsoncxx::types::b_binary>(Key key)
		{
			return mappers::ToBinary(key);
		}

		template<typename TDescriptor>
		struct DescriptorConnector {

		};

		template<>
		struct DescriptorConnector<state::LockFundHeightIndexDescriptor>
		{
			using MongoKeyType = int64_t;
			using MongoSubKeyType = bsoncxx::types::b_binary;
		};
		template<>
		struct DescriptorConnector<state::LockFundKeyIndexDescriptor>
		{
			using MongoKeyType = bsoncxx::types::b_binary;
			using MongoSubKeyType = int64_t;
		};

		template<typename TDescriptor>
		using AssociatedKeyType = typename DescriptorConnector<TDescriptor>::MongoKeyType;
		template<typename TDescriptor>
		using AssociatedSubKeyType = typename DescriptorConnector<TDescriptor>::MongoSubKeyType;
		void StreamMosaics(bsoncxx::builder::basic::sub_document& mosaicDocument, const std::string& name, const std::map<MosaicId, Amount> mosaics) {
			using bsoncxx::builder::basic::kvp;
			using bsoncxx::builder::basic::sub_array;
			using bsoncxx::builder::basic::sub_document;

			mosaicDocument.append(kvp(name, [&](sub_array mosaicArray) {
				for(auto& mosaic : mosaics)
				{
					mosaicArray.append([&](sub_document subdoc){
						subdoc.append(kvp("id", mappers::ToInt64(mosaic.first)), kvp("amount", mappers::ToInt64(mosaic.second)));
					});
				}
			}));
		}

		template<typename TDescriptor>
		void StreamLockFundRecordField(bsoncxx::builder::basic::sub_array& recordsArray, const std::pair<typename TDescriptor::ValueIdentifier, state::LockFundRecord>& field) {

			using bsoncxx::builder::basic::kvp;
			using bsoncxx::builder::basic::sub_array;
			using bsoncxx::builder::basic::sub_document;

			recordsArray.append([&](sub_document lockFundRecord) {
				lockFundRecord.append(
					kvp("key", ToMongoId<typename TDescriptor::ValueIdentifier, AssociatedSubKeyType<TDescriptor>>(field.first)));
				if(field.second.Active())
					StreamMosaics(lockFundRecord, "activeMosaics", field.second.Get());
				lockFundRecord.append(kvp("inactiveRecords", [&](sub_array inactiveRecords) {
					for(auto& inactiveRecord : field.second.InactiveRecords)
					{
						inactiveRecords.append([&](sub_document inactiveRecordDoc){
						  StreamMosaics(inactiveRecordDoc, "mosaics", inactiveRecord);
						});
					}
				}));
			});
		}

	}



	/// Maps a \a lockFundRecordGroup to the corresponding db model value. Consider replacing with stream builder for performance if needed.
	template<typename TDescriptor>
	bsoncxx::document::value ToDbModel(const state::LockFundRecordGroup<TDescriptor>& lockFundRecordGroup)
	{
		using bsoncxx::builder::basic::kvp;
		using bsoncxx::builder::basic::sub_array;
		using bsoncxx::builder::basic::sub_document;

		bsoncxx::builder::basic::document basic_builder{};

		basic_builder.append(
			kvp("identifier", detail::ToMongoId<typename TDescriptor::KeyType, detail::AssociatedKeyType<TDescriptor>>(lockFundRecordGroup.Identifier)),
			kvp("records", [&](sub_array lockFundRecordsArray) {
				for (const auto& field : lockFundRecordGroup.LockFundRecords)
					detail::StreamLockFundRecordField<TDescriptor>(lockFundRecordsArray, field);
			})
		);
		return basic_builder.extract();
	}

	/// Maps a database \a document to the corresponding model value.
	template<typename TDescriptor>
	state::LockFundRecordGroup<TDescriptor> ToLockFundRecord(const bsoncxx::document::view& document);
}}}
