#pragma once

#include <cstdint>
#include <filesystem>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace clean
{
	namespace fs = std::filesystem;

	struct FileRecord
	{
		fs::path path;
		uintmax_t size = 0;
		long long modified = 0;
	};

	struct MoveExample
	{
		FileRecord file;
		fs::path destination;
		bool approved = false;
	};

	struct MoveSuggestion
	{
		fs::path destination;
		std::string reason;
	};

	struct SuggestionPoint
	{
		size_t cluster = 0;
		double similarity = 0;
		std::string label;
		bool approved = false;
		bool proposed = false;
	};

	class SuggestionModel
	{
	public:
		explicit SuggestionModel(const std::vector<MoveExample>& observations);

		MoveSuggestion suggest(const FileRecord& file) const;
		std::vector<SuggestionPoint> plot_examples() const;
		SuggestionPoint plot_suggestion(const FileRecord& file, const MoveSuggestion& suggestion) const;
		size_t cluster_count() const;

	private:
		struct Name
		{
			std::vector<std::string> words;
			std::string extension;
			std::string folder;
		};

		struct Example
		{
			MoveExample move;
			Name name;
			size_t cluster = 0;
		};

		struct Center
		{
			std::map<std::string, double> weights;
			double squaredLength = 0;
		};

		enum class Feature { Folder, Extension, Word };

		struct Rule
		{
			Feature feature = Feature::Word;
			std::string value;

			bool matches(const Name& name) const;
		};

		struct Node
		{
			Rule rule;
			std::unique_ptr<Node> yes;
			std::unique_ptr<Node> no;
			std::vector<size_t> examples;
		};

		std::vector<Example> examples;
		std::vector<Center> centers;
		std::unique_ptr<Node> approvalTree;

		static Name read_name(const fs::path& path);
		static double name_similarity(const Name& a, const Name& b);
		static double dot(const Name& name, const Center& center);
		size_t nearest_cluster(const Name& name) const;
		void cluster_names(size_t count);
		std::unique_ptr<Node> train_tree(std::vector<size_t> indices, int depth) const;
		double impurity(const std::vector<size_t>& indices) const;
		MoveSuggestion vote(const FileRecord& file, const Name& name, size_t cluster,
			const std::vector<size_t>& indices, bool namesOnly, const char* reason) const;
		SuggestionPoint plot(const Name& name, std::string label, bool approved, bool proposed) const;
	};
}
