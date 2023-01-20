/**
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/cache/ReadOnlyArtifactCache.h"
#include "extensions/fastfinality/src/dbrb/DbrbUtils.h"

namespace catapult {
	namespace cache {
		class BasicViewSequenceCacheDelta;
		class BasicViewSequenceCacheView;
	}
	namespace state { class ViewSequenceEntry; }
}

namespace catapult { namespace cache {

	/// A read-only overlay on top of an account cache.
	class ReadOnlyViewSequenceCache : public ReadOnlyArtifactCache<BasicViewSequenceCacheView, BasicViewSequenceCacheDelta, const Hash256&, state::ViewSequenceEntry> {
	private:
		using ReadOnlySubCache = ReadOnlyArtifactCache<BasicViewSequenceCacheView, BasicViewSequenceCacheDelta, const Hash256&, state::ViewSequenceEntry>;

	public:
		/// Creates a read-only overlay on top of \a cache.
		explicit ReadOnlyViewSequenceCache(const BasicViewSequenceCacheView& cache);

		/// Creates a read-only overlay on top of \a cache.
		explicit ReadOnlyViewSequenceCache(const BasicViewSequenceCacheDelta& cache);

	public:
		/// Gets the network identifier.
		dbrb::View getLatestView() const;

	public:
		using ReadOnlySubCache::size;
		using ReadOnlySubCache::contains;
		using ReadOnlySubCache::find;

	private:
		const BasicViewSequenceCacheView* m_pCache;
		const BasicViewSequenceCacheDelta* m_pCacheDelta;
	};
}}
