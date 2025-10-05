#pragma once

#include <string>
#include <format>

namespace clean
{
	constexpr std::string_view delline = "\33[2K"; 
	constexpr std::string_view move_up = "\033[A";
	constexpr std::string_view version = "1.0.0";


	std::string red(std::string_view str);

	std::string green(std::string_view str);

	std::string yellow(std::string_view str);

	std::string blue(std::string_view str);

	std::string bold(std::string_view str);

	std::string underline(std::string_view str);

	std::string loading_spinner(int frame);

	std::string loading_bar(int width, float progress);

	void print_error(std::string_view msg);


}
