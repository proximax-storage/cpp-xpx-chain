/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "DriveBaseSets.h"
#include "catapult/cache/CacheMixinAliases.h"
#include "catapult/cache/ReadOnlyArtifactCache.h"
#include "catapult/cache/ReadOnlyViewSupplier.h"
#include "catapult/deltaset/BaseSetDelta.h"

namespace catapult { namespace cache {

	/// Mixins used by the drive cache delta.
	using DriveCacheDeltaMixins = PatriciaTreeCacheMixins<DriveCacheTypes::PrimaryTypes::BaseSetDeltaType, DriveCacheDescriptor>;

	/// Basic delta on top of the drive cache.
	class BasicDriveCacheDelta
			: public utils::MoveOnly
			, public DriveCacheDeltaMixins::Size
			, public DriveCacheDeltaMixins::Contains
			, public DriveCacheDeltaMixins::ConstAccessor
			, public DriveCacheDeltaMixins::MutableAccessor
			, public DriveCacheDeltaMixins::PatriciaTreeDelta
			, public DriveCacheDeltaMixins::BasicInsertRemove
			, public DriveCacheDeltaMixins::DeltaElements
			, public DriveCacheDeltaMixins::Enable
			, public DriveCacheDeltaMixins::Height {
	public:
		using ReadOnlyView = DriveCacheTypes::CacheReadOnlyType;

	public:
		/// Creates a delta around \a driveSets.
		explicit BasicDriveCacheDelta(const DriveCacheTypes::BaseSetDeltaPointers& driveSets)
				: DriveCacheDeltaMixins::Size(*driveSets.pPrimary)
				, DriveCacheDeltaMixins::Contains(*driveSets.pPrimary)
				, DriveCacheDeltaMixins::ConstAccessor(*driveSets.pPrimary)
				, DriveCacheDeltaMixins::MutableAccessor(*driveSets.pPrimary)
				, DriveCacheDeltaMixins::PatriciaTreeDelta(*driveSets.pPrimary, driveSets.pPatriciaTree)
				, DriveCacheDeltaMixins::BasicInsertRemove(*driveSets.pPrimary)
				, DriveCacheDeltaMixins::DeltaElements(*driveSets.pPrimary)
				, m_pDriveEntries(driveSets.pPrimary)
		{}

	public:
		using DriveCacheDeltaMixins::ConstAccessor::find;
		using DriveCacheDeltaMixins::MutableAccessor::find;

	private:
		DriveCacheTypes::PrimaryTypes::BaseSetDeltaPointerType m_pDriveEntries;
	};

	/// Delta on top of the drive cache.
	class DriveCacheDelta : public ReadOnlyViewSupplier<BasicDriveCacheDelta> {
	public:
		/// Creates a delta around \a driveSets.
		explicit DriveCacheDelta(const DriveCacheTypes::BaseSetDeltaPointers& driveSets)
				: ReadOnlyViewSupplier(driveSets)
		{}
	};
}}
