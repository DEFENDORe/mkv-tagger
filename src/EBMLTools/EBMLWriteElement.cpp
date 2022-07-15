#include <EBMLTools/EBMLWriteElement.hpp>
#include <EBMLTools/EBMLParser.hpp>
#include <CRC.hpp>
#include <swap_endian.hpp>
#include <timegm.hpp>

#include <iomanip>
#include <sstream>
#include <cmath>

namespace EBMLTools
{
	uint8_t EBMLWriteElement::DetermineByteLengthOfValue(uint64_t size)
	{
		uint8_t sizeLength;

		if (size < std::pow(2,7) - 2)
			sizeLength = 1;
		else if (size < std::pow(2,14) - 2)
			sizeLength = 2;
		else if (size < std::pow(2,21) - 2)
			sizeLength = 3;
		else if (size < std::pow(2,28) - 2)
			sizeLength = 4;
		else if (size < std::pow(2,35) - 2)
			sizeLength = 5;
		else if (size < std::pow(2,42) - 2)
			sizeLength = 6;
		else if (size < std::pow(2,49) - 2)
			sizeLength = 7;
		else
			sizeLength = 8;
		return sizeLength;
	}

	EBMLWriteElement::EBMLWriteElement(EBMLReadElement &ele)
	{
		name = ele.GetElementName();
		type = ele.GetElementType();
		parentId = ele.GetElementParentId();
		id = ele.GetElementId();
		dataSize = ele.GetElementDataSize();
		dataSizeByteLength = ele.GetElementDataSizeByteLength();

		if (type == Master)
		{
			for (auto &child : ele.Children())
			{
				std::unique_ptr<EBMLWriteElement> wchild = std::make_unique<EBMLWriteElement>(child);
				wchild->parent = this;
				children.push_back(std::move(wchild));   
			}
		}
		else
			data = ele.GetData();
	}

	EBMLWriteElement::EBMLWriteElement(const EBMLWriteElement & ele)
	{
		if (type == Master)
			children.clear();
		else
			delete [] data;	
		name = ele.name;
		type = ele.type;
		parentId = ele.parentId;
		id = ele.id;
		dataSize = ele.dataSize;
		dataSizeByteLength = ele.dataSizeByteLength;
		data = ele.data;
		parent = ele.parent;
		children = std::move(ele.children);
	}

	EBMLWriteElement& EBMLWriteElement::operator= (const EBMLWriteElement& ele)
	{
		if (type == Master)
			children.clear();
		else
			delete [] data;	
		name = ele.name;
		type = ele.type;
		parentId = ele.parentId;
		id = ele.id;
		dataSize = ele.dataSize;
		dataSizeByteLength = ele.dataSizeByteLength;
		data = ele.data;
		parent = ele.parent;
		children = std::move(ele.children);
		return *this;
	}

	EBMLWriteElement::EBMLWriteElement(const EBMLElement & ele)
	{
		name = ele.GetElementName();
		type = ele.GetElementType();
		parentId = ele.GetElementParentId();
		id = ele.GetElementId();
	}
	
	EBMLWriteElement::~EBMLWriteElement()
	{		
		if (type == Master)
			children.clear();
		else
			delete [] data;	
	}

	void EBMLWriteElement::Validate()
	{
		if (type == Master)
		{
			size_t newDataSize = 0;
			for (auto &child : children) {
				child->parent = this;
				child->Validate();
				newDataSize += child->GetElementByteLength();
			}
			dataSize = newDataSize;
			dataSizeByteLength = DetermineByteLengthOfValue(newDataSize);
			for (auto &child : children) {
				if (child->name != "CRC-32")
					break;
				child->SetUintData(CalculateCRC32());
			}
		}
	}

	std::vector<EBMLWriteElement *> EBMLWriteElement::Children(const EBMLElement & query) const
	{
		if (type != Master)
			throw std::logic_error("EBMLWriteElement::Children(). Element is not of type Master, no children");
		std::vector<EBMLWriteElement *> filtered;
		for (auto & child : children)
			if (child->GetElementId() == query.GetElementId())
				filtered.push_back(child.get());
		return filtered;
	}

	std::vector<std::unique_ptr<EBMLWriteElement>> & EBMLWriteElement::Children() const 
	{
		if (type != Master)
			throw std::logic_error("EBMLWriteElement::Children(). Element is not of type Master, no children");
		return children;
	}

	bool EBMLWriteElement::operator ==(const EBMLReadElement &rhs) const { return id == rhs.GetElementId(); }
	bool EBMLWriteElement::operator !=(const EBMLReadElement &rhs) const { return !(*this == rhs); };

	std::ostream& operator<<(std::ostream& os, const EBMLWriteElement& element)  
	{
		os << element.ToString();
		return os;
	}

	std::string EBMLWriteElement::ToString(bool showChildren) const
	{
		std::stringstream str;
		str << std::setw(6) << std::left << std::string(GetElementLevel(), '+')
		   << std::setw(21) << std::left << name 
		   << " | ID: 0x" << std::setw(8) << std::uppercase << std::hex << id
		   << " (" << std::dec << int(GetElementIdByteLength()) << ") | Size: " << std::setw(12) << dataSize
		   << " (" << int(dataSizeByteLength) << ") | Data: ";
		switch (type) 
		{
			case String:
				str << "(string) - " << GetStringData();
				break;
			case UTF8:
				str << "(utf8) - " << GetStringData();
				break;
			case Uint:
				str << "(uint) - " << GetUintData();
				break;
			case Float:
				str << "(float) - " << GetFloatData();
				break;
			case Binary:
				str << "(binary)";
				if (name == "CRC-32")
					str << " - " << std::dec << GetUintData();
				break;
			case Date:
				str << "(date) - " << asctime(GetDateData());
				break;
			case Master:
				str << "(master)";
				break;
			default:
				str << "(unknown)";
				break;
		}
		str << std::endl;
		if (showChildren && type == Master)
		{
			for (auto &child : children)
				str << child->ToString(showChildren);
		}
		return str.str();
	}

	uint8_t * EBMLWriteElement::ToBytes() const
	{
		uint8_t * bytes = new uint8_t[GetElementByteLength()];
		uint8_t * idBlock = EBMLParser::CreateBlock(id, GetElementIdByteLength());
		uint8_t * sizeBlock = EBMLParser::CreateBlock(dataSize, dataSizeByteLength, true);
		size_t bytesIndex = 0;
		for (size_t i = 0; i < GetElementIdByteLength(); i++, bytesIndex++)
			bytes[bytesIndex] = idBlock[i];
		for (size_t i = 0; i < GetElementDataSizeByteLength(); i++, bytesIndex++)
			bytes[bytesIndex] = sizeBlock[i];
		delete [] idBlock;
		delete [] sizeBlock;
		if (type == Master)
		{
			uint8_t * dataBlock = new uint8_t[dataSize];
			size_t dataBlockIndex = 0;
			for (auto & child : Children())
			{
				uint8_t * tmp = child->ToBytes();
				for (size_t i = 0; i < child->GetElementByteLength(); i++, dataBlockIndex++)
					dataBlock[dataBlockIndex] = tmp[i];
				delete [] tmp;
			}
			for (size_t i = 0; i < dataSize; i++, bytesIndex++)
				bytes[bytesIndex] = dataBlock[i];
			delete [] dataBlock;
		} else {
			uint8_t * dataBlock = GetData();
			for (size_t i = 0; i < dataSize; i++, bytesIndex++)
				bytes[bytesIndex] = dataBlock[i];
		}
		return bytes;
	}

	uint32_t EBMLWriteElement::CalculateCRC32() const
	{
		if (type != Master)
			throw std::logic_error("EBMLWriteElement::CalculateCRC32(), cannot calculate crc32 from a non master element.");
		if (!(dataSize > 0))
			throw std::logic_error("EBMLWriteElement::CalculateCRC32(), cannot calculate crc32 from a master element with no children.");
		size_t byteCount = dataSize;
		if (Children().at(0)->GetElementName() == "CRC-32")
			byteCount -= Children().at(0)->GetElementByteLength();

		uint8_t * bytes = new uint8_t [byteCount];
		size_t bytesIndex = 0;
		for (auto & child : Children())
		{
			if (child->GetElementName() == "CRC-32")
				continue;
			uint8_t * element = child->ToBytes();
			for (size_t i = 0; i < child->GetElementByteLength(); i++, bytesIndex++)
				bytes[bytesIndex] = element[i];
			delete [] element;
		}
		uint32_t crc = CRC::Calculate(bytes, byteCount, CRC::CRC_32());
		delete [] bytes;
		crc = swap_endian<uint32_t>(crc);
		return crc;
	}

	uint8_t * EBMLWriteElement::GetData() const
	{
		if (type == Master)
			throw std::logic_error("EBMLWriteElement::GetData(). Element is of type Master, data is children..");
		return data;
	}

	std::string EBMLWriteElement::GetStringData() const
	{
		if (type != String && type != UTF8)
			throw std::logic_error("EBMLWriteElement::GetStringData(). Element is not of type UTF or String.");
		std::string result;
		for (size_t i = 0; i < dataSize; i++)
			result.push_back(data[i]);
		return result;
	}

	uint64_t EBMLWriteElement::GetUintData() const
	{
		if (type != Uint && type != Int && type != Date && type != Binary)
			throw std::logic_error("EBMLWriteElement::GetUintData(). Element is not of type Uint or Int or Date");
		uint64_t result = 0;
		for (size_t i = 0; i < dataSize; i++)
			result = (result << 8) | (uint64_t) data[i];
		return result;
	}

	double EBMLWriteElement::GetFloatData() const
	{
		if (type != Float)
			throw std::logic_error("EBMLWriteElement::GetFloatData(). Element is not of type Float");
		double result;
		if (dataSize == 4) 
		{
			FloatUnion _float;
			for (size_t i = 0; i < dataSize; i++)
				_float.buf[i] = data[dataSize - 1 - i];
			result = _float.number;
		} else {
			DoubleUnion _double;
			for (size_t i = 0; i < dataSize; i++)
				_double.buf[i] = data[dataSize - 1 - i];
			result = _double.number;
		} 
		return result;
	}

	tm * EBMLWriteElement::GetDateData() const
	{
		time_t epocheTime = GetUintData() / 1000000000 + 978307200;
		tm *gmtm = std::gmtime(&epocheTime);
		return gmtm;
	}

	void EBMLWriteElement::SetData(std::vector<uint8_t> & data)
	{
		if (type != Binary)
			throw std::logic_error("EBMLWriteElement::SetData(). Element is not of type Binary..");
		delete [] this->data;
		this->data = new uint8_t [data.size()];
		this->dataSize = data.size();
		this->dataSizeByteLength = DetermineByteLengthOfValue(this->dataSize);
		for (size_t i = 0; i < this->dataSize; i++)
			this->data[i] = data[i];
	}

	void EBMLWriteElement::SetStringData(std::string value)
	{
		if (type != String && type != UTF8)
			throw std::logic_error("EBMLWriteElement::SetStringData(). Element is not of type UTF or String.");
		delete [] data;
		dataSize = value.length();
		dataSizeByteLength = DetermineByteLengthOfValue(dataSize);
		data = new uint8_t [dataSize];
		for (size_t i = 0; i < dataSize; i++)
			data[i] = value[i];
	}

	void EBMLWriteElement::SetUintData(uint64_t value)
	{
		if (type != Uint
			&& type != Int 
			&& type != Date)
			throw std::logic_error("EBMLWriteElement::SetUintData(). Element is not of type Uint or Int or Date");
		
		delete [] data;
		if (value <= std::numeric_limits<uint8_t>::max())
			dataSize = 1;	
		else if (value <= std::numeric_limits<uint16_t>::max())
			dataSize = 2;
		else if (value <= std::numeric_limits<uint32_t>::max())
			dataSize = 4;
		else
			dataSize = 8;
		//dataSize = DetermineByteLengthOfValue(value);
		dataSizeByteLength = DetermineByteLengthOfValue(dataSize);
		if (type == Date)
		{
			dataSize = 8;
			dataSizeByteLength = 1;
		} else if (name == "CRC-32") {
			dataSize = 4;
			dataSizeByteLength = 1;
		}
		data = new uint8_t[dataSize];
		for (size_t i = 0; i < dataSize; i++)
			data[dataSize - 1 - i] = (value >> (i * 8));
	}

	void EBMLWriteElement::SetFloatData(double value)
	{
		if (type != Float)
			throw std::logic_error("EBMLWriteElement::SetFloatData(). Element is not of type Float");
		delete [] data;
		if (value == float(value))
		{
			dataSize = 4;
			data = new uint8_t [dataSize];
			FloatUnion _float;
			_float.number = dataSize;
			for (size_t i = 0; i < dataSize; i++)
				data[i] = _float.buf[i];
		} else {
			dataSize = 8;
			data = new uint8_t [dataSize];
			DoubleUnion _double;
			_double.number = dataSize;
			for (size_t i = 0; i < dataSize; i++)
				data[i] = _double.buf[i];
		}
	}

	void EBMLWriteElement::SetDateData(tm * value)
	{
		uint64_t newTime = (internal_timegm(value) - 978307200) * 1000000000;
		SetUintData(newTime);
	}
}