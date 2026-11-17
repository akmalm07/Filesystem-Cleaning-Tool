#pragma once

#include <memory>
#include <expected>
#include "pritty_text.h"
#include "enums.h"


namespace clean
{
	namespace fs = std::filesystem;

	class Command
	{
	public:
		virtual ~Command() = default;
		virtual void execute(const std::vector<std::string_view>& options) = 0;
	};

	class HelpCommand : public Command
	{
		public:
			void execute(const std::vector<std::string_view>& options) override;
	};
	
	class VersionCommand : public Command
	{
		public:
			void execute(const std::vector<std::string_view>& options) override;
	};

	class ListenCommand : public Command
	{
	public:
		void execute(const std::vector<std::string_view>& options) override;
	};

	class ExecuteCommand : public Command
	{
	public:
		void execute(const std::vector<std::string_view>& options) override;
	};

	class SortCommand : public Command
	{
	public:
		void execute(const std::vector<std::string_view>& options) override;
	private:

		enum class SortOptions : uint32_t {
			Null = 0,
			ByTypePrimary = 1 << 0,
			ByTypeSecondary = 1 << 1,
			ByTypeTertiary = 1 << 2,

			ByDatePrimary = 1 << 3,
			ByDateSecondary = 1 << 4,
			ByDateTertiary = 1 << 5,

			BySizePrimary = 1 << 6,
			BySizeSecondary = 1 << 7,
			BySizeTertiary = 1 << 8,

			Move = 1 << 9,
			Overwrite = 1 << 10,
			Threads = 1 << 11,
			NoGit = 1 << 12,
		};

		static constexpr SortOptions ByType = SortOptions::ByTypePrimary | SortOptions::ByTypeSecondary | SortOptions::ByTypeTertiary;
		static constexpr SortOptions ByDate = SortOptions::ByDatePrimary | SortOptions::ByDateSecondary | SortOptions::ByDateTertiary;
		static constexpr SortOptions BySize = SortOptions::BySizePrimary | SortOptions::BySizeSecondary | SortOptions::BySizeTertiary;


		template<SortOptions T>
		constexpr SortOptions set_enum(std::string_view next = "", size_t& i = 0)
		{

			if (is_number(next))
			{
				i++;
			}

			if constexpr (contains_any(T, ByType))
			{
				if (next != "")
					{
					if (next == "1")
						return SortOptions::ByTypePrimary;
					else if (next == "2")
						return SortOptions::ByTypeSecondary;
					else if (next == "3")
						return SortOptions::ByTypeTertiary;
				}
				return SortOptions::ByTypePrimary;
			}
			else if constexpr (contains_any(T, ByDate))
			{
				if (next != "")
				{
					if (next == "1")
						return SortOptions::ByDatePrimary;
					else if (next == "2")
						return SortOptions::ByDateSecondary;
					else if (next == "3")
						return SortOptions::ByDateTertiary;
				}
				return SortOptions::ByDatePrimary;
			}
			else if constexpr (contains_any(T, BySize))
			{
				if (next != "")
				{
					if (next == "1")
						return SortOptions::BySizePrimary;
					else if (next == "2")
						return SortOptions::BySizeSecondary;
					else if (next == "3")
						return SortOptions::BySizeTertiary;
				}
				return SortOptions::BySizePrimary;
			}
			else
			{
				return T;
			}
		}

		void print_sort_help() const;

		void print_sort_example() const;

		void perform_sort_operation(const fs::path& sourceDir,
			const fs::path& destDir,
			SortOptions options,
			std::optional<uint8_t> threadCount);
	};
	

	fs::path normalize_path(const fs::path& path);

	std::optional<fs::path> validate_directory(fs::path& path, bool must_exist = true);

	std::optional<uint8_t> parse_thread_count(std::string_view str);

	bool try_initialize_git(const fs::path& destDir);

	std::unique_ptr<Command> create_command(const char* command);
	
	void print_help();


	// Legacy function, may be removed in future
	std::expected <fs::path, bool> build_path(const fs::path& path);


}
