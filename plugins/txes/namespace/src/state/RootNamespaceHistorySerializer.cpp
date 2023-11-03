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

#include "RootNamespaceHistorySerializer.h"
#include "catapult/io/PodIoUtils.h"
#include "catapult/utils/Casting.h"
#include <map>
#include <vector>

namespace catapult { namespace state {

	// region RootNamespaceHistoryNonHistoricalSerializer

	namespace {
		enum class HeaderMode { Include_History_Depth, Exclude_History_Depth };

		void SaveHeader(io::OutputStream& output, const RootNamespaceHistory& history, HeaderMode headerMode, VersionType version) {
			io::Write32(output, version);

			if (HeaderMode::Include_History_Depth == headerMode)
				io::Write64(output, history.historyDepth());

			io::Write(output, history.id());
		}

		void SaveAlias(io::OutputStream& output, const NamespaceAlias& alias, VersionType) {
			io::Write8(output, utils::to_underlying_type(alias.type()));
			switch (alias.type()) {
			case AliasType::Mosaic:
				io::Write(output, alias.mosaicId());
				break;

			case AliasType::Address:
				output.write(alias.address());
				break;

			default:
				break;
			}
		}

		void SaveChildren(io::OutputStream& output, const RootNamespace& root, VersionType version) {
			auto sortedChildPaths = root.sortedChildPaths();
			io::Write64(output, sortedChildPaths.size());
			for (const auto& path : sortedChildPaths) {
				// don't write the first part of the path (the root id) because it is redundant
				auto i = 1u;
				for (; i < path.size(); ++i)
					io::Write(output, path[i]);

				// pad the storage so that all children have a fixed size in the storage
				for (; i < path.capacity(); ++i)
					io::Write(output, NamespaceId());

				SaveAlias(output, root.alias(path[path.size() - 1]), version);
			}
		}

		const Key& SaveRootNamespace(io::OutputStream& output, const RootNamespace& root, const Key* pLastOwner, VersionType version) {
			output.write(root.owner());
			io::Write(output, root.lifetime().Start);
			io::Write(output, root.lifetime().End);
			if(version == 2) {
				io::Write(output, root.lifetime().GracePeriodEnd-root.lifetime().End);
			}
			SaveAlias(output, root.alias(root.id()), version);

			if (pLastOwner && *pLastOwner == root.owner())
				io::Write64(output, 0); // shared owner, don't rewrite children
			else
				SaveChildren(output, root, version);

			return root.owner();
		}
	}

	void RootNamespaceHistoryNonHistoricalSerializer::Save(const RootNamespaceHistory& history, io::OutputStream& output) {
		VersionType version{2};
		SaveHeader(output, history, HeaderMode::Exclude_History_Depth, version);
		if (0 == history.historyDepth())
			CATAPULT_THROW_RUNTIME_ERROR_1("cannot save empty namespace history", history.id());

		SaveRootNamespace(output, history.back(), nullptr, version);
	}

	namespace {
		struct Header {
			VersionType Version = 0;
			uint64_t HistoryDepth = 0;
			NamespaceId Id;
		};

		Header ReadHeader(io::InputStream& input, HeaderMode headerMode) {
			Header header;
			header.Version = io::Read32(input);

			if (headerMode == HeaderMode::Include_History_Depth)
				header.HistoryDepth = io::Read64(input);

			header.Id = io::Read<NamespaceId>(input);
			return header;
		}

		Namespace::Path LoadPath(io::InputStream& input, NamespaceId rootId, VersionType) {
			Namespace::Path path;
			path.push_back(rootId);
			for (auto i = 0u; i < path.capacity() - 1; ++i) {
				NamespaceId idPart;
				io::Read(input, idPart);
				if (NamespaceId() == idPart)
					continue;

				path.push_back(idPart);
			}

			return path;
		}

		NamespaceAlias LoadAlias(io::InputStream& input, VersionType) {
			auto aliasType = AliasType(io::Read8(input));
			switch (aliasType) {
			case AliasType::Mosaic:
				return NamespaceAlias(io::Read<MosaicId>(input));

			case AliasType::Address: {
				Address address;
				input.read(address);
				return NamespaceAlias(address);
			}

			default:
				return NamespaceAlias();
			}
		}

		using ChildDataPairs = std::vector<std::pair<Namespace::Path, NamespaceAlias>>;

		ChildDataPairs LoadChildren(io::InputStream& input, NamespaceId rootId, size_t numChildren, VersionType version) {
			ChildDataPairs childDataPairs;
			for (auto i = 0u; i < numChildren; ++i) {
				auto path = LoadPath(input, rootId, version);
				auto alias = LoadAlias(input, version);
				childDataPairs.emplace_back(path, alias);
			}

			return childDataPairs;
		}

		void LoadRootNamespace(io::InputStream& input, RootNamespaceHistory& history, VersionType version) {
			Key owner;
			input.read(owner);
			auto lifetimeStart = io::Read<Height>(input);
			auto lifetimeEnd = io::Read<Height>(input);
			auto gracePeriod = BlockDuration(0);
			if(version == 2) {
				gracePeriod = io::Read<BlockDuration>(input);
			}
			history.push_back(owner, NamespaceLifetime(lifetimeStart, lifetimeEnd, gracePeriod));

			auto alias = LoadAlias(input, version);
			history.back().setAlias(history.id(), alias);

			auto numChildren = io::Read64(input);
			auto childDataPairs = LoadChildren(input, history.id(), numChildren, version);

			auto& currentRoot = history.back();
			for (const auto& pair : childDataPairs) {
				auto ns = Namespace(pair.first);
				currentRoot.add(ns);
				currentRoot.setAlias(ns.id(), pair.second);
			}
		}
	}

	RootNamespaceHistory RootNamespaceHistoryNonHistoricalSerializer::Load(io::InputStream& input) {
		auto header = ReadHeader(input, HeaderMode::Exclude_History_Depth);
		RootNamespaceHistory history(header.Id);

		LoadRootNamespace(input, history, header.Version);
		return history;
	}

	// endregion

	// region RootNamespaceHistorySerializer

	void RootNamespaceHistorySerializer::Save(const RootNamespaceHistory& history, io::OutputStream& output) {
		VersionType version{2};
		SaveHeader(output, history, HeaderMode::Include_History_Depth, version);

		const Key* pLastOwner = nullptr;
		for (const auto& root : history)
			pLastOwner = &SaveRootNamespace(output, root, pLastOwner, version);
	}

	RootNamespaceHistory RootNamespaceHistorySerializer::Load(io::InputStream& input) {
		auto header = ReadHeader(input, HeaderMode::Include_History_Depth);
		RootNamespaceHistory history(header.Id);

		for (auto i = 0u; i < header.HistoryDepth; ++i)
			LoadRootNamespace(input, history, header.Version);

		return history;
	}

	// endregion
}}
