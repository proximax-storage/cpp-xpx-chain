/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "ExtractorContext.h"

namespace catapult { namespace model {

	ExtractorContext::ExtractorContext()
			: ExtractorContext(
				[](const auto& address) { return UnresolvedAddressSet{ address }; },
				[](const auto& publicKey) { return PublicKeySet{ publicKey }; }
			)
	{}

	ExtractorContext::ExtractorContext(const AddressesExtractor& addressesExtractor, const PublicKeysExtractor& publicKeysExtractor)
			: m_addressesExtractor(addressesExtractor), m_publicKeysExtractor(publicKeysExtractor)
	{}

	UnresolvedAddressSet ExtractorContext::extract(const UnresolvedAddress& address) const {
		return m_addressesExtractor(address);
	}

	PublicKeySet ExtractorContext::extract(const Key& publicKey) const {
		return m_publicKeysExtractor(publicKey);
	}
}}
