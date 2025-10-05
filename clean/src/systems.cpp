#include "headers.h"
#include "systems.h"

namespace sys
{
	void sleep_for_milliseconds(unsigned int seconds)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(seconds));
	}

	bool git_exists()
	{
#if defined(_WIN32) || defined(_WIN64)
		int result = (int)std::system("where git >nul 2>nul");
#else
		int result = (int)std::system("which git >/dev/null 2>&1");
#endif
		return result == 0;
	}

	bool user_said_yes_repeat()
	{
		std::string answer;
		while (true)
		{
			std::cout << "Enter Y or N: ";
			std::getline(std::cin, answer);
			if (answer == "Y" || answer == "y")
				return true;
			else if (answer == "N" || answer == "n")
				return false;
			else
				std::cout << "Invalid input. Please enter Y or N." << std::endl;
		}
	}
}

