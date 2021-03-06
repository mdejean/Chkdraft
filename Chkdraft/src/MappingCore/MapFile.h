#ifndef MAPFILE_H
#define MAPFILE_H
#include "Scenario.h"
#include <cstdio>
#include <time.h>

#define MapFilePtr std::shared_ptr<MapFile>

class MapFile : public Scenario // MapFile is a scenario file + the stuff here
{
	public:
		MapFile();

		bool LoadFile(const char* &path); // If you're not providing a path, pass in nullptr
		bool LoadFile();

		bool SaveFile(bool SaveAs);

		bool GetPath();
		bool SetPath(const char* newPath);
		const char* FilePath();
		void SetSaveType(u8 newSaveType);

		std::string GetFileName();
		std::string GetFilePath();

	protected:
		bool OpenFile();

	private:
		char filePath[FILENAME_MAX];
		u8 SaveType;
};

#endif