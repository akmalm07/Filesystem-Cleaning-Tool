#include "headers.h"
#include "kcluster.h"

#include <iomanip>
#include <limits>
#include <set>
#include <stdexcept>

namespace clean
{
	namespace
	{
		std::string escape_svg(std::string_view value)
		{
			std::string result;
			for (char c : value)
			{
				if (c == '&') result += "&amp;";
				else if (c == '<') result += "&lt;";
				else if (c == '>') result += "&gt;";
				else if (c == '"') result += "&quot;";
				else result += c;
			}
			return result;
		}
	}

	KCluster::KCluster(const fs::path& directory)
		: root(fs::weakly_canonical(directory)), stateDir(root / ".clean")
	{
		if (!fs::is_directory(root))
			throw std::runtime_error("Run clean from a directory.");
		load();
	}

	bool KCluster::valid_relative_path(const fs::path& path) const
	{
		if (path.empty() || path.is_absolute() || path.has_root_name()) return false;
		for (const auto& part : path)
			if (part == ".." || part == ".clean" || part == ".git") return false;
		return true;
	}

	void KCluster::load()
	{
		std::ifstream file(stateDir / "state.txt");
		if (!file) return;
		initialized = true;

		std::string version;
		file >> version >> accepted >> rejected;
		if (!file || version != "CLEAN1") throw std::runtime_error("Unknown clean state version.");

		char type;
		while (file >> type)
		{
			std::string path, target, kind;
			FileRecord record;
			if (type == 'R')
			{
				file >> std::quoted(path) >> std::quoted(target);
				if (!file || !valid_relative_path(fs::path(path)) || !valid_relative_path(fs::path(target))) throw std::runtime_error("Invalid clean state.");
				declined.insert({ fs::path(path), fs::path(target) });
			}
			else if (type == 'S' || type == 'O' || type == 'C')
			{
				file >> std::quoted(path) >> record.size >> record.modified;
				record.path = fs::path(path);
				if (!file || !valid_relative_path(record.path)) throw std::runtime_error("Invalid clean state.");
				if (type == 'S') snapshot[record.path] = record;
				else
				{
					if (type == 'O')
					{
						file >> std::quoted(target);
						if (!file || !valid_relative_path(fs::path(target))) throw std::runtime_error("Invalid clean state.");
						observations.push_back({ record, fs::path(target) });
					}
					else
					{
						file >> std::quoted(kind) >> std::quoted(target);
						if (!file || (!target.empty() && !valid_relative_path(fs::path(target)))) throw std::runtime_error("Invalid clean state.");
						changes.push_back({ kind, record, fs::path(target) });
					}
				}
			}
			else throw std::runtime_error("Invalid clean state.");
		}
	}

	void KCluster::save() const
	{
		fs::create_directories(stateDir);
		fs::path temporary = stateDir / "state.tmp";
		std::ofstream file(temporary, std::ios::trunc);
		if (!file) throw std::runtime_error("Could not save clean state.");
		file << "CLEAN1 " << accepted << ' ' << rejected << '\n';
		for (const auto& [path, record] : snapshot)
			file << "S " << std::quoted(path.generic_string()) << ' ' << record.size << ' ' << record.modified << '\n';
		for (const auto& observation : observations)
			file << "O " << std::quoted(observation.file.path.generic_string()) << ' ' << observation.file.size << ' ' << observation.file.modified << ' ' << std::quoted(observation.destination.generic_string()) << '\n';
		for (const auto& change : changes)
			file << "C " << std::quoted(change.file.path.generic_string()) << ' ' << change.file.size << ' ' << change.file.modified << ' ' << std::quoted(change.kind) << ' ' << std::quoted(change.destination.generic_string()) << '\n';
		for (const auto& [path, destination] : declined)
			file << "R " << std::quoted(path.generic_string()) << ' ' << std::quoted(destination.generic_string()) << '\n';
		file.close();
		if (!file) throw std::runtime_error("Could not save clean state.");
		fs::path current = stateDir / "state.txt";
		if (fs::exists(current)) fs::remove(current);
		fs::rename(temporary, current);
	}

	std::map<fs::path, FileRecord> KCluster::scan() const
	{
		std::map<fs::path, FileRecord> files;
		fs::recursive_directory_iterator it(root, fs::directory_options::skip_permission_denied), end;
		for (; it != end; ++it)
		{
			std::error_code ec;
			if (it->is_symlink(ec))
			{
				it.disable_recursion_pending();
				continue;
			}
			fs::path relative = it->path().lexically_relative(root);
			if (it->is_directory(ec))
			{
				if (relative == ".clean" || relative == ".git") it.disable_recursion_pending();
				continue;
			}
			if (!it->is_regular_file(ec) || relative.parent_path().empty() || !valid_relative_path(relative)) continue;
			auto size = it->file_size(ec);
			if (ec) continue;
			auto modified = it->last_write_time(ec);
			if (ec) continue;
			files[relative] = { relative, size, modified.time_since_epoch().count() };
		}
		return files;
	}

	KCluster::Point KCluster::features(const FileRecord& file)
	{
		Point point{};
		std::string extension = file.path.extension().string();
		std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
		unsigned int hash = 2166136261u;
		for (unsigned char c : extension) hash = (hash ^ c) * 16777619u;
		point[hash % 16] = 2.0;
		point[16] = std::log2(static_cast<double>(file.size) + 1.0) / 20.0;
		return point;
	}

	double KCluster::distance(const Point& a, const Point& b)
	{
		double total = 0;
		for (size_t i = 0; i < a.size(); ++i) total += (a[i] - b[i]) * (a[i] - b[i]);
		return total;
	}

	std::vector<KCluster::Center> KCluster::cluster() const
	{
		std::vector<Center> centers;
		std::set<fs::path> destinations;
		for (const auto& observation : observations) destinations.insert(observation.destination);
		if (destinations.empty()) return centers;
		for (const auto& destination : destinations)
		{
			if (centers.size() == 8) break;
			for (const auto& observation : observations)
				if (observation.destination == destination)
				{
					centers.push_back({ features(observation.file), destination });
					break;
				}
		}

		std::vector<size_t> assignment(observations.size());
		for (int iteration = 0; iteration < 30; ++iteration)
		{
			std::vector<Point> sums(centers.size());
			std::vector<int> counts(centers.size());
			for (size_t i = 0; i < observations.size(); ++i)
			{
				Point point = features(observations[i].file);
				size_t best = 0;
				for (size_t j = 1; j < centers.size(); ++j)
					if (distance(point, centers[j].point) < distance(point, centers[best].point)) best = j;
				assignment[i] = best;
				counts[best]++;
				for (size_t n = 0; n < point.size(); ++n) sums[best][n] += point[n];
			}
			bool unchanged = true;
			for (size_t j = 0; j < centers.size(); ++j)
			{
				if (!counts[j]) continue;
				for (size_t n = 0; n < sums[j].size(); ++n) sums[j][n] /= counts[j];
				if (distance(sums[j], centers[j].point) > 0.000001) unchanged = false;
				centers[j].point = sums[j];
			}
			if (unchanged) break;
		}

		for (size_t j = 0; j < centers.size(); ++j)
		{
			std::map<fs::path, int> votes;
			for (size_t i = 0; i < observations.size(); ++i)
				if (assignment[i] == j) votes[observations[i].destination]++;
			for (const auto& [destination, count] : votes)
			{
				centers[j].members += count;
				if (count > centers[j].confidence)
				{
					centers[j].confidence = static_cast<double>(count);
					centers[j].destination = destination;
				}
			}
			if (centers[j].members) centers[j].confidence /= centers[j].members;
		}
		return centers;
	}

	fs::path KCluster::suggest(const FileRecord& file, const std::vector<Center>& centers) const
	{
		if (centers.empty()) return {};
		Point point = features(file);
		size_t best = 0;
		for (size_t j = 1; j < centers.size(); ++j)
			if (distance(point, centers[j].point) < distance(point, centers[best].point)) best = j;
		const Center& center = centers[best];
		if (center.members < 2 || center.confidence < 0.75 || distance(point, center.point) > 1.0) return {};
		if (file.path.parent_path() == center.destination) return {};
		return center.destination;
	}

	void KCluster::poll()
	{
		auto current = scan();
		std::vector<FileRecord> removed, added;
		for (const auto& [path, old] : snapshot)
		{
			auto found = current.find(path);
			if (found == current.end()) removed.push_back(old);
			else if (found->second.size != old.size || found->second.modified != old.modified)
				changes.push_back({ "modified", found->second, {} });
		}
		for (const auto& [path, file] : current)
			if (!snapshot.contains(path)) added.push_back(file);

		std::vector<bool> used(added.size());
		for (const auto& old : removed)
		{
			std::vector<size_t> matches;
			for (size_t i = 0; i < added.size(); ++i)
				if (!used[i] && old.size == added[i].size && old.modified == added[i].modified && old.path.extension() == added[i].path.extension()) matches.push_back(i);
			if (matches.size() > 1)
			{
				std::vector<size_t> named;
				for (size_t match : matches)
					if (old.path.filename() == added[match].path.filename()) named.push_back(match);
				matches = std::move(named);
			}
			if (matches.size() == 1 && old.path.parent_path() != added[matches[0]].path.parent_path())
			{
				const auto& moved = added[matches[0]];
				observations.push_back({ old, moved.path.parent_path() });
				changes.push_back({ "moved", old, moved.path });
				used[matches[0]] = true;
			}
			else changes.push_back({ "deleted", old, {} });
		}
		for (size_t i = 0; i < added.size(); ++i)
			if (!used[i]) changes.push_back({ "added", added[i], {} });

		snapshot = std::move(current);
		save();
		write_graph(cluster());
		std::cout << "Scanned " << snapshot.size() << " files. Learned " << observations.size() << " moves. Graph: " << stateDir / "graph.svg" << std::endl;
	}

	void KCluster::listen(bool once, int interval)
	{
		if (!initialized)
		{
			snapshot = scan();
			save();
			initialized = true;
			write_graph(cluster());
			std::cout << "Baseline saved for " << snapshot.size() << " files. Watching subdirectories only.\n";
			if (once) return;
		}
		if (once) poll();
		else
		{
			std::cout << "Listening every " << interval << " seconds. Press Ctrl+C to stop.\n";
			while (true)
			{
				std::this_thread::sleep_for(std::chrono::seconds(interval));
				poll();
			}
		}
	}

	void KCluster::execute(bool automatic)
	{
		if (automatic && (accepted < 3 || accepted < 3 * rejected))
			throw std::runtime_error("Automatic mode needs at least 3 accepted suggestions and 3 accepts per rejection.");
		auto centers = cluster();
		if (centers.empty())
		{
			std::cout << "No moves learned yet. Run clean listen and organize files inside subdirectories first.\n";
			return;
		}

		auto current = scan();
		int suggested = 0, moved = 0;
		for (const auto& [path, record] : current)
		{
			fs::path destination = suggest(record, centers);
			if (destination.empty() || !valid_relative_path(destination) || declined.contains({ path, destination })) continue;
			fs::path target = root / destination / path.filename();
			fs::path parent = root;
			bool unsafe = false;
			for (const auto& part : destination)
			{
				parent /= part;
				if (fs::is_symlink(parent)) { unsafe = true; break; }
			}
			if (unsafe) continue;
			if (fs::exists(target) || fs::is_symlink(target)) continue;
			suggested++;
			bool approved = automatic;
			if (!automatic)
			{
				std::string answer;
				while (true)
				{
					std::cout << "Move " << path << " to " << destination << "? [y/n/q] " << std::flush;
					if (!std::getline(std::cin, answer) || answer == "q" || answer == "Q") break;
					if (answer == "y" || answer == "Y" || answer == "n" || answer == "N") break;
					std::cout << "Please enter y, n, or q.\n";
				}
				if (!std::cin || answer == "q" || answer == "Q") break;
				approved = answer == "y" || answer == "Y";
				if (!approved)
				{
					rejected++;
					declined.insert({ path, destination });
					save();
					continue;
				}
			}
			if (!fs::is_regular_file(root / path) || fs::is_symlink(root / path) || fs::exists(target) || fs::is_symlink(target)) continue;
			fs::create_directories(target.parent_path());
			fs::rename(root / path, target);
			accepted++;
			moved++;
			observations.push_back({ record, destination });
			changes.push_back({ "moved", record, destination / path.filename() });
			snapshot.erase(path);
			FileRecord next = record;
			next.path = destination / path.filename();
			snapshot[next.path] = next;
			save();
			std::cout << "Moved " << path << " to " << next.path << '\n';
		}
		write_graph(cluster());
		std::cout << "Suggested " << suggested << ", moved " << moved << ". Accepted: " << accepted << ", rejected: " << rejected << ".\n";
	}

	void KCluster::write_graph(const std::vector<Center>& centers) const
	{
		fs::create_directories(stateDir);
		std::ofstream graph(stateDir / "graph.svg", std::ios::trunc);
		if (!graph) throw std::runtime_error("Could not write graph.");
		graph << "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"900\" height=\"540\" viewBox=\"0 0 900 540\">\n";
		graph << "<rect width=\"900\" height=\"540\" fill=\"#111827\"/><g fill=\"#e5e7eb\" font-family=\"Arial\">";
		graph << "<text x=\"40\" y=\"35\" font-size=\"20\">Clean: file changes and K-means clusters</text>";
		graph << "<text x=\"40\" y=\"510\">File size (log scale)</text><text x=\"40\" y=\"65\">Extension bucket</text></g>\n";
		graph << "<path d=\"M 60 80 V 470 H 850\" stroke=\"#9ca3af\" fill=\"none\"/>\n";
		const char* colors[] = { "#60a5fa", "#f472b6", "#34d399", "#fbbf24", "#a78bfa", "#fb7185", "#2dd4bf", "#f97316" };
		for (const auto& change : changes)
		{
			Point point = features(change.file);
			size_t bucket = 0;
			for (; bucket < 16 && point[bucket] == 0; ++bucket) {}
			double x = 60 + std::min(1.0, point[16]) * 790;
			double y = 470 - static_cast<double>(bucket) / 15.0 * 390;
			std::string color = "#9ca3af";
			if (!centers.empty())
			{
				size_t best = 0;
				for (size_t j = 1; j < centers.size(); ++j)
					if (distance(point, centers[j].point) < distance(point, centers[best].point)) best = j;
				color = colors[best];
			}
			graph << "<circle cx=\"" << x << "\" cy=\"" << y << "\" r=\"" << (change.kind == "moved" ? 6 : 4) << "\" fill=\"" << color << "\" opacity=\"0.8\"><title>";
			graph << escape_svg(change.kind + ": " + change.file.path.generic_string());
			if (!change.destination.empty()) graph << escape_svg(" to " + change.destination.generic_string());
			graph << "</title></circle>\n";
		}
		graph << "<g fill=\"#e5e7eb\" font-family=\"Arial\" font-size=\"12\">";
		for (size_t i = 0; i < centers.size(); ++i)
			graph << "<text x=\"" << 60 + (i % 4) * 200 << "\" y=\"" << 95 + (i / 4) * 18 << "\" fill=\"" << colors[i] << "\">" << escape_svg(centers[i].destination.generic_string()) << " (" << centers[i].members << " moves)</text>";
		graph << "</g></svg>\n";
	}
}
