#pragma once

#include <string>

namespace clean
{
	struct FileMetaData 
	{
		std::string filePath;
		std::string fileName;
		std::string fileExtension;
		std::string creationTime;
		std::string lastAccessTime;
		std::string lastWriteTime;
		uintmax_t fileSize;

		FileMetaData(const std::string& path,
			const std::string& name,
			const std::string& extension,
			uintmax_t size,
			const std::string& cTime,
			const std::string& aTime,
			const std::string& wTime)
			: filePath(path), fileName(name), fileExtension(extension),
			fileSize(size), creationTime(cTime), lastAccessTime(aTime), lastWriteTime(wTime) {
		}
	};

	FileMetaData get_file_metadata(const std::string& filePath);

	void print_data(const FileMetaData& meta);


}