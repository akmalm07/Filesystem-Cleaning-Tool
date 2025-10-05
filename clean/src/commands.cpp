#include "headers.h"
#include "commands.h"
#include "systems.h"

namespace clean
{

	fs::path normalize_path(const fs::path& path)
	{
		fs::path p = path;

#ifdef _WIN32
		std::string str = path.string();
		if (!path.is_absolute() && !str.empty() && str[0] == '/') 
	        p = fs::current_path() / str.substr(1);
		
#endif

		if (!p.is_absolute())
			p = fs::absolute(p);

		return fs::weakly_canonical(p);
	}


	std::optional<fs::path> validate_directory(fs::path& path, bool mustExist)
	{
		if (mustExist)
		{
			if (!fs::exists(path) || !fs::is_directory(path))
			{
				print_error("Directory does not exist or is not a directory: " + path.string());
				return std::nullopt;
			}
		}
		else
		{
			if (!fs::exists(path))
			{
				std::error_code ec;
				fs::create_directories(path, ec);
				if (ec)
				{
					print_error("Failed to create directory: " + path.string());
					return std::nullopt;
				}
			}
		}
		return path;
	}

	std::optional<uint8_t> parse_thread_count(std::string_view str)
	{
		try
		{
			int count = std::stoi(std::string(str));
			if (count < 1 || count > 8)
			{
				print_error("Thread count must be between 1 and 8.");
				return std::nullopt;
			}
			return static_cast<uint8_t>(count);
		}
		catch (...)
		{
			print_error("Invalid thread count specified.");
			return std::nullopt;
		}
	}

	bool try_initialize_git(const fs::path& destDir)
	{
		std::cout << "Attempting to initialize " << bold("GIT") << " repository in " << destDir << std::endl;

		if (sys::git_exists())
		{
			print_error("Git not installed. Proceeding without Git history (less efficient).");
			return false;
		}

		fs::current_path(destDir);
		auto cmd = std::string(sys::gitinit) + ">nul 2>nul";
		std::system(cmd.c_str());
		return true;
	}


	std::unique_ptr<Command> create_command(const char* command)
	{
		std::string cmdStr(command);

		if (cmdStr == "help" || cmdStr == "--help")
		{
			return std::make_unique<HelpCommand>();
		}
		else if (cmdStr == "version" || cmdStr == "--version")
		{
			return std::make_unique<VersionCommand>();
		}
		else if (cmdStr == "sort")
		{
			return std::make_unique<SortCommand>();
		}
		return nullptr;
	}

	void print_help()
	{
		std::cout 
			<< "Clean - A File Cleaning Utility" << std::endl << std::endl
			<< "Usage: clean [command] [options]" << std::endl << std::endl
			<< "Available Commands:" << std::endl << std::endl
			<< "\thelp        Show this help message" << std::endl << std::endl
			<< "\tversion     Show version information" << std::endl << std::endl
			<< "\tsort        Sorts all your files/direcotries by given options" << std::endl << std::endl
			<< "Use 'clean [command] --help' for more information on a command" << std::endl << std::endl
			<< std::endl << std::endl;
	}

	// Legacy function, may be removed in future
	std::expected <fs::path, bool> build_path(const fs::path& path)
	{
		if (fs::is_regular_file(path))
			return std::unexpected(true);

		fs::path newPath(fs::current_path());

		for (const auto& part : path)
		{
			if (part == ".")
				continue;
			else if (part == "..")
				newPath = newPath.parent_path();
			else if (fs::is_directory(part))
				newPath /= part;
			else if (!fs::exists(part))
			{
				try
				{
					fs::create_directory(part);
					newPath /= part;
				}
				catch (const fs::filesystem_error& e)
				{
					std::cout << red("Error: Unable to create directory ") << part << ". " << e.what() << std::endl;
					return std::unexpected(true);
				}
			}
			else
				return std::unexpected(true);
		}
	}

	void HelpCommand::execute(const std::vector<std::string_view>& options)
	{
		std::cout << "Help Command Executed" << std::endl;
		print_help();
	}

	void VersionCommand::execute(const std::vector<std::string_view>& options)
	{
		std::cout << "Clean version "<< version << std::endl;
	}

	void SortCommand::print_sort_help() const
	{
		std::cout
			<< "Usage: clean sort <source_directory> <destination_directory(defaults to current)> [options]\n\n"
			<< "Options:\n\n"
			<< "  The <num> options determine order priority when sorting by multiple attributes.\n"
			<< "  If only one attribute is selected, <num> will be ignored.\n\n"
			<< "\t--by-type <num>, -bt <num>     Sort by file type/extension\n"
			<< "\t--by-date <num>, -bd <num>     Sort by creation date\n"
			<< "\t--by-size <num>, -bs <num>     Sort by file size\n"
			<< "\t--see-example, -e              See example layout\n\n"
			<< "Non-hierarchical options:\n"
			<< "\t--no-copy, -nc                 Make shortcuts instead of copies\n"
			<< "\t--move, -m                     Move files instead of copying\n"
			<< "\t--overwrite, -o                Overwrite existing files\n"
			<< "\t--no-git, -ngit                Skip Git tracking\n"
			<< "\t--threads <num>, -t <num>      Number of threads (1–8)\n"
			<< "\t--help, -h                     Show this help message\n\n"
			<< "Example:\n"
			<< "\tclean sort --by-type /path/to/source /path/to/destination\n"
			<< std::endl;
	}

	void SortCommand::print_sort_example() const
	{
		std::cout << "Example Directory Structure:\n\n"
			<< "/source\n"
			<< "  /image1.jpg\n"
			<< "  /document1.pdf\n"
			<< "  /video1.mp4\n"
			<< "  /subfolder\n"
			<< "    /image2.png\n"
			<< "    /document2.docx\n\n"
			<< "After running:\n"
			<< "  clean sort /source /destination --by-type 1 --by-date 2 --move --threads 4\n\n"
			<< "/destination\n"
			<< "  /Images\n"
			<< "    /2023\n"
			<< "      /image1.jpg\n"
			<< "      /image2.png\n"
			<< "  /Documents\n"
			<< "    /2022\n"
			<< "      /document1.pdf\n"
			<< "      /document2.docx\n"
			<< "  /Videos\n"
			<< "    /2021\n"
			<< "      /video1.mp4\n\n"
			<< "Files are organized by type first, then by creation year within each type.\n"
			<< std::endl;
	}

	void SortCommand::execute(const std::vector<std::string_view>& options)
	{
		SortOptions optionsFlags = SortOptions::Null;
		std::optional<uint8_t> threadCount;

		auto it = options.begin();

		if (options.size() < 1)
		{
			print_error("Not enough arguments provided.");
			print_sort_help();
			return;
		}

		// --- Example Request ---
		if (it != options.end() && (*it == "-e" || *it == "--see-example"))
		{
			print_sort_example();
			return;
		}

		// --- Parse Source Directory ---
		fs::path sourceDir(normalize_path(options[0]));
		auto validSource = validate_directory(sourceDir, true);
		if (!validSource) return;

		// --- Destination Directory (optional) ---
		std::optional<fs::path> destDir;
		size_t optionStartIndex = 1;
		if (options.size() > 1 && options[1][0] != '-')
		{
			destDir = fs::path(normalize_path(options[1]));
			optionStartIndex++;
		}

		if (destDir)
		{
			auto validDest = validate_directory(*destDir, false);
			if (!validDest) return;
		}
		else
		{
			destDir = fs::current_path();
			std::cout << "Destination Directory: Current Directory\n";
		}

		// --- Option Parsing ---
		auto next_value = [&](size_t& i) -> std::string_view {
			return (i + 1 < options.size()) ? options[i + 1] : "";
			};

		for (size_t i = optionStartIndex; i < options.size(); ++i)
		{
			std::string_view opt = options[i];

			if (opt == "--help" || opt == "-h")
			{
				print_sort_help();
				return;
			}
			else if ((opt == "--by-type" || opt == "-bt") && !contains_any(optionsFlags, ByType))
			{
				optionsFlags |= set_enum<SortOptions::ByTypePrimary>(next_value(i), i);
			}
			else if ((opt == "--by-date" || opt == "-bd") && !contains_any(optionsFlags, ByDate))
			{
				optionsFlags |= set_enum<SortOptions::ByDatePrimary>(next_value(i), i);
			}
			else if ((opt == "--by-size" || opt == "-bs") && !contains_any(optionsFlags, BySize))
			{
				optionsFlags |= set_enum<SortOptions::BySizePrimary>(next_value(i), i);
			}
			else if (opt == "--move" || opt == "-m")
			{
				optionsFlags |= SortOptions::Move;
			}
			else if (opt == "--overwrite" || opt == "-o")
			{
				optionsFlags |= SortOptions::Overwrite;
			}
			else if (opt == "--no-git" || opt == "-ngit")
			{
				optionsFlags |= SortOptions::NoGit;
			}
			else if (opt == "--threads" || opt == "-t")
			{
				if (contains_any(optionsFlags, SortOptions::Threads))
					continue;

				if (i + 1 >= options.size())
				{
					print_error("No thread count specified.");
					print_sort_help();
					return;
				}

				optionsFlags |= SortOptions::Threads;
				threadCount = parse_thread_count(options[++i]);
				if (!threadCount) return;

				std::cout << "Thread Count set to: " << (int)threadCount.value() << std::endl;
			}
		}

		if (!contains_any(optionsFlags, ByType | ByDate | BySize))
		{
			print_error("No sorting attribute specified. Use --by-type, --by-date, or --by-size.");
			print_sort_help();
			return;
		}

		// --- Git Handling ---
		if (destDir.value() == sourceDir && !contains(optionsFlags & SortOptions::NoGit) && !try_initialize_git(destDir.value()))
		{
			std::cout << "Proceeding without Git support.\n";
			optionsFlags |= SortOptions::NoGit;
		}

		std::cout << "Sorting files from " << bold(sourceDir.string()) << " to " << bold(destDir.value().string()) << std::endl;

		// --- Debug: print enabled flags ---
		std::cout << "Enabled flags: " << std::hex << static_cast<uint32_t>(optionsFlags) << std::dec << std::endl;

		perform_sort_operation(sourceDir, destDir.value(), optionsFlags, threadCount);
	}


	void SortCommand::perform_sort_operation(const fs::path& sourceDir,
		const fs::path& destDir,
		SortOptions options,
		std::optional<uint8_t> threadCount)
	{
		using namespace std::chrono;

		std::cout << "Beginning file sort operation...\n";

		// --- Collect all files ---
		std::vector<fs::directory_entry> files;
		for (auto& entry : fs::recursive_directory_iterator(sourceDir))
		{
			if (entry.is_regular_file())
				files.push_back(entry);
		}

		std::cout << "Found " << files.size() << " files.\n";

		// --- No files to process ---
		if (files.empty())
		{
			std::cout << "No files found to process. Exiting.\n";
			return;
		}

		// --- Helper for categorizing files ---
		auto get_category_path = [&](const fs::path& file) -> fs::path
		{
			fs::path subdir;

			// --- Type sorting ---
			if (contains_any(options, SortOptions::ByTypePrimary | SortOptions::ByTypeSecondary | SortOptions::ByTypeTertiary))
			{
				std::string ext = file.extension().string();
				std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

				if (ext == ".jpg" || ext == ".jpeg" || ext == ".png" || ext == ".gif" || ext == ".bmp" || ext == ".tiff")
					subdir /= "Images";
				else if (ext == ".mp4" || ext == ".mov" || ext == ".avi" || ext == ".mkv" || ext == ".flv" || ext == ".wmv")
					subdir /= "Videos";
				else if (ext == ".mp3" || ext == ".wav" || ext == ".flac" || ext == ".aac" || ext == ".ogg")
					subdir /= "Audio";
				else if (ext == ".pdf" || ext == ".docx" || ext == ".doc" || ext == ".txt" || ext == ".rtf" || ext == ".odt")
					subdir /= "Documents";
				else if (ext == ".zip" || ext == ".rar" || ext == ".7z" || ext == ".tar" || ext == ".gz" || ext == ".bz2")
					subdir /= "Archives";
				else if (ext == ".cpp" || ext == ".h" || ext == ".hpp" || ext == ".c" || ext == ".py" || ext == ".js" || ext == ".java")
					subdir /= "Code";
				else if (ext == ".html" || ext == ".css" || ext == ".js" || ext == ".ts" || ext == ".json" || ext == ".xml")
					subdir /= "Web";
				else
					subdir /= "Other";
			}

			// --- Date sorting ---
			if (contains_any(options, SortOptions::ByDatePrimary | SortOptions::ByDateSecondary | SortOptions::ByDateTertiary))
			{
				auto ftime = fs::last_write_time(file);
				auto sctp = time_point_cast<system_clock::duration>(ftime - fs::file_time_type::clock::now() + system_clock::now());
				std::time_t tt = system_clock::to_time_t(sctp);
				std::tm local_tm{};
#ifdef _WIN32
				localtime_s(&local_tm, &tt);
#else
				localtime_r(&tt, &local_tm);
#endif
				subdir /= std::to_string(1900 + local_tm.tm_year);
			}

			// --- Size sorting (very rough, can be refined) ---
			if (contains_any(options, SortOptions::BySizePrimary | SortOptions::BySizeSecondary | SortOptions::BySizeTertiary))
			{
				auto size = fs::file_size(file);
				if (size < 1'000'000) subdir /= "Small";
				else if (size < 10'000'000) subdir /= "Medium";
				else subdir /= "Large";
			}

			return subdir;
		};

		std::mutex coutMutex;
		size_t threads = threadCount.value_or(1);
		if (threads < 1) threads = 1;
		if (threads > 8) threads = 8;

		// --- File moving/copying logic ---
		auto process_file = [&](const fs::directory_entry& entry)
		{
			fs::path categoryPath = get_category_path(entry);
			fs::path targetDir = destDir / categoryPath;

			fs::create_directories(targetDir);
			fs::path targetPath = targetDir / entry.path().filename();

			if (fs::exists(targetPath))
			{
				if (!contains_any(options, SortOptions::Overwrite))
				{
					std::lock_guard<std::mutex> lock(coutMutex);
					std::cout << "Skipping existing file: " << targetPath << "\n";
					return;
				}
			}

			try
			{
				if (contains_any(options, SortOptions::Move))
					fs::rename(entry.path(), targetPath);
				else
					fs::copy_file(entry.path(), targetPath, fs::copy_options::overwrite_existing);
			}
			catch (const std::exception& e)
			{
				std::lock_guard<std::mutex> lock(coutMutex);
				std::cerr << "Error processing " << entry.path() << ": " << e.what() << "\n";
			}
		};


		// --- Multi-threaded execution ---

		std::vector<std::thread> pool;
		std::atomic<size_t> index = 0;

		auto worker = [&]()
			{
				while (true)
				{
					size_t i = index++;
					if (i >= files.size()) break;
					process_file(files[i]);
				}
			};

		for (size_t i = 0; i < threads; ++i)
			pool.emplace_back(worker);

		for (auto& t : pool)
			t.join();

		std::cout << "File sorting completed successfully.\n";
	}
}