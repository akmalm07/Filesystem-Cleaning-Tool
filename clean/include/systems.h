#pragma once

#define _GIT_EXISTS

namespace sys
{
#if defined(_WIN32) || defined(_WIN64)
	constexpr const char* clear = "cls";
	constexpr const char* mkdir = "mkdir";
	constexpr const char* rmdir = "rmdir /s /q";
	constexpr const char* rmfile = "del /f /q";
	constexpr const char* copyfile = "copy /y";
	constexpr const char* movefile = "move /y";


	constexpr const char* cd = "cd";

#elif defined(__linux__) || defined(__APPLE__)
	constexpr const char* clear = "clear";
	constexpr const char* mkdir = "mkdir -p";
	constexpr const char* rmdir = "rm -rf";
	constexpr const char* rmfile = "rm -f";
	constexpr const char* copyfile = "cp -f";
	constexpr const char* movefile = "mv -f";

#else
	constexpr const char* clear = "clear";
	constexpr const char* mkdir = "mkdir -p";
#endif

#ifdef _GIT_EXISTS
	constexpr const char* gitstatus = "git status";
	constexpr const char* gitadd = "git add .";
	constexpr const char* gitcommit = "git commit -m";
	constexpr const char* gitinit = "git init";
	constexpr const char* gitcheck = "git --version";

#else
	constexpr const char* gitstatus = "echo 'Error, GIT is not installed! This tool cannot funciton without git!'";
	constexpr const char* gitadd = "echo 'Error, GIT is not installed! This tool cannot funciton without git!'";
	constexpr const char* gitcommit = "echo 'Error, GIT is not installed! This tool cannot funciton without git!' && ";
#endif


	bool user_said_yes_repeat();

	void sleep_for_milliseconds(unsigned int seconds);

	bool git_exists();
}