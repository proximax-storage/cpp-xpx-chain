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

#include "catapult/cache/ReadOnlyCatapultCache.h"
#include "catapult/model/ResolverContext.h"
#include "catapult/model/ContainerTypes.h"
#include "catapult/model/ExtractorContext.h"
#include "catapult/model/NotificationPublisher.h"
#include "catapult/model/TransactionUtils.h"

namespace catapult {
	namespace model {
		struct BlockElement;
		struct TransactionElement;
	}
}

namespace catapult { namespace addressextraction {

	/// Utility class for extracting addresses.
	class AddressExtractor {
	private:
		using ResolverHandleFactory = std::function<util::ResolverContextHandle ()>;
		
	public:
		/// Creates an extractor around \a pPublisher and \a contextFactory.
		AddressExtractor(std::unique_ptr<const model::NotificationPublisher>&& pPublisher,
				const model::ExtractorContextFactoryFunc & contextFactory,
				const ResolverHandleFactory& resolver);

	public:
		/// Extracts transaction addresses into \a transactionInfo.
		void extract(model::TransactionInfo& transactionInfo) const;

		/// Extracts transaction addresses into \a transactionInfos.
		void extract(model::TransactionInfosSet& transactionInfos) const;

		/// Extracts transaction addresses at \a height into \a transactionElement.
		void extract(model::TransactionElement& transactionElement, const Height& height) const;

		/// Extracts transaction addresses into \a blockElement.
		void extract(model::BlockElement& blockElement) const;

	private:
		std::unique_ptr<const model::NotificationPublisher> m_pPublisher;
		model::ExtractorContextFactoryFunc m_extractorContextFactory;
		ResolverHandleFactory m_resolverHandleFactory;
		
	};
}}
