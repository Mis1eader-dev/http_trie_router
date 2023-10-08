/*
            DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE
                    Version 2, December 2004

 Copyright (C) 2023 Mis1eader

 Everyone is permitted to copy and distribute verbatim or modified
 copies of this license document, and changing it is allowed as long
 as the name is changed.

            DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE
   TERMS AND CONDITIONS FOR COPYING, DISTRIBUTION AND MODIFICATION

  0. You just DO WHAT THE FUCK YOU WANT TO.
*/

#include <cstddef>
#include <iostream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

using std::cout;
using std::string;
using std::string_view;

struct RedirectLocation
{
	string host, path;
};

struct RedirectFrom
{
	string host, path;
	bool isWildcard = false;
	size_t toIdx;
};

struct RedirectGroup
{
	std::unordered_map<string_view, RedirectGroup*> groups;
	size_t maxPathLen = string_view::npos;
	RedirectLocation* to = nullptr, *wildcard = nullptr;
};

static std::unordered_map<string_view, RedirectGroup> rulesFrom_;
static std::vector<RedirectLocation> rulesTo_;
static std::vector<RedirectFrom> rulesFromData_;

static void initAndStart(
	const std::unordered_map<
		string, std::unordered_map<
			string, std::vector<
				string
			>
		>
	>& config);

static void printMap();

static void lookup(string& host, string& path);
static void redirectingAdvice(const string& reqHost, const string& reqPath);

int main()
{
	std::unordered_map<
		string, std::unordered_map<
			string, std::vector<
				string
			>
		>
	> config
	{{
		"rules", {
			// Strict and wildcard simultaneously
			{
				"strict.example.com", {
					"10.10.10.1",
				}
			},
			{
				"www.example.com", {
					"10.10.10.1/*",
					"search.example.com/*",
				}
			},

			// Path in redirectFrom
			{
				"images.example.com", {
					"image.example.com/*",
					"www.example.com/image",
					"www.example.com/images",
				}
			},

			// Path in redirectTo
			{
				"www.example.com/maps", {
					"www.example.com/map/*",
					"map.example.com",
					"maps.example.com",
				}
			},

			// Wildcard
			{
				"files.example.com", {
					"file.example.com/*",
					"www.example.com/file/*",
					"www.example.com/files/*",
				}
			},

			// Recursive
			{
				"www.third.com", {
					"www.second.com/third/*",
					"www.first.com/third/*",
				}
			},
			{
				"www.second.com", {
					"www.first.com/second/*",
				}
			},

			// Path only
			{
				"/", {
					"/home/*",
				}
			},
			{
				"/websocket", {
					"/ws/*",
				}
			},
			{
				"/str/ict", {
					"/strict",
				}
			},

			// Cyclic host
			{
				"cyclic1.example.com/final", {
					"cyclic2.example.com/cyclic"
				}
			},
			{
				"cyclic2.example.com/cyclic", {
					"cyclic1.example.com/cyclic"
				}
			},
		},
	}};

	cout << "-- Generating map --\n";
	initAndStart(config);
	printMap();

	cout << "\n-- Starting lookup --\n";
	redirectingAdvice("10.10.10.1", "/");
	redirectingAdvice("10.10.10.1", "/maps");
	redirectingAdvice("10.10.10.1", "/maps/search");
	redirectingAdvice("10.10.10.1", "/map");
	redirectingAdvice("10.10.10.1", "/map/search");
	redirectingAdvice("search.example.com", "/");
	redirectingAdvice("search.example.com", "/maps/search");

	redirectingAdvice("image.example.com", "/");
	redirectingAdvice("image.example.com", "/places");
	redirectingAdvice("www.example.com", "/ima");
	redirectingAdvice("www.example.com", "/image");
	redirectingAdvice("www.example.com", "/image/png");
	redirectingAdvice("www.example.com", "/images");
	redirectingAdvice("www.example.com", "/images/png");

	redirectingAdvice("www.example.com", "/maps");
	redirectingAdvice("www.example.com", "/maps/places");
	redirectingAdvice("www.example.com", "/map");
	redirectingAdvice("www.example.com", "/map/places");
	redirectingAdvice("map.example.com", "/");
	redirectingAdvice("map.example.com", "/places");
	redirectingAdvice("maps.example.com", "/");
	redirectingAdvice("maps.example.com", "/places");

	redirectingAdvice("file.example.com", "/");
	redirectingAdvice("file.example.com", "/docs");
	redirectingAdvice("file.example.com", "/docs/untitled.docx");
	redirectingAdvice("www.example.com", "/file");
	redirectingAdvice("www.example.com", "/file/docs");
	redirectingAdvice("www.example.com", "/file/docs/untitled.docx");
	redirectingAdvice("www.example.com", "/files");
	redirectingAdvice("www.example.com", "/files/docs");
	redirectingAdvice("www.example.com", "/files/docs/untitled.docx");

	redirectingAdvice("www.first.com", "/second");
	redirectingAdvice("www.first.com", "/third");
	redirectingAdvice("www.first.com", "/second/third");
	redirectingAdvice("www.second.com", "/third");

	redirectingAdvice("10.10.10.1", "/home");
	redirectingAdvice("10.10.10.1", "/home/page");
	redirectingAdvice("10.10.10.1", "/ws");
	redirectingAdvice("10.10.10.1", "/ws/controller");
	redirectingAdvice("10.10.10.1", "/strict");
	redirectingAdvice("10.10.10.1", "/strict/rule");
	redirectingAdvice("www.example.com", "/strict");
	redirectingAdvice("www.example.com", "/strict/rule");

	redirectingAdvice("cyclic1.example.com", "/cyclic");
	redirectingAdvice("cyclic2.example.com", "/cyclic");

	redirectingAdvice("random.example.com", "/");
	redirectingAdvice("localhost", "/");
	redirectingAdvice("localhost", "/home");
	redirectingAdvice("localhost", "/home/strict");
}

static void initAndStart(
	const std::unordered_map<
		string, std::unordered_map<
			string, std::vector<
				string
			>
		>
	>& config)
{
	const auto& configRules = config.at("rules");

	// Store config data
	for(const auto& [redirectTo, redirectFroms] : configRules)
	{
		const string redirectToStr = redirectTo;
		string redirectToHost, redirectToPath;
		auto pathIdx = redirectToStr.find('/');
		if(pathIdx != string::npos)
		{
			redirectToHost = redirectToStr.substr(0, pathIdx);
			redirectToPath = redirectToStr.substr(pathIdx);
		}
		else
			redirectToPath = '/';

		auto toIdx = rulesTo_.size();
		rulesTo_.push_back({
			.host = std::move(redirectToHost.empty() && pathIdx != 0 ? redirectToStr : redirectToHost),
			.path = std::move(redirectToPath),
		});

		for(const auto& redirectFrom : redirectFroms)
		{
			string redirectFromStr = redirectFrom;
			auto len = redirectFromStr.size();
			bool isWildcard = false;
			if(len > 1 && redirectFromStr[len - 2] == '/' && redirectFromStr[len - 1] == '*')
			{
				redirectFromStr.resize(len - 2);
				isWildcard = true;
			}

			string redirectFromHost, redirectFromPath;
			pathIdx = redirectFromStr.find_first_of('/');
			if (pathIdx != string::npos)
			{
				redirectFromHost = redirectFromStr.substr(0, pathIdx);
				redirectFromPath = redirectFromStr.substr(pathIdx);
			}
			else
				redirectFromPath = '/';

			rulesFromData_.push_back({
				.host = std::move(redirectFromHost.empty() && pathIdx != 0 ? redirectFromStr
												: redirectFromHost),
				.path = std::move(redirectFromPath),
				.isWildcard = isWildcard,
				.toIdx = toIdx,
			});
		}
	}

	struct RedirectDepthGroup
	{
		std::vector<const RedirectFrom*> fromData;
		size_t maxPathLen;
	};

	std::unordered_map<RedirectGroup*, RedirectDepthGroup> leafs, leafsBackbuffer; // Keep track of most recent leaf nodes

	// Find minimum required path length for each host
	for(const auto& redirectFrom : rulesFromData_)
	{
		const auto& path = redirectFrom.path;
		auto& rule = rulesFrom_[redirectFrom.host];
		if(path == "/") // Root rules are part of the host group
			continue;

		auto len = path.size();
		if(len < rule.maxPathLen)
			rule.maxPathLen = len;
	}

	// Create initial leaf nodes
	for(const auto& redirectFrom : rulesFromData_)
	{
		string_view path = redirectFrom.path;
		auto& rule = rulesFrom_[redirectFrom.host];
		if(path == "/")
		{
			(redirectFrom.isWildcard ? rule.wildcard : rule.to) = &rulesTo_[redirectFrom.toIdx];
			continue;
		}

		size_t maxLen = rule.maxPathLen;

		string_view pathGroup = path.substr(0, maxLen);

		auto& groups = rule.groups;
		auto find = groups.find(pathGroup);
		RedirectGroup* group;
		if(find != groups.end())
			group = find->second;
		else
		{
			group = new RedirectGroup;
			groups[pathGroup] = group;
		}

		if(path.size() == maxLen) // Reached the end
		{
			(redirectFrom.isWildcard ? group->wildcard : group->to) = &rulesTo_[redirectFrom.toIdx];
			group->maxPathLen = 0;
			continue; // No need to queue this node up, it reached the end
		}

		auto& leaf = leafs[group];
		leaf.fromData.push_back(&redirectFrom);
		leaf.maxPathLen = maxLen;
	}

	// Populate subsequent leaf nodes
	while(!leafs.empty())
	{
		for(auto& [group, depthGroup] : leafs)
		{
			size_t minIdx = depthGroup.maxPathLen, maxIdx = string::npos;
			const auto& fromData = depthGroup.fromData;
			for(const auto& redirectFrom : fromData)
			{
				auto len = redirectFrom->path.size();
				if(len >= minIdx && len < maxIdx)
					maxIdx = len;
			}

			size_t maxLen = maxIdx - minIdx;
			group->maxPathLen = maxLen;
			for(const auto& redirectFrom : fromData)
			{
				string_view path = redirectFrom->path;
				string_view pathGroup = path.substr(minIdx, maxLen);

				auto& groups = group->groups;
				auto find = groups.find(pathGroup);
				RedirectGroup* childGroup;
				if(find != groups.end())
					childGroup = find->second;
				else
				{
					childGroup = new RedirectGroup;
					groups[pathGroup] = childGroup;
				}

				if(path.size() == maxIdx) // Reached the end
				{
					(redirectFrom->isWildcard ? childGroup->wildcard : childGroup->to) = &rulesTo_[redirectFrom->toIdx];
					childGroup->maxPathLen = 0;
					continue; // No need to queue this node up, it reached the end
				}

				auto& leaf = leafsBackbuffer[childGroup];
				leaf.fromData.push_back(redirectFrom);
				leaf.maxPathLen = maxIdx;
			}
		}

		leafs = leafsBackbuffer;
		leafsBackbuffer.clear();
	}
}

static void lookup(string& host, string& path)
{
	do
	{
		auto findHost = rulesFrom_.find(host);
		if(findHost == rulesFrom_.end())
			break;

		if(path == "/")
		{
			const RedirectGroup& group = findHost->second;
			const RedirectLocation* location = group.to;
			if(location) // Strict takes priority
			{
				host = location->host;
				path = location->path;
				continue;
			}
			location = group.wildcard;
			if(location) // If strict didn't match, then it could match wildcard
			{
				host = location->host;
				path = location->path;
				continue;
			}
			break; // Neither exist for root
		}

		bool isWildcard = true;
		const RedirectLocation* to = nullptr;
		size_t lastWildcardPathViewLen = 0;
		std::unordered_map<string_view, RedirectGroup*>::const_iterator find;
		string_view pathView = path;
		for(const RedirectGroup* group = &(findHost->second); ; group = find->second)
		{
			const RedirectLocation* location = group->wildcard;
			bool pathIsExhausted = pathView.empty();
			if(location && (pathIsExhausted || pathView.front() == '/'))
			{
				to = location;
				lastWildcardPathViewLen = pathView.size();
			}

			const auto& groups = group->groups;
			auto maxPathLen = group->maxPathLen;
			if(!maxPathLen || pathIsExhausted) // Final node or path is shorter than tree
			{
				if(!pathIsExhausted) // Path is longer than tree
					break;

				location = group->to;
				if(location)
				{
					to = location;
					isWildcard = false;
				}
				break;
			}

			find = groups.find(pathView.substr(0, maxPathLen));
			if(find == groups.end()) // Cannot go deeper
				break;

			pathView = pathView.substr(maxPathLen);
		}

		if(to)
		{
			host = to->host;
			string newPath = to->path;
			if(isWildcard)
			{
				auto start = path.size() - lastWildcardPathViewLen;
				newPath.append(path.substr(start + (newPath.back() == '/' && path[start] == '/')));
			}
			path = newPath;
		}
		else break;
	} while(true);
}

static void redirectingAdvice(const string& reqHost, const string& reqPath)
{
	string host, path = reqPath;

	// Lookup within non-host rules first
	lookup(host, path);
	if(host.empty()) // Not altered, meaning only path could have changed
	{
		host = reqHost;
		lookup(host, path);
	}

	bool hostChanged = host != reqHost;
	if(hostChanged || path != reqPath)
	{
		if(hostChanged)
			cout << reqHost << reqPath << " -> " << host << path << '\n';
		else
			cout << reqHost << reqPath << " -> " << path << '\n';
	}
	else
		cout << "'" << reqHost << reqPath << "' not found, forward.\n";
}

static void recurse(const RedirectGroup* parent, size_t tabs = 1)
{
	const auto more = tabs + 1;
	for(const auto& [groupPath, group] : parent->groups)
	{
		for(auto t = tabs; t; --t)
			cout << '\t';

		cout << groupPath << '\n';

		recurse(group, more);
	}
}
static void printMap()
{
	for(const auto& rule : rulesFrom_)
	{
		cout << rule.first << ":\n";
		recurse(&rule.second);
	}
}
