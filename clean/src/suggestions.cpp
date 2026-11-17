#include "headers.h"
#include "suggestions.h"

#include <limits>
#include <numeric>
#include <set>

namespace clean
{
	namespace
	{
		std::string path_text(const fs::path& path)
		{
			auto text = path.generic_u8string();
			return std::string(text.begin(), text.end());
		}

		char lower_ascii(char c)
		{
			return c >= 'A' && c <= 'Z' ? c - 'A' + 'a' : c;
		}
	}

	SuggestionModel::Name SuggestionModel::read_name(const fs::path& path)
	{
		Name name;
		name.extension = path_text(path.extension());
		std::transform(name.extension.begin(), name.extension.end(), name.extension.begin(), lower_ascii);
		name.folder = path_text(path.parent_path());

		// Split punctuation, whitespace and number runs so months and versions can vary.
		std::string word;
		for (unsigned char c : path_text(path.stem()))
		{
			if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c >= 128)
				word += lower_ascii(static_cast<char>(c));
			else if (!word.empty())
			{
				name.words.push_back(std::move(word));
				word.clear();
			}
		}
		if (!word.empty()) name.words.push_back(std::move(word));
		std::sort(name.words.begin(), name.words.end());
		name.words.erase(std::unique(name.words.begin(), name.words.end()), name.words.end());
		return name;
	}

	double SuggestionModel::name_similarity(const Name& a, const Name& b)
	{
		size_t shared = 0;
		auto left = a.words.begin(), right = b.words.begin();
		while (left != a.words.end() && right != b.words.end())
		{
			if (*left < *right) ++left;
			else if (*right < *left) ++right;
			else { ++shared; ++left; ++right; }
		}
		if (!shared) return 0;
		return static_cast<double>(shared) / (a.words.size() + b.words.size() - shared);
	}

	SuggestionModel::SuggestionModel(const std::vector<MoveExample>& observations)
	{
		std::map<fs::path, size_t> latest;
		for (const auto& move : observations)
		{
			auto [it, inserted] = latest.try_emplace(move.file.path, examples.size());
			if (inserted) examples.push_back({ move, read_name(move.file.path) });
			else examples[it->second].move = move;
		}

		std::set<fs::path> destinations;
		std::vector<size_t> approved;
		for (size_t i = 0; i < examples.size(); ++i)
		{
			destinations.insert(examples[i].move.destination);
			if (examples[i].move.approved) approved.push_back(i);
		}
		cluster_names(std::min<size_t>(8, destinations.size()));
		approvalTree = train_tree(std::move(approved), 0);
	}

	double SuggestionModel::dot(const Name& name, const Center& center)
	{
		if (name.words.empty()) return 0;
		double sum = 0;
		for (const auto& word : name.words)
		{
			auto found = center.weights.find(word);
			if (found != center.weights.end()) sum += found->second;
		}
		return sum / std::sqrt(static_cast<double>(name.words.size()));
	}

	size_t SuggestionModel::nearest_cluster(const Name& name) const
	{
		size_t nearest = 0;
		double best = std::numeric_limits<double>::infinity();
		for (size_t i = 0; i < centers.size(); ++i)
		{
			double distance = centers[i].squaredLength - 2 * dot(name, centers[i]);
			if (distance < best)
			{
				best = distance;
				nearest = i;
			}
		}
		return nearest;
	}

	void SuggestionModel::cluster_names(size_t count)
	{
		// Pick distinct seeds far apart, then alternate assignment and centroid updates.
		while (centers.size() < count)
		{
			const Name* seed = nullptr;
			double farthest = 0.000001;
			for (const auto& example : examples)
			{
				if (example.name.words.empty()) continue;
				if (centers.empty()) { seed = &example.name; break; }
				const auto& center = centers[nearest_cluster(example.name)];
				double distance = 1 + center.squaredLength - 2 * dot(example.name, center);
				if (distance > farthest)
				{
					farthest = distance;
					seed = &example.name;
				}
			}
			if (!seed) break;
			Center center;
			double weight = 1 / std::sqrt(static_cast<double>(seed->words.size()));
			for (const auto& word : seed->words) center.weights[word] = weight;
			center.squaredLength = 1;
			centers.push_back(std::move(center));
		}
		if (centers.empty()) return;

		for (int iteration = 0; iteration < 30; ++iteration)
		{
			std::vector<Center> next(centers.size());
			std::vector<size_t> counts(centers.size());
			bool changed = iteration == 0;
			for (auto& example : examples)
			{
				if (example.name.words.empty()) continue;
				size_t nearest = nearest_cluster(example.name);
				if (nearest != example.cluster) changed = true;
				example.cluster = nearest;
				counts[nearest]++;
				double weight = 1 / std::sqrt(static_cast<double>(example.name.words.size()));
				for (const auto& word : example.name.words) next[nearest].weights[word] += weight;
			}
			if (!changed) break;
			for (size_t i = 0; i < centers.size(); ++i)
			{
				if (!counts[i]) continue;
				for (auto& [word, weight] : next[i].weights)
				{
					weight /= counts[i];
					next[i].squaredLength += weight * weight;
				}
				centers[i] = std::move(next[i]);
			}
		}
		for (auto& example : examples) example.cluster = nearest_cluster(example.name);
	}

	bool SuggestionModel::Rule::matches(const Name& name) const
	{
		if (feature == Feature::Folder) return name.folder == value;
		if (feature == Feature::Extension) return name.extension == value;
		return std::binary_search(name.words.begin(), name.words.end(), value);
	}

	double SuggestionModel::impurity(const std::vector<size_t>& indices) const
	{
		std::map<fs::path, size_t> counts;
		for (size_t i : indices) counts[examples[i].move.destination]++;
		double result = 1;
		for (const auto& [destination, count] : counts)
		{
			double share = static_cast<double>(count) / indices.size();
			result -= share * share;
		}
		return result;
	}

	std::unique_ptr<SuggestionModel::Node> SuggestionModel::train_tree(std::vector<size_t> indices, int depth) const
	{
		if (indices.empty()) return nullptr;
		auto node = std::make_unique<Node>();
		double best = impurity(indices);
		if (depth < 6 && best > 0.000001)
		{
			std::set<std::pair<Feature, std::string>> rules;
			for (size_t i : indices)
			{
				const auto& name = examples[i].name;
				rules.emplace(Feature::Folder, name.folder);
				rules.emplace(Feature::Extension, name.extension);
				for (const auto& word : name.words) rules.emplace(Feature::Word, word);
			}
			std::vector<size_t> bestYes, bestNo;
			for (const auto& [feature, value] : rules)
			{
				Rule rule{ feature, value };
				std::vector<size_t> yes, no;
				for (size_t i : indices)
					(rule.matches(examples[i].name) ? yes : no).push_back(i);
				if (yes.empty() || no.empty()) continue;
				double mixed = (impurity(yes) * yes.size() + impurity(no) * no.size()) / indices.size();
				if (mixed + 0.000001 >= best) continue;
				best = mixed;
				node->rule = std::move(rule);
				bestYes = std::move(yes);
				bestNo = std::move(no);
			}
			if (!bestYes.empty())
			{
				node->yes = train_tree(std::move(bestYes), depth + 1);
				node->no = train_tree(std::move(bestNo), depth + 1);
				return node;
			}
		}
		node->examples = std::move(indices);
		return node;
	}

	MoveSuggestion SuggestionModel::vote(const FileRecord& file, const Name& name, size_t cluster,
		const std::vector<size_t>& indices, bool namesOnly, const char* reason) const
	{
		struct Vote
		{
			double weight = 0;
			size_t count = 0;
			std::vector<fs::path> samples;
		};
		std::map<fs::path, Vote> votes;
		double total = 0;
		for (size_t i : indices)
		{
			const auto& example = examples[i];
			if (file.path == example.move.file.path || name.extension != example.name.extension) continue;
			double weight = 1;
			if (namesOnly)
			{
				double similarity = name_similarity(name, example.name);
				if (similarity < 0.25) continue;
				auto size_difference = std::abs(std::log2(1.0 + file.size) - std::log2(1.0 + example.move.file.size));
				weight = 2 * similarity + 0.1 / (1 + size_difference);
				if (name.folder == example.name.folder) weight += 0.2;
				if (cluster == example.cluster) weight += 0.15;
			}
			auto& vote = votes[example.move.destination];
			vote.count++;
			vote.weight += weight;
			total += weight;
			if (vote.samples.size() < 2) vote.samples.push_back(example.move.file.path.filename());
		}
		for (const auto& [destination, vote] : votes)
		{
			if (vote.count < 2 || vote.weight < 0.75 * total) continue;
			std::string details = std::string(reason) + " (" + std::to_string(vote.count) + " examples";
			if (namesOnly)
				details += ": " + path_text(vote.samples[0]) + ", " + path_text(vote.samples[1]);
			return { destination, details + ")" };
		}
		return {};
	}

	MoveSuggestion SuggestionModel::suggest(const FileRecord& file) const
	{
		Name name = read_name(file.path);
		const Node* leaf = approvalTree.get();
		while (leaf && leaf->yes)
			leaf = leaf->rule.matches(name) ? leaf->yes.get() : leaf->no.get();

		size_t cluster = nearest_cluster(name);
		if (leaf)
		{
			auto suggestion = vote(file, name, cluster, leaf->examples, true, "Matches approved filename patterns");
			if (!suggestion.destination.empty()) return suggestion;
		}
		std::vector<size_t> all(examples.size());
		std::iota(all.begin(), all.end(), 0);
		auto suggestion = vote(file, name, cluster, all, true, "Similar filenames");
		if (!suggestion.destination.empty()) return suggestion;

		std::vector<size_t> sameFolder;
		for (size_t i : all)
			if (file.path != examples[i].move.file.path && name.folder == examples[i].name.folder
				&& name.extension == examples[i].name.extension)
				sameFolder.push_back(i);
		if (sameFolder.size() >= 2)
			suggestion = vote(file, name, cluster, sameFolder, false, "Same extension and source folder");
		else
			suggestion = vote(file, name, cluster, all, false, "Same extension");
		if (suggestion.destination.empty()) return {};

		// A broad extension match should not override a conflicting filename clue.
		for (const auto& example : examples)
			if (file.path != example.move.file.path && name.extension == example.name.extension &&
				example.move.destination != suggestion.destination && name_similarity(name, example.name) >= 0.25)
				return {};
		return suggestion;
	}

	size_t SuggestionModel::cluster_count() const
	{
		return centers.size();
	}

	SuggestionPoint SuggestionModel::plot(const Name& name, std::string label, bool approved, bool proposed) const
	{
		size_t cluster = centers.size();
		double similarity = 0;
		if (!name.words.empty() && !centers.empty())
		{
			cluster = nearest_cluster(name);
			const auto& center = centers[cluster];
			similarity = std::clamp(dot(name, center) / std::sqrt(center.squaredLength), 0.0, 1.0);
		}
		return { cluster, similarity, std::move(label), approved, proposed };
	}

	std::vector<SuggestionPoint> SuggestionModel::plot_examples() const
	{
		std::vector<SuggestionPoint> points;
		for (const auto& example : examples)
		{
			auto label = path_text(example.move.file.path) + " to " + path_text(example.move.destination);
			points.push_back(plot(example.name, std::move(label), example.move.approved, false));
		}
		return points;
	}

	SuggestionPoint SuggestionModel::plot_suggestion(const FileRecord& file, const MoveSuggestion& suggestion) const
	{
		auto label = path_text(file.path) + " to " + path_text(suggestion.destination) + ": " + suggestion.reason;
		return plot(read_name(file.path), std::move(label), false, true);
	}
}
