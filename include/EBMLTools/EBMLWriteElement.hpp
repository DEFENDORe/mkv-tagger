#ifndef EBMLWRITEELEMENT_H
#define EBMLWRITEELEMENT_H

#include "EBMLReadElement.hpp"

#include <ostream>
#include <iostream>
#include <memory>

namespace EBMLTools
{
	class EBMLWriteElement : public EBMLElementTemplate
	{
		private:
			friend class EBMLParser;
			static uint8_t DetermineByteLengthOfValue(uint64_t size);
			uint8_t * data = { NULL };
			mutable std::vector<std::unique_ptr<EBMLWriteElement>> children;
			EBMLWriteElement * parent = NULL;
			
		public:
			EBMLWriteElement(EBMLReadElement &rhs);
			EBMLWriteElement(const EBMLElement & ele);
			EBMLWriteElement(const EBMLWriteElement & wele);
			EBMLWriteElement& operator= (const EBMLWriteElement& wele);
			~EBMLWriteElement();

			bool operator ==(const EBMLReadElement &rhs) const;
			bool operator !=(const EBMLReadElement &rhs) const;

			std::vector<std::unique_ptr<EBMLWriteElement>> & Children() const;
			std::vector<EBMLWriteElement *> Children(const EBMLElement & query) const;

			friend std::ostream& operator<<(std::ostream& os, const EBMLWriteElement& element);
			std::string ToString(bool showChildren = false) const;
			uint8_t * ToBytes() const;

			uint32_t CalculateCRC32() const;

			uint8_t * GetData() const;
			std::string GetStringData() const;
			uint64_t GetUintData() const;
			double GetFloatData() const;
			tm * GetDateData() const;

			void SetData(std::vector<uint8_t> & data);
			void SetStringData(std::string value);
			void SetUintData(uint64_t value);
			void SetFloatData(double value);
			void SetDateData(tm * value);

			void Validate();
	};
}

#endif