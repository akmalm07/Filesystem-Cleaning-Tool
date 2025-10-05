#include "headers.h"
#include "file_metadata.h"
#include "commands.h"
#include "pritty_text.h"

int main(int argc, char* argv[])
{

	if (argc < 2)
	{
		clean::print_help();
		return 0;
	}

	/*
	int frame = 0;
	clean::loading_bar(30, 0.0f);
	while (frame <= 100)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
		std::cout << clean::loading_bar(30, frame / 100.0f) << std::flush;
		frame++;
	}
	std::cout.flush();
	std::cout << std::endl << "Finished!" << std::endl;
	*/
	std::vector<std::string_view> options(argv + 2, argv + argc);

	std::unique_ptr<clean::Command> cmd = clean::create_command(argv[1]);
	if (cmd)
	{
		cmd->execute(options);
	}
	else
	{
		std::cout << clean::red("Error: Unknown command '") << argv[1] << clean::red("'") << std::endl;
		clean::print_help();
		return 1;
	}


	//std::cout << "Argc: " << argc << std::endl;

	//for (int i = 0; i < argc; ++i)
	//{
	//	std::cout << "Argv[" << i << "]: " << argv[i] << std::endl;
	//}

	//clean::FileMetaData meta = clean::get_file_metadata(argv[1]);

	//clean::print_data(meta);
	

}