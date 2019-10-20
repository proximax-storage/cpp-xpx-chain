/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "ExchangeEntryUtils.h"
#include "catapult/observers/ObserverContext.h"

namespace catapult { namespace state {

	// Offer entry.
	class OfferDeadlineEntry {
	public:
		// Creates an exchange entry around \a height.
		OfferDeadlineEntry(const Height& height) : m_height(height)
		{}

	public:
		using OfferDeadlineMap = std::multimap<Timestamp, utils::ShortHash>;
		using OfferHeightMap = std::multimap<Height, utils::ShortHash>;

	public:
		/// Gets the hash of the transaction with offer.
		const Height& height() const {
			return m_height;
		}

		/// Gets the buyOfferDeadlines.
		OfferDeadlineMap& buyOfferDeadlines() {
			return m_buyOfferDeadlines;
		}

		/// Gets the buyOfferDeadlines.
		const OfferDeadlineMap& buyOfferDeadlines() const {
			return m_buyOfferDeadlines;
		}

		/// Gets the buyOfferHeights.
		OfferHeightMap& buyOfferHeights() {
			return m_buyOfferHeights;
		}

		/// Gets the buyOfferHeights.
		const OfferHeightMap& buyOfferHeights() const {
			return m_buyOfferHeights;
		}

		/// Gets the sellOfferDeadlines.
		OfferDeadlineMap& sellOfferDeadlines() {
			return m_sellOfferDeadlines;
		}

		/// Gets the sellOfferDeadlines.
		const OfferDeadlineMap& sellOfferDeadlines() const {
			return m_sellOfferDeadlines;
		}

		/// Gets the sellOfferHeights.
		OfferHeightMap& sellOfferHeights() {
			return m_sellOfferHeights;
		}

		/// Gets the sellOfferHeights.
		const OfferHeightMap& sellOfferHeights() const {
			return m_sellOfferHeights;
		}

	private:
		Height m_height;
		OfferDeadlineMap m_buyOfferDeadlines;
		OfferHeightMap m_buyOfferHeights;
		OfferDeadlineMap m_sellOfferDeadlines;
		OfferHeightMap m_sellOfferHeights;
	};
}}
