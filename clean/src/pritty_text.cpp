#include "headers.h"
#include "pritty_text.h"

namespace clean
{
	std::string red(std::string_view str)
	{
		return std::format("\033[31m{}\033[0m", str);
	}

	std::string bold(std::string_view str)
	{
		return std::format("\033[1m{}\033[0m", str);
	}

	std::string underline(std::string_view str)
	{
		return std::format("\033[4m{}\033[0m", str);
	}

	std::string loading_spinner(int frame)
	{
		const char* spinner_frames[] = { "|", "/", "-", "\\" };
		return std::format("\r{}", spinner_frames[frame % 4]);
	}

	std::string loading_bar(int width, float progress)
	{
		int pos = static_cast<int>(width * progress);

		std::string barFilled = std::string(pos, '#');
		std::string barEmpty = std::string(width - pos, '-');

		// Color codes
		const char* green = "\033[32m";
		const char* gray = "\033[90m";
		const char* reset = "\033[0m";

		// Build bar with colors
		std::string bar = "\r[";
		bar += green + barFilled + reset;
		bar += gray + barEmpty + reset;
		bar += "] ";
		bar += std::format("{:6.2f}%", progress * 100.0f);

		return bar;
	}

	void print_error(std::string_view msg)
	{
		std::cout << "\033[31mError:\033[0m " << msg << std::endl;
	}

	std::string green(std::string_view str)
	{
		return std::format("\033[32m{}\033[0m", str);
	}

	std::string yellow(std::string_view str)
	{
		return std::format("\033[33m{}\033[0m", str);
	}

	std::string blue(std::string_view str)
	{
		return std::format("\033[34m{}\033[0m", str);
	}

	
}