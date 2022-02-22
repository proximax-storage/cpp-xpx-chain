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
#include "LockFundCacheDelta.h"

namespace catapult { namespace cache {

		namespace {

		}

	BasicLockFundCacheDelta::BasicLockFundCacheDelta(
			const LockFundCacheTypes::BaseSetDeltaPointers& LockFundSets,
			std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder)
			: LockFundCacheDeltaMixins::Size(*LockFundSets.pPrimary, *LockFundSets.pKeyedInverseMap)
		, LockFundCacheDeltaMixins::LookupMixin(*LockFundSets.pPrimary, *LockFundSets.pKeyedInverseMap)
		, LockFundCacheDeltaMixins::PrimaryMixins::Contains(*LockFundSets.pPrimary)
		, LockFundCacheDeltaMixins::PrimaryMixins::PatriciaTreeDelta(*LockFundSets.pPrimary, LockFundSets.pPatriciaTree)
		, LockFundCacheDeltaMixins::PrimaryMixins::DeltaElements(*LockFundSets.pPrimary)
		, LockFundCacheDeltaMixins::KeyedMixins::Contains(*LockFundSets.pKeyedInverseMap)
		, m_pLockFundGroupsByHeight(LockFundSets.pPrimary)
		, m_pLockFundGroupsByKey(LockFundSets.pKeyedInverseMap)
	{}

	/// Important: No Validation happens here
	void BasicLockFundCacheDelta::insert(const Key& publicKey, Height unlockHeight, const std::map<MosaicId, Amount>& mosaics) {
		auto heightGroupIterator = m_pLockFundGroupsByHeight->find(unlockHeight);
		auto* heightGroup = heightGroupIterator.get();
		auto keyGroupIterator = m_pLockFundGroupsByKey->find(publicKey);
		auto* keyGroup = keyGroupIterator.get();
		if(heightGroup){
			auto keyRecord = heightGroup->LockFundRecords.find(publicKey);
			if(keyRecord != heightGroup->LockFundRecords.end())
				keyRecord->second.Set(state::LockFundRecordMosaicMap(mosaics));
			else
				heightGroup->LockFundRecords.insert(std::make_pair(publicKey, state::LockFundRecord(state::LockFundRecordMosaicMap(mosaics))));
		}
		else
		{
			state::LockFundRecordGroup<LockFundHeightIndexDescriptor> group;
			group.Identifier = unlockHeight;
			group.LockFundRecords.insert(std::make_pair(publicKey, state::LockFundRecord(state::LockFundRecordMosaicMap(mosaics))));
			m_pLockFundGroupsByHeight->insert(group);
		}
		if(keyGroup)
		{
			auto heightRecord = keyGroup->LockFundRecords.find(unlockHeight);
			if(heightRecord != keyGroup->LockFundRecords.end())
				heightRecord->second.Set(state::LockFundRecordMosaicMap(mosaics));
			else
				keyGroup->LockFundRecords.insert(std::make_pair(unlockHeight, state::LockFundRecord(state::LockFundRecordMosaicMap(mosaics))));
		}
		else
		{
			state::LockFundRecordGroup<LockFundKeyIndexDescriptor> group;
			group.Identifier = publicKey;
			group.LockFundRecords.insert(std::make_pair(unlockHeight, state::LockFundRecord(state::LockFundRecordMosaicMap(mosaics))));
			m_pLockFundGroupsByKey->insert(group);
		}
	}

	/// Important: No Validation happens here
	void BasicLockFundCacheDelta::insert(const state::LockFundRecordGroup<LockFundHeightIndexDescriptor>& record) {
		m_pLockFundGroupsByHeight->insert(record);
		for(auto lockFundRecord : record.LockFundRecords)
		{
			auto keyGroupIterator = m_pLockFundGroupsByKey->find(lockFundRecord.first);
			auto* keyGroup = keyGroupIterator.get();
			if(keyGroup)
			{
				auto heightRecord = keyGroup->LockFundRecords.find(record.Identifier);
				if(heightRecord != keyGroup->LockFundRecords.end())
				{
					if(lockFundRecord.second.Active())
						heightRecord->second.Set(lockFundRecord.second.Get());
					heightRecord->second.InactiveRecords = lockFundRecord.second.InactiveRecords;
				}
				else
					keyGroup->LockFundRecords.insert(std::make_pair(record.Identifier, lockFundRecord.second));
			}
			else
			{
				state::LockFundRecordGroup<LockFundKeyIndexDescriptor> group;
				group.Identifier = lockFundRecord.first;
				group.LockFundRecords.insert(std::make_pair(record.Identifier, lockFundRecord.second));
				m_pLockFundGroupsByKey->insert(group);
			}
		}


	}

	void BasicLockFundCacheDelta::remove(const Key& publicKey, Height height)
	{
		auto keyGroupIterator = m_pLockFundGroupsByKey->find(publicKey);
		auto* keyGroup = keyGroupIterator.get();
		if(keyGroup)
		{
			auto heightGroupIterator = m_pLockFundGroupsByHeight->find(height);
			auto* heightGroup = heightGroupIterator.get();
			if(keyGroup->LockFundRecords.find(height) != keyGroup->LockFundRecords.end()){
				if(keyGroup->LockFundRecords[height].Empty())
					keyGroup->LockFundRecords.erase(height);
				else
					keyGroup->LockFundRecords[height].Unset();
				if(heightGroup->LockFundRecords[publicKey].Empty())
					heightGroup->LockFundRecords.erase(publicKey);
				else
					heightGroup->LockFundRecords[publicKey].Unset();
				if(keyGroup->LockFundRecords.empty())
				{
					m_pLockFundGroupsByKey->remove(publicKey);
				}
				if(heightGroup->LockFundRecords.empty())
				{
					m_pLockFundGroupsByHeight->remove(height);
				}
				return;
			}
			CATAPULT_THROW_INVALID_ARGUMENT("Height does not exist!");
		}
		CATAPULT_THROW_INVALID_ARGUMENT("Key does not exist!");
	}

	void BasicLockFundCacheDelta::disable(const Key& publicKey, Height height)
	{
		auto keyGroupIterator = m_pLockFundGroupsByKey->find(publicKey);
		auto* keyGroup = keyGroupIterator.get();
		if(keyGroup)
		{
			auto heightGroupIterator = m_pLockFundGroupsByHeight->find(height);
			auto* heightGroup = heightGroupIterator.get();
			if(keyGroup->LockFundRecords.find(height) != keyGroup->LockFundRecords.end()){
				keyGroup->LockFundRecords[height].Inactivate();
				heightGroup->LockFundRecords[publicKey].Inactivate();
				return;
			}
			CATAPULT_THROW_INVALID_ARGUMENT("Height does not exist!");
		}
		CATAPULT_THROW_INVALID_ARGUMENT("Key does not exist!");
	}

	void BasicLockFundCacheDelta::enable(const Key& publicKey, Height height)
	{
		auto keyGroupIterator = m_pLockFundGroupsByKey->find(publicKey);
		auto* keyGroup = keyGroupIterator.get();
		if(keyGroup)
		{
			auto heightGroupIterator = m_pLockFundGroupsByHeight->find(height);
			auto* heightGroup = heightGroupIterator.get();
			if(keyGroup->LockFundRecords.find(height) != keyGroup->LockFundRecords.end()){
				keyGroup->LockFundRecords[height].Reactivate();
				heightGroup->LockFundRecords[publicKey].Reactivate();
				return;
			}
			CATAPULT_THROW_INVALID_ARGUMENT("Height does not exist!");
		}
		CATAPULT_THROW_INVALID_ARGUMENT("Key does not exist!");
	}

	void BasicLockFundCacheDelta::remove(Height height)
	{
		auto heightGroupIterator = m_pLockFundGroupsByHeight->find(height);
		auto* heightGroup = heightGroupIterator.get();
		if(heightGroup)
		{
			for(auto& tiedRecord : heightGroup->LockFundRecords)
			{
				auto keyGroupIterator = m_pLockFundGroupsByKey->find(tiedRecord.first);
				auto* keyGroup = keyGroupIterator.get();
				keyGroup->LockFundRecords[height].Inactivate();
				tiedRecord.second.Inactivate();
			}
			return;
		}
		CATAPULT_THROW_INVALID_ARGUMENT("Record at Height does not exist!");
	}

	void BasicLockFundCacheDelta::recover(Height height)
	{
		auto heightGroupIterator = m_pLockFundGroupsByHeight->find(height);
		auto* heightGroup = heightGroupIterator.get();
		if(heightGroup)
		{
			for(auto& tiedRecord : heightGroup->LockFundRecords)
			{
				auto keyGroupIterator = m_pLockFundGroupsByKey->find(tiedRecord.first);
				auto* keyGroup = keyGroupIterator.get();
				keyGroup->LockFundRecords[height].Reactivate();
				tiedRecord.second.Reactivate();
			}
			return;
		}
		CATAPULT_THROW_INVALID_ARGUMENT("Record at Height does not exist!");
	}


	void BasicLockFundCacheDelta::prune(Height height)
	{
		auto heightGroupIterator = m_pLockFundGroupsByHeight->find(height);
		auto* heightGroup = heightGroupIterator.get();
		if(heightGroup)
		{
			for(auto& tiedRecord : heightGroup->LockFundRecords)
			{
				auto keyGroupIterator = m_pLockFundGroupsByKey->find(tiedRecord.first);
				auto* keyGroup = keyGroupIterator.get();
				keyGroup->LockFundRecords.erase(height);
				if(keyGroup->LockFundRecords.empty())
				{
					m_pLockFundGroupsByKey->remove(tiedRecord.first);
				}
			}
			m_pLockFundGroupsByHeight->remove(height);
			return;
		}
		CATAPULT_THROW_INVALID_ARGUMENT("Record at Height does not exist!");
	}



}}
