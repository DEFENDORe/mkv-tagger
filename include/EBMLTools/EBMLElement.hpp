#ifndef EBMLELEMENT_H
#define EBMLELEMENT_H 

#include <string>
#include <cstdint>
#include <stack>

#include "EBMLElementType.hpp"

namespace EBMLTools
{
	class EBMLReadElement;
	class EBMLElement
	{
		private:
			static const size_t KNOWN_ELEMENTS_LENGTH = 232;
			static const EBMLElement KNOWN_ELEMENTS[KNOWN_ELEMENTS_LENGTH];

		protected:
			union FloatUnion { uint8_t buf[4]; float number; };		// For converting binary to a float
			union DoubleUnion { uint8_t buf[8]; double number; };	// For converting binary to a double

			std::string name;
			ElementType type;
			uint64_t id;
			uint64_t parentId;
			bool manditory;
			bool multiple;

			EBMLElement();

		public:
			static const EBMLElement & Find(uint64_t id);
			static const EBMLElement & Find(std::string name);

			EBMLElement(std::string name, ElementType type, uint64_t id, uint64_t parentId, bool manditory, bool multiple);

			std::string GetElementName() const;
			ElementType GetElementType() const;
			uint64_t GetElementId() const;
			uint64_t GetElementParentId() const;
			size_t GetElementIdByteLength() const;
			size_t GetElementLevel() const;
			std::stack<EBMLElement> GetElementParentMap() const;
			bool isGlobalElement() const; // No defined parent (global element; can be found anywhere in EBML)
			bool isRootElement() const;   // Root element (has no parent)

			bool operator == (const EBMLElement &rhs) const;
			bool operator !=(const EBMLElement &rhs) const;
			bool operator ==(const EBMLReadElement &rhs) const;
			bool operator !=(const EBMLReadElement &rhs) const;
	};
}

#endif