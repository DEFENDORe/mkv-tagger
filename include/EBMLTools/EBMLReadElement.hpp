#ifndef EBMLREADELEMENT_H
#define EBMLREADELEMENT_H 

#include <vector>
#include <ostream>
#include <ctime>

#include "EBMLElementTemplate.hpp"

namespace EBMLTools
{
	class EBMLReader;
	class EBMLWriteElement;

	class EBMLReadElement : public EBMLElementTemplate
	{
		private:
			EBMLReader * reader = 0;
			EBMLReadElement(const EBMLElement &e);
			size_t position;
			size_t parentPosition;
		public:
			EBMLReadElement(EBMLReader * reader, const EBMLElement & base, uint64_t dataSize, uint8_t dataSizeByteLength, size_t position);

			bool operator ==(const EBMLWriteElement &rhs) const;
			bool operator !=(const EBMLWriteElement &rhs) const;
			bool operator ==(const EBMLReadElement &rhs) const;
			bool operator !=(const EBMLReadElement &rhs) const;
			

			friend std::ostream& operator<<(std::ostream& os, const EBMLReadElement& element);
			friend std::ostream& operator<<(std::ostream& os, const std::vector<EBMLReadElement>& elements);
			std::string ToString(bool showChildren = false) const;

			size_t GetElementPosition() const;
			size_t GetElementByteLength() const;
			size_t GetElementDataSize() const;
			size_t GetElementDataSizeByteLength() const;

			std::vector<EBMLReadElement> Children() const;
			std::vector<EBMLReadElement> Children(const EBMLElement & filter) const;
			EBMLReadElement FirstChild() const;
			EBMLReadElement Parent() const;

			uint32_t CalculateCRC32() const;
            
			uint8_t * GetData() const;
			std::string GetStringData() const;
			uint64_t GetUintData() const;
			double GetFloatData() const;
			tm * GetDateData() const;
	};
}

#endif