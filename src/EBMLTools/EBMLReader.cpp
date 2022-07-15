#include <EBMLTools/EBMLReader.hpp>

namespace EBMLTools
{
	// PRIVATE STATIC
	uint8_t EBMLReader::ParseBlockLength(uint8_t value)
	{
		if (value == 0xFF)
			throw std::invalid_argument("Unknown Block Length's are not supported...");
		if (value == 0x00)
			throw std::invalid_argument("Block Length's greater than 8 bits are currently not supported");
		uint8_t length = 0;
		uint8_t mask = 0x80; // 1000 0000
		uint8_t cmp = 0;	 // 0000 0000

		while (cmp != value)
		{
			cmp = value | mask; // Bitwise OR the value against the mask
			mask >>= 1; 		// Bitwise Shift the mask right by 1
			length++;			// Increase the length
		}
		return length;
	}

	uint8_t EBMLReader::ParseBlockData(uint8_t byte, uint8_t blockLength)
	{
		byte <<= blockLength;
		byte >>= blockLength;
		return byte;
	}
	
	// PRIVATE
	bool EBMLReader::EndOfRead() { return GetReadPosition() >= fileSize; }
	void EBMLReader::SetReadPosition(size_t position) { fileStream.seekg(position, std::ios::beg); }
	size_t EBMLReader::GetReadPosition() { return (size_t) fileStream.tellg(); }

	uint64_t EBMLReader::ReadNextBlock(uint8_t &length, bool isSize)
	{
		uint64_t value = 0;
		uint8_t * bytes = new uint8_t[length];
		fileStream.read((char *) bytes, length);
		if (isSize)
		{
			*bytes <<= length;
			*bytes >>= length;
		}
		uint8_t maxLength = isSize ? maxSizeLength : maxIdLength;
		if (length == 0 || length > maxLength)
			throw std::invalid_argument("Length of next block is not valid.. in EBMLReader::ReadNextBlock");
		for (std::size_t i = length; i != 0; i--)
			value |= (uint64_t) bytes[length - i] << ((i * 8) - 8);
		delete [] bytes;
		return value;
	}

	uint8_t EBMLReader::GetNextByte()
	{
		uint8_t nextByte;
		fileStream.read((char *) &nextByte, sizeof(nextByte));
		fileStream.seekg(-(sizeof(nextByte)), std::ios::cur);
		return nextByte;
	}

	EBMLReadElement EBMLReader::ReadElement(ReadMode mode)
	{
		if (EndOfRead())
			throw std::out_of_range("EBMLReader::ReadElement: Reached end of file..");

		size_t position = GetReadPosition();

		uint8_t idByteLength = ParseBlockLength(GetNextByte());
		uint64_t id = ReadNextBlock(idByteLength);
		uint8_t dataSizeByteLength = ParseBlockLength(GetNextByte());
		uint64_t dataSize = ReadNextBlock(dataSizeByteLength, true);

		EBMLReadElement ele (this, EBMLElement::Find(id), dataSize, dataSizeByteLength, position);

		if (ele.GetElementType() == Master)
			parentStructure[ele.GetElementPosition()] = std::make_pair(ele.GetElementByteLength(), ele.GetElementId());

		if (ele.GetElementType() != Master || mode == ReadMode::Normal)
			fileStream.seekg(dataSize, std::ios::cur);

		return ele;
	}

	EBMLReadElement EBMLReader::GetElement(size_t fileposition)
	{
		if (fileposition < 0 || fileposition > fileSize)
			throw std::out_of_range("fileposition out of range of file. EBMLReader::ReadElement(uint64_t)");
		size_t cachedPosition = GetReadPosition();
		SetReadPosition(fileposition);
		EBMLReadElement element = ReadElement();
		SetReadPosition(cachedPosition);
		return element;
	}

	EBMLReadElement EBMLReader::operator [] (size_t fileposition) { return GetElement(fileposition); }

	// PUBLIC
	EBMLReader::EBMLReader(){}
	EBMLReader::EBMLReader(std::string file, bool dataIntegrityCheck) { OpenFile(file, dataIntegrityCheck); }
	EBMLReader::~EBMLReader() { if (fileStream.is_open()) CloseFile(); }

	void EBMLReader::OpenFile(std::string file, bool dataIntegrityCheck)
	{
		integrityCheck = dataIntegrityCheck;
		fileStream.open(file.c_str(), std::ios::binary | std::ios::in);
		if (fileStream.fail())
			throw std::ifstream::failure("The file: " + file + " is inaccessable");
		fileName = file;
		fileStream.seekg(0, std::ios::end);
		fileSize = GetReadPosition();
		SetReadPosition(0);

		EBMLReadElement ebmlHeader = ReadElement(); // ebml
		for(auto child : ebmlHeader.Children())
		{
			if (child.GetElementName() == "EBMLMaxIDLength")
				maxIdLength = child.GetUintData();
			if (child.GetElementName() == "EBMLMaxSizeLength")
				maxSizeLength = child.GetUintData();
		}
		EBMLReadElement segment = ReadElement(); //segment
		EBMLReadElement seekHead = segment.FirstChild();
		if (seekHead.GetElementName() == "SeekHead")
		{
			firstSeekHead = std::make_unique<EBMLReadElement>(std::move(seekHead));
			for (auto & seek : firstSeekHead->Children(EBMLElement::Find("Seek")))
			{
				size_t pos = seek.Children(EBMLElement::Find("SeekPosition")).at(0).GetUintData() + firstSeekHead->GetElementPosition();
				uint64_t id = seek.Children(EBMLElement::Find("SeekID")).at(0).GetUintData();
				this->seekHead[pos] = id;
			}
		}
		SetReadPosition(0);
	}

	void EBMLReader::CloseFile()
	{
		fileStream.close();
		fileName = "";
		fileSize = 0;
		seekHead.clear();
		parentStructure.clear();
		maxIdLength = 4;
		maxSizeLength = 4;
	}

	const std::string EBMLReader::GetFilename() const
	{
		return fileName;
	}

	void EBMLReader::DisableDataIntegrityCheck() { integrityCheck = false; }
	void EBMLReader::EnableDataIntegrityCheck() { integrityCheck = true; }

	std::vector<EBMLReadElement> EBMLReader::GetRootElements()
	{
		std::vector<EBMLReadElement> results;
		size_t cachedposition = GetReadPosition();
		SetReadPosition(0);
		while (!EndOfRead())
			results.push_back(ReadElement());
		SetReadPosition(cachedposition);
		return results;
	}

	std::vector<EBMLReadElement> EBMLReader::GetRootElements(const EBMLElement & filter)
	{
		if (!filter.isRootElement())
			throw std::invalid_argument("EBMLReader::GetRootElements(EBMLElement filter), filter must be a root element");
		std::vector<EBMLReadElement> results;
		size_t cachedposition = GetReadPosition();
		SetReadPosition(0);
		while (!EndOfRead())
		{
			EBMLReadElement element = ReadElement();
			if (filter == element)
				results.push_back(element);
		}
		SetReadPosition(cachedposition);
		return results;
	}

	std::vector<EBMLReadElement> EBMLReader::Search(const EBMLElement & query)
	{
		if (query.isGlobalElement())
			throw std::invalid_argument("EBMLReader::Search(), cannot search for global elements.");

		std::stack<EBMLElement> queryMap = query.GetElementParentMap();
		std::vector<std::vector<EBMLReadElement>> cache(queryMap.size());

		cache[0] = GetRootElements(queryMap.top());
		queryMap.pop();

		for (size_t i = 0; i < cache.size() - 1; i++)
		{
			for (auto &ele : cache[i])
				for (auto &child : ele.Children(queryMap.top()))
					cache[i + 1].push_back(child);
			queryMap.pop();
		}
		
		return cache[cache.size() - 1];
	}

	std::vector<EBMLReadElement> EBMLReader::FastSearch(const EBMLElement & query)
	{
		if (query.isGlobalElement())
			throw std::invalid_argument("EBMLReader::FastSearch(), cannot search for global elements.");
		
		std::stack<EBMLElement> queryMap = query.GetElementParentMap();

		if (queryMap.top().GetElementName() != "Segment" || query.isRootElement())
			return Search(query);
		queryMap.pop();

		std::vector<uint64_t> positions;
		for (auto &seek : seekHead)
			if (seek.second == queryMap.top().GetElementId())
				positions.push_back(seek.first);

		if (positions.size() == 0)
			return Search(query);

		std::vector<std::vector<EBMLReadElement>> cache(queryMap.size());

		for(auto & i : positions)
			cache[0].push_back(GetElement(i));
		queryMap.pop();

		for (size_t i = 0; i < cache.size() - 1; i++)
		{
			for (auto &ele : cache[i])
				for (auto &child : ele.Children(queryMap.top()))
					cache[i + 1].push_back(child);
			queryMap.pop();
		}

		return cache[cache.size() - 1];
	}
}
