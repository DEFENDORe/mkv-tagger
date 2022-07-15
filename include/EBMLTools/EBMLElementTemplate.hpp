#ifndef EBMLELEMENTEMPLATE_H
#define EBMLELEMENTEMPLATE_H

#include "EBMLElement.hpp"

namespace EBMLTools
{
	class EBMLElementTemplate : public EBMLElement
	{
		protected:
			uint64_t dataSize = 0;
			uint8_t dataSizeByteLength = 0;
		public:
			uint64_t GetElementByteLength() const { return (uint64_t) GetElementIdByteLength() + (uint64_t) dataSizeByteLength + dataSize; }
			uint64_t GetElementDataSize() const { return dataSize; }
			uint8_t GetElementDataSizeByteLength() const { return dataSizeByteLength; }

			bool operator ==(const EBMLElement &rhs) const { return id == rhs.GetElementId(); };
			bool operator !=(const EBMLElement &rhs) const { return !(*this == rhs); };

			virtual std::string ToString(bool showChildren = false) const = 0;

			virtual uint32_t CalculateCRC32() const = 0;

			virtual uint8_t * GetData() const = 0;
			virtual std::string GetStringData() const = 0;
			virtual uint64_t GetUintData() const = 0;
			virtual double GetFloatData() const = 0;
			virtual tm * GetDateData() const = 0;
	};
}

#endif