#ifndef EBMLREADER_H
#define EBMLREADER_H 

#include <map>
#include <fstream>
#include <memory>
#include "EBMLReadElement.hpp"


namespace EBMLTools
{	
	class EBMLReader 
	{
		private:
			friend class EBMLReadElement;
			enum class ReadMode { Normal, Descende }; // Normal read mode does not descende into child elements.. it will skip to next element on the same level.

			static uint8_t ParseBlockLength(uint8_t value);
			static uint8_t ParseBlockData(uint8_t byte, uint8_t blockLength);
		protected:
			std::string fileName = "";
			size_t fileSize = 0;
			std::unique_ptr<EBMLReadElement> firstSeekHead = NULL;
			std::map<size_t, uint64_t> seekHead; // position, id
			uint8_t maxIdLength = 4;	// Default as per EBML spec (The max EBML ID byte length to read)
			uint8_t maxSizeLength = 4;	// Default per EBML spec (The max EBML Size byte length to read) **obviously 64bit files (>4GB) will set this to 8
			std::map<size_t, std::pair<size_t, uint64_t>> parentStructure; // position, size, id
			bool integrityCheck = false;
			
			mutable std::fstream fileStream;

			bool EndOfRead();
			void SetReadPosition(size_t position);
			size_t GetReadPosition();
			uint64_t ReadNextBlock(uint8_t &length, bool isSize = false);
			uint8_t GetNextByte();

			EBMLReadElement ReadElement(ReadMode mode = ReadMode::Normal);
			EBMLReadElement GetElement(size_t fileposition);
			EBMLReadElement operator [] (size_t fileposition);
		public:
			EBMLReader();
			EBMLReader(std::string file, bool dataIntegrityCheck = false);
			~EBMLReader();

			void OpenFile(std::string file, bool dataIntegrityCheck = false);
			void CloseFile();

			const std::string GetFilename() const;

			void DisableDataIntegrityCheck();
			void EnableDataIntegrityCheck();

			std::vector<EBMLReadElement> GetRootElements();
			std::vector<EBMLReadElement> GetRootElements(const EBMLElement & filter);
			std::vector<EBMLReadElement> Search(const EBMLElement & query);
			std::vector<EBMLReadElement> FastSearch(const EBMLElement & query);
	}; 
}

#endif