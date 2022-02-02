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
			const LockFundCacheTypes::BaseSetDeltaPointers& LockFundSets)
			: LockFundCacheDeltaMixins::PrimaryMixins::Size(*LockFundSets.pPrimary)
		, LockFundCacheDeltaMixins::LookupMixin(*LockFundSets.pPrimary, *LockFundSets.pKeyedInverseMap)
		, LockFundCacheDeltaMixins::PrimaryMixins::Contains(*LockFundSets.pPrimary)
		, LockFundCacheDeltaMixins::PrimaryMixins::PatriciaTreeDelta(*LockFundSets.pPrimary, LockFundSets.pPatriciaTree)
		, LockFundCacheDeltaMixins::PrimaryMixins::DeltaElements(*LockFundSets.pPrimary)
		, LockFundCacheDeltaMixins::KeyedMixins::Size(*LockFundSets.pKeyedInverseMap)
		, LockFundCacheDeltaMixins::KeyedMixins::Contains(*LockFundSets.pKeyedInverseMap)
		, LockFundCacheDeltaMixins::KeyedMixins::DeltaElements(*LockFundSets.pKeyedInverseMap)
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
				keyRecord->second.ActiveRecord.emplace(state::LockFundRecord(mosaics));
			else
				heightGroup->LockFundRecords.insert(std::make_pair(publicKey, state::LockFundRecordContainer(state::LockFundRecord(mosaics))));
		}
		else
		{
			state::LockFundRecordGroup<LockFundHeightIndexDescriptor> group;
			group.Identifier = unlockHeight;
			group.LockFundRecords.insert(std::make_pair(publicKey, state::LockFundRecordContainer(state::LockFundRecord(mosaics))));
			m_pLockFundGroupsByHeight->insert(group);
		}
		if(keyGroup)
		{
			auto heightRecord = keyGroup->LockFundRecords.find(unlockHeight);
			if(heightRecord != keyGroup->LockFundRecords.end())
				heightRecord->second.ActiveRecord.emplace(state::LockFundRecord(mosaics));
			else
				keyGroup->LockFundRecords.insert(std::make_pair(unlockHeight, state::LockFundRecordContainer(state::LockFundRecord(mosaics))));
		}
		else
		{
			state::LockFundRecordGroup<LockFundKeyIndexDescriptor> group;
			group.Identifier = publicKey;
			group.LockFundRecords.insert(std::make_pair(unlockHeight, state::LockFundRecordContainer(state::LockFundRecord(mosaics))));
			m_pLockFundGroupsByKey->insert(group);
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
				if(keyGroup->LockFundRecords[height].InactiveRecords.empty())
					keyGroup->LockFundRecords.erase(height);
				else
					keyGroup->LockFundRecords[height].ActiveRecord.reset();
				if(heightGroup->LockFundRecords[publicKey].InactiveRecords.empty())
					heightGroup->LockFundRecords.erase(publicKey);
				else
					heightGroup->LockFundRecords[publicKey].ActiveRecord.reset();
			}
			if(keyGroup->LockFundRecords.empty())
			{
				m_pLockFundGroupsByKey->remove(publicKey);
			}
			if(heightGroup->LockFundRecords.empty())
			{
				m_pLockFundGroupsByHeight->remove(height);
			}
		}
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
			}
		}
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
			}
		}
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
		}
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
		}
	}



}}
