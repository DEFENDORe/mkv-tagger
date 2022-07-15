#ifndef EBMLPARSER_H
#define EBMLPARSER_H

#include "EBMLReader.hpp"
#include "EBMLWriteElement.hpp"

namespace EBMLTools
{
	class EBMLParser : public EBMLReader
	{
		private:
			friend class EBMLWriteElement;
			const uint8_t ZEROBYTE = 0x00;
			static EBMLWriteElement CreateVoid(uint64_t totalSize);
			static uint8_t * CreateBlock(const uint64_t &value, const size_t &byteLength, bool encode = false);

			void SetWritePosition(size_t position);
			size_t GetWritePosition();
			void RawWrite(const EBMLWriteElement & wele);
			EBMLWriteElement CreateSeekHead();
			void MergeConsecutiveVoidElements();
			void UpdateSeekHead();
			void AppendElement(EBMLWriteElement & wele);
			void OverwriteElement(EBMLReadElement & ele, EBMLWriteElement & wele);
		public:
			EBMLParser();
			EBMLParser(std::string file, bool dataIntegrityCheck = false);
			void OpenFile(std::string file, bool dataIntegrityCheck = false);
			void UpdateElement(EBMLReadElement & ele, EBMLWriteElement & wele);
			void AddElement(EBMLWriteElement & wele);
	};
}

#endif