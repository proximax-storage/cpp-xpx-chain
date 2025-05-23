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

#include "catapult/deltaset/DeltaElementsMixin.h"
#include "tests/catapult/deltaset/test/BaseSetTestsInclude.h"
#include "tests/test/cache/DeltaElementsMixinTests.h"

namespace catapult { namespace deltaset {

#define TEST_CLASS DeltaElementsMixinTests

	namespace {
		template<typename TMutabilityTraits>
		using OrderedSetTraits = test::BaseSetTraits<
			TMutabilityTraits,
			test::OrderedSetTraits<test::SetElementType<TMutabilityTraits>>>;

		template<typename TMutabilityTraits>
		using UnorderedMapTraits = test::BaseSetTraits<
			TMutabilityTraits,
			test::UnorderedMapSetTraits<test::SetElementType<TMutabilityTraits>>>;

		using OrderedSetMutableTraits = OrderedSetTraits<test::MutableElementValueTraits>;
		using UnorderedMapMutableTraits = UnorderedMapTraits<test::MutableElementValueTraits>;

		// need to wrap DeltaElementsMixin in cache-like object in order to use MAKE_DELTA_ELEMENTS_MIXIN_TESTS

		template<typename TTraits>
		struct DeltaProxy : public DeltaElementsMixin<typename TTraits::DeltaType> {
		private:
			using DeltaType = typename TTraits::DeltaType;

		public:
			explicit DeltaProxy(const std::shared_ptr<DeltaType>& pSetDelta)
					: DeltaElementsMixin<DeltaType>(*pSetDelta)
					, m_pSetDelta(pSetDelta)
			{}

		public:
			void insert(const typename TTraits::ElementType& element) {
				m_pSetDelta->insert(element);
			}

			void remove(unsigned int id) {
				auto key = test::BatchElementFactory<TTraits>::CreateKey(std::to_string(id), id);
				m_pSetDelta->remove(key);
			}

		private:
			std::shared_ptr<DeltaType> m_pSetDelta;
		};

		template<typename TTraits>
		struct CacheProxy {
		public:
			auto createDelta(const Height&) {
				return std::make_unique<DeltaProxy<TTraits>>(m_set.rebase());
			}

			void commit() {
				m_set.commit();
			}

		private:
			typename TTraits::Type m_set;
		};

		struct BaseTraits {
			using IdType = unsigned int;

			static IdType MakeId(uint8_t id) {
				return id;
			}

			static IdType GetId(const test::MutableTestElement& element) {
				return element.Value;
			}
		};

		template<typename TSetTraits>
		struct MutableTraits : public BaseTraits {
			using CacheType = CacheProxy<TSetTraits>;
			using ValueType = test::MutableTestElement;

			static ValueType CreateWithId(uint8_t id) {
				ValueType element;
				element.Name = std::to_string(id);
				element.Value = id;
				return element;
			}
		};
	}

	DEFINE_DELTA_ELEMENTS_MIXIN_TESTS(MutableTraits<OrderedSetMutableTraits>, _OrderedSetMutable)
	DEFINE_DELTA_ELEMENTS_MIXIN_TESTS(MutableTraits<UnorderedMapMutableTraits>, _UnorderedMapMutable)
}}
