/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "ContainerTypes.h"
#include <functional>

namespace catapult { namespace model {

	/// Context used to extract addresses.
	class ExtractorContext {
	private:
		using AddressesExtractor = std::function<UnresolvedAddressSet (const UnresolvedAddress&)>;
		using PublicKeysExtractor = std::function<PublicKeySet (const Key&)>;
	public:
		/// Creates a default context.
		ExtractorContext();

		/// Creates a context around \a extractor.
		explicit ExtractorContext(const AddressesExtractor& addressesExtractor, const PublicKeysExtractor& publicKeysExtractor);

	public:
		/// Extracts addresses from \a address.
		UnresolvedAddressSet extract(const UnresolvedAddress& address) const;

		/// Extracts public keys from \a publicKey.
		PublicKeySet extract(const Key& publicKey) const;

	private:
		AddressesExtractor m_addressesExtractor;
		PublicKeysExtractor m_publicKeysExtractor;
	};

	using ExtractorContextFactoryFunc = std::function<ExtractorContext ()>;
}}
