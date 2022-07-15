#include <EBMLTools/EBMLReadElement.hpp>
#include <EBMLTools/EBMLReader.hpp>
#include <EBMLTools/EBMLWriteElement.hpp>
#include <CRC.hpp>
#include <swap_endian.hpp>

#include <iomanip>
#include <sstream>

namespace EBMLTools
{
	EBMLReadElement::EBMLReadElement(EBMLReader * reader, const EBMLElement & base, uint64_t dataSize, uint8_t dataSizeByteLength, size_t position)
	{
		*this = base;
		this->reader = reader;
		this->dataSize = dataSize;
		this->dataSizeByteLength = dataSizeByteLength;
		this->position = position;

		if(!isRootElement() && !isGlobalElement())
		{
			bool found = false;
			for (auto &masterPair : reader->parentStructure)
			{
				if (masterPair.first > position)
					break;
				if (masterPair.first + masterPair.second.first < position)
					continue;
				if (masterPair.second.second != parentId)
					continue;
				found = true;
				parentPosition = masterPair.first;
			}
			if (!found)
				throw std::logic_error("EBMLReadElement::EBMLReadElement(): cannot find parent in EBMLReader->parentStructure.");
		}
		if (type == Master)
			reader->parentStructure[position] = std::make_pair(GetElementByteLength(), id);
		if (reader->integrityCheck && type == Master && dataSize > 0) {
			EBMLReadElement firstChild = FirstChild();
			if (firstChild.GetElementName() == "CRC-32") {
				if (firstChild.GetUintData() != CalculateCRC32())
					throw std::runtime_error("Data Integrety Compromised: The computed CRC32 value does not equal the CRC32 found.");
			}
		}
	}
	
	EBMLReadElement::EBMLReadElement(const EBMLElement &e)
	{
		this->id = e.GetElementId();
		this->name = e.GetElementName();
		this->type = e.GetElementType();
		this->parentId = e.GetElementParentId();
	}

	bool EBMLReadElement::operator ==(const EBMLReadElement &rhs) const { return position == rhs.position; }
	bool EBMLReadElement::operator !=(const EBMLReadElement &rhs) const { return !(*this == rhs); };
	bool EBMLReadElement::operator ==(const EBMLWriteElement &rhs) const { return id == rhs.GetElementId(); }
	bool EBMLReadElement::operator !=(const EBMLWriteElement &rhs) const { return !(*this == rhs); };

	std::ostream& operator<<(std::ostream& os, const EBMLReadElement& element)  
	{
		return os << element.ToString();
	}

	std::ostream& operator<<(std::ostream& os, const std::vector<EBMLReadElement>& elements)
	{
		for (auto ele : elements)
			os << ele;
		return os;
	}

	std::string EBMLReadElement::ToString(bool showChildren) const
	{
		std::stringstream str;
		str << std::setw(6) << std::left << std::string(GetElementLevel(), '+')
		   << std::setw(21) << std::left << name 
		   << " | Pos: " << std::setw(12) << position
		   << " | ID: 0x" << std::setw(8) << std::uppercase << std::hex << id
		   << " (" << std::dec << int(GetElementIdByteLength()) << ") | Size: " << std::setw(12) << dataSize
		   << " (" << int(dataSizeByteLength) << ") | Data: ";
		switch (type)
		{
			case UTF8:
				str << "(utf8) - " << GetStringData();
				break;
			case String:
				str << "(string) - " << GetStringData();
				break;
			case Uint:
				str << "(uint) - " << GetUintData();
				break;
			case Int:
				str << "(int) - " << (int) GetUintData();
				break;
			case Float:
				str << "(float) - " << GetFloatData();
				break;
			case Date:
			{
				std::string timeString = asctime(GetDateData());
				timeString[timeString.size() - 1] = 0;
				str << "(date) - " << timeString;
				break;
			}
			case Master:
				str << "(master)";
				break;
			case Binary:
				str << "(binary)";
				if (name == "CRC-32")
					str << " - " << std::dec << GetUintData();
				break;
			default:
				str << "(unknown)";				
				break;
		}
		str << std::endl;
		if (showChildren && type == Master)
		{
			auto children = Children();
			for (auto &child : children)
				str << child.ToString(showChildren);
		}
		return str.str();
	}
	
	size_t EBMLReadElement::GetElementPosition() const { return position; }
	size_t EBMLReadElement::GetElementDataSize() const { return dataSize; }
	size_t EBMLReadElement::GetElementDataSizeByteLength() const { return dataSizeByteLength; }
	size_t EBMLReadElement::GetElementByteLength() const { return GetElementIdByteLength() + dataSizeByteLength + GetElementDataSize(); }

	std::vector<EBMLReadElement> EBMLReadElement::Children() const { return Children(*this); }

	std::vector<EBMLReadElement> EBMLReadElement::Children(const EBMLElement & filter) const
	{
		if (type != Master)
			throw std::runtime_error("EBMLReadElement::Children failed because it is not of Type Master.. Element: " + name);
		std::vector<EBMLReadElement> children;
		size_t cachedPosition = reader->GetReadPosition();
		reader->SetReadPosition(position + GetElementIdByteLength() + dataSizeByteLength);
		while (reader->GetReadPosition() < position + GetElementByteLength())
		{
			EBMLReadElement element = reader->ReadElement();
			if (filter == element || filter == *this)
				children.push_back(element);
		}
			
		reader->SetReadPosition(cachedPosition);
		return children;
	}

	EBMLReadElement EBMLReadElement::FirstChild() const
	{
		if (type != Master)
			throw std::runtime_error("EBMLReadElement::Children failed because it is not of Type Master");
		size_t cachedPosition = reader->GetReadPosition();
		reader->SetReadPosition(position + GetElementIdByteLength() + dataSizeByteLength);
		EBMLReadElement result = reader->ReadElement();
		reader->SetReadPosition(cachedPosition);
		return result;
	}

	EBMLReadElement EBMLReadElement::Parent() const
	{
		if (isRootElement())
			throw std::runtime_error("EBMLReadElement::Parent(). element is root element, no parents..");
		return reader->GetElement(parentPosition);
	}

	uint32_t EBMLReadElement::CalculateCRC32() const
	{
		if (type != Master)
			throw std::logic_error("EBMLReadElement::CalculateCRC32(), cannot calculate the crc32 of a non master element.");
		if (!(dataSize > 0))
			throw std::logic_error("EBMLReadElement::CalculateCRC32(), cannot calculate the crc32 of an element with no children");
		
		size_t cachedPosition = reader->GetReadPosition();
		
		EBMLReadElement firstChild = FirstChild();
		size_t startPosition = position + GetElementIdByteLength() + GetElementDataSizeByteLength();
		size_t byteCount = GetElementDataSize();
		if (firstChild.GetElementName() == "CRC-32") {
			startPosition += firstChild.GetElementByteLength();
			byteCount -= firstChild.GetElementByteLength();
		}
		
		reader->SetReadPosition(startPosition);
		uint8_t * bytes = new uint8_t[byteCount];
		reader->fileStream.read((char *) bytes, byteCount);
		uint32_t crc = CRC::Calculate(bytes, byteCount, CRC::CRC_32());
		crc = swap_endian<uint32_t>(crc);
		delete [] bytes;
		reader->SetReadPosition(cachedPosition);
		return crc;
	}

	uint8_t * EBMLReadElement::GetData() const
	{
		if (type == Master)
			throw std::runtime_error("EBMLReadElement::GetData(). element is of type Master, data is EBML..");
		size_t cachedposition = reader->GetReadPosition();
		uint8_t * bytes = new uint8_t[dataSize];
		reader->SetReadPosition(position + GetElementIdByteLength() + dataSizeByteLength);
		reader->fileStream.read((char *) bytes, dataSize);
		reader->SetReadPosition(cachedposition);
		return bytes;
	}
	
	std::string EBMLReadElement::GetStringData() const
	{
		if (type != String && type != UTF8)
			throw std::runtime_error("EBMLReadElement::GetStringData(). element is not of type String or UTF8");
		uint8_t * bytes = GetData();
		size_t nullIndex = 0;
		while (nullIndex < dataSize)
			if (bytes[nullIndex++] == '\0')
				break;
		std::string value = std::string((char *)bytes, nullIndex);
		delete [] bytes;
		return value;
	}

	uint64_t EBMLReadElement::GetUintData() const
	{
		if (type != Uint && type != Int && type != Date)
			throw std::runtime_error("EBMLReadElement::GetUintData(). element is not of type Uint.");
		uint64_t value = 0;
		uint8_t * bytes = GetData();
		for (size_t i = 0; i < dataSize; i++)
			value = (value << 8) | (uint64_t) bytes[i];
		delete [] bytes;
		return value;
	}

	double EBMLReadElement::GetFloatData() const
	{
		if (type != Float)
			throw std::runtime_error("EBMLReadElement::GetFloatData(). element is not of type Float.");
		double value = 0;
		uint8_t * bytes = GetData();
		if (dataSize == 4) 
		{
			FloatUnion _float;
			for (size_t i = 0; i < dataSize; i++)
				_float.buf[i] = bytes[dataSize - 1 - i];
			value = _float.number;
		}
		else if (dataSize == 8)
		{
			DoubleUnion _double;
			for (size_t i = 0; i < dataSize; i++)
				_double.buf[i] = bytes[dataSize - 1 - i];
			value = _double.number;
		} 
		else
		{
			throw std::runtime_error("EBMLReadElement::GetFloatData(). Not implemented");
		}
		delete [] bytes;
		return value;
	}

	tm * EBMLReadElement::GetDateData() const
	{
		time_t epocheTime = GetUintData() / 1000000000 + 978307200;
		tm *gmtm = std::gmtime(&epocheTime);
		return  gmtm;
	}
}