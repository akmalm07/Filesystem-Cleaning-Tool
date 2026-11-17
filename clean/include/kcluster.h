#pragma once

#include <array>
#include <filesystem>
#include <map>
#include <set>
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

	struct FileChange
	{
		std::string kind;
		FileRecord file;
		fs::path destination;
	};

	class KCluster
	{
	public:
		explicit KCluster(const fs::path& root);

		void listen(bool once, int interval);
		void execute(bool automatic);

	private:
		using Point = std::array<double, 17>;

		struct Observation
		{
			FileRecord file;
			fs::path destination;
		};

		struct Center
		{
			Point point{};
			fs::path destination;
			int members = 0;
			double confidence = 0;
		};

		fs::path root;
		fs::path stateDir;
		std::map<fs::path, FileRecord> snapshot;
		std::vector<Observation> observations;
		std::vector<FileChange> changes;
		std::set<std::pair<fs::path, fs::path>> declined;
		int accepted = 0;
		int rejected = 0;
		bool initialized = false;

		void load();
		void save() const;
		std::map<fs::path, FileRecord> scan() const;
		void poll();
		void write_graph(const std::vector<Center>& centers) const;
		std::vector<Center> cluster() const;
		fs::path suggest(const FileRecord& file, const std::vector<Center>& centers) const;
		static Point features(const FileRecord& file);
		static double distance(const Point& a, const Point& b);
		bool valid_relative_path(const fs::path& path) const;
	};
}
