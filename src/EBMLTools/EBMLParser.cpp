#include <EBMLTools/EBMLParser.hpp>

#include <algorithm>

namespace EBMLTools
{
	EBMLParser::EBMLParser() : EBMLReader() {};
	EBMLParser::EBMLParser(std::string file, bool dataIntegrityCheck) { OpenFile(file, dataIntegrityCheck); }
	void EBMLParser::OpenFile(std::string file, bool dataIntegrityCheck)
	{
		EBMLReader::OpenFile(file, dataIntegrityCheck);
		fileStream.close();
		fileStream.clear();
		fileStream.open(file.c_str(), std::ios::binary | std::ios::in | std::ios::out);
		if (fileStream.fail())
			throw std::ifstream::failure("The file: " + file + " is not writeable");
	}

	EBMLWriteElement EBMLParser::CreateVoid(uint64_t totalSize)
	{
		if (totalSize < 2)
			throw std::logic_error("EBMLParser::CreateVoid(), cannot create a void element smaller than 2 bytes.");
		EBMLWriteElement wele(EBMLElement::Find("Void"));
		totalSize--; //0xEC Void ID (1 byte)
		wele.dataSizeByteLength = EBMLWriteElement::DetermineByteLengthOfValue(totalSize);
		wele.dataSize = totalSize - wele.dataSizeByteLength;
		return wele;
	}

	uint8_t * EBMLParser::CreateBlock(const uint64_t &value, const size_t &byteLength, bool encode)
	{
		uint8_t * encodedBytes = new uint8_t[byteLength];
		for (size_t i = 0; i < byteLength; i++)
			encodedBytes[byteLength - 1 - i] = (value >> (i * 8));
		if (encode)
			encodedBytes[0] = encodedBytes[0] | (0x80 >> (byteLength - 1));
		return encodedBytes;
	}

	void EBMLParser::SetWritePosition(size_t position) { fileStream.seekp(position, std::ios::beg); }
	
	size_t EBMLParser::GetWritePosition() { return (size_t) fileStream.tellp(); }

	void EBMLParser::MergeConsecutiveVoidElements()
	{
		EBMLReadElement segment = GetRootElements(EBMLElement::Find("Segment")).at(0);
		std::map<size_t, size_t> toMerge;
		EBMLReadElement * lastVoid = NULL;
		for (auto &voidChild : segment.Children(EBMLElement::Find("Void")))
		{
			if (lastVoid != NULL)
			{
				if (voidChild.GetElementPosition() == lastVoid->GetElementPosition() + lastVoid->GetElementByteLength())
				{
					bool found = false;
					for (auto &e : toMerge)
					{
						if (e.first + e.second == voidChild.GetElementPosition())
						{
							found = true;
							e.second += voidChild.GetElementByteLength();
						}
					}
					if (!found)
						toMerge[lastVoid->GetElementPosition()] = voidChild.GetElementByteLength() + lastVoid->GetElementByteLength();
				}
			}
			lastVoid = &voidChild;
		}
		for (auto &merge : toMerge)
		{
			SetWritePosition(merge.first);
			EBMLWriteElement newVoid = CreateVoid(merge.second);
			RawWrite(newVoid);
		}
	}

	EBMLWriteElement EBMLParser::CreateSeekHead()
	{
		EBMLReadElement segment = GetRootElements(EBMLElement::Find("Segment")).at(0);
		EBMLWriteElement eleSeekHead(EBMLElement::Find("SeekHead"));
		for (auto & seek : seekHead)
		{
			auto eleSeek = std::make_unique<EBMLWriteElement>(EBMLElement::Find("Seek"));
			auto eleSeekPosition = std::make_unique<EBMLWriteElement>(EBMLElement::Find("SeekPosition"));
			auto eleSeekID = std::make_unique<EBMLWriteElement>(EBMLElement::Find("SeekID"));
			eleSeekPosition->SetUintData(seek.first - segment.GetElementPosition() - segment.GetElementIdByteLength() - segment.GetElementDataSizeByteLength());
			eleSeekID->SetUintData(seek.second);
			eleSeek->Children().push_back(std::move(eleSeekID));
			eleSeek->Children().push_back(std::move(eleSeekPosition));
			eleSeekHead.Children().push_back(std::move(eleSeek));
		}
		eleSeekHead.Validate();
		return eleSeekHead;
	}

	void EBMLParser::OverwriteElement(EBMLReadElement & ele, EBMLWriteElement & wele)
	{
		SetWritePosition(ele.GetElementPosition());
		if (ele.GetElementByteLength() == wele.GetElementByteLength())
			RawWrite(wele);
		else
		{
			uint64_t diff = ele.GetElementByteLength() - wele.GetElementByteLength();
			if (diff < 2) {
				wele.dataSizeByteLength++;
				RawWrite(wele);
			} else {
				RawWrite(wele);
				EBMLWriteElement voidEle = CreateVoid(diff);
				RawWrite(voidEle);
				MergeConsecutiveVoidElements();
			}
		}
	}

	bool putVoidsUpTop(EBMLReadElement i, EBMLReadElement j) { return (i.GetElementName() == "Void"); }

	void EBMLParser::UpdateSeekHead()
	{
		if (firstSeekHead == NULL)
			throw std::invalid_argument("EBMLParser::UpdateSeekHead(). SeekHead does not exist.");
		
		size_t seekHeadPosition = firstSeekHead->GetElementPosition();
		EBMLWriteElement voidOutEle = CreateVoid(firstSeekHead->GetElementByteLength());
		SetWritePosition(seekHeadPosition);
		RawWrite(voidOutEle);
		MergeConsecutiveVoidElements();

		EBMLReadElement segment = GetRootElements(EBMLElement::Find("Segment")).at(0);

		EBMLWriteElement newSeekHead = CreateSeekHead();
		if (newSeekHead.GetElementByteLength() > GetElement(seekHeadPosition).GetElementByteLength()) {
			std::vector<EBMLReadElement> elesBeforeCluster;
			for (auto & child : segment.Children())
			{
				if (child.GetElementName() == "Cluster")
					break;
				elesBeforeCluster.push_back(child);
			}
			std::sort(elesBeforeCluster.begin(), elesBeforeCluster.end(), putVoidsUpTop);
			std::vector<std::unique_ptr<EBMLWriteElement>> elesToWrite;
			for (auto eleBeforeCluster : elesBeforeCluster)
			{
				seekHead.erase(eleBeforeCluster.GetElementPosition());
				auto toMove = std::make_unique<EBMLWriteElement>(eleBeforeCluster);
				elesToWrite.push_back(std::move(toMove));
			}
			SetWritePosition(seekHeadPosition);
			for (auto &wele : elesToWrite)
			{
				if (wele->GetElementName() != "Void")
					seekHead[GetWritePosition()] = wele->GetElementId();
				RawWrite(*wele);
			}
			MergeConsecutiveVoidElements();
			newSeekHead = CreateSeekHead();
		}
		while (newSeekHead.GetElementByteLength() > GetElement(seekHeadPosition).GetElementByteLength())
		{
			EBMLReadElement secondChildOfSegment = segment.Children().at(1);
			if (secondChildOfSegment.GetElementName() == "Cluster")
				throw std::logic_error("Cannot relocate Cluster elements");
			seekHead.erase(secondChildOfSegment.GetElementPosition());
			EBMLWriteElement voidElement = CreateVoid(secondChildOfSegment.GetElementByteLength());
			EBMLWriteElement toAppend(secondChildOfSegment);
			SetWritePosition(secondChildOfSegment.GetElementPosition());
			RawWrite(voidElement);
			MergeConsecutiveVoidElements();
			seekHead[fileSize] = toAppend.GetElementId();
			AppendElement(toAppend);
			newSeekHead = CreateSeekHead();
		}
		EBMLReadElement firstVoidElement = segment.FirstChild();
		OverwriteElement(firstVoidElement, newSeekHead);
		*firstSeekHead = GetElement(seekHeadPosition);
	}

	void EBMLParser::UpdateElement(EBMLReadElement & ele, EBMLWriteElement & wele)
	{
		if (ele != wele)
			throw std::invalid_argument("EBMLParser::UpdateElement(). The ReadElement and WriteElement are not the same element.");
		if (ele.GetElementLevel() != 1)
			throw std::invalid_argument("EBMLParser::UpdateElement(). The element being updated is not a level 1 element. (child of segment). This must be the case.");
		if (ele.GetElementName() == "SeekHead")
			throw std::invalid_argument("EBMLParser::UpdateElement(). SeekHead is automatically updated as elements are updated or added. So you can't use this method for raw SeekHead manipulation");
		wele.Validate();
		if (ele.GetElementByteLength() < wele.GetElementByteLength())
		{
			EBMLWriteElement voidOutEle = CreateVoid(ele.GetElementByteLength());
			SetWritePosition(ele.GetElementPosition());
			RawWrite(voidOutEle);
			MergeConsecutiveVoidElements();
			if (firstSeekHead != NULL)
				seekHead.erase(ele.GetElementPosition());
			auto voidElements = firstSeekHead->Parent().Children(EBMLElement::Find("Void"));
			EBMLReadElement * voidEle = NULL;
			for (auto & i : voidElements) 
			{
				if (i.GetElementByteLength() >= wele.GetElementByteLength())
				{
					voidEle = &i;
					break;
				}
			}
			if (voidEle != NULL)
			{
				if (firstSeekHead != NULL)
					seekHead[voidEle->GetElementPosition()] = wele.GetElementId();
				OverwriteElement(*voidEle, wele);
			}
			else
			{
				if (firstSeekHead != NULL)
					seekHead[fileSize] = wele.GetElementId();
				AppendElement(wele);
			}
			if (firstSeekHead != NULL)
				UpdateSeekHead();
		}
		else
			OverwriteElement(ele, wele);
	}

	void EBMLParser::AppendElement(EBMLWriteElement & wele)
	{
		SetWritePosition(fileSize);
		RawWrite(wele);
		EBMLReadElement segment = GetRootElements(EBMLElement::Find("Segment")).at(0);
		uint64_t newDataSize = segment.GetElementDataSize() + wele.GetElementByteLength();
		uint8_t newDataSizeByteLength = EBMLWriteElement::DetermineByteLengthOfValue(newDataSize);
		if (newDataSizeByteLength > segment.GetElementDataSizeByteLength())
			// This will involve adjusting any referenced position within all seek and cue entries as any position referenced is the offset from the segment data start position, 
			// which will be now increased because we need to increase the segment's dataSizeByteLength, which will in turn push the data start position over...
			throw std::logic_error("Unable to increase segement data size. Not implemented yet...");
		else
		{
			SetWritePosition(segment.GetElementPosition() + segment.GetElementIdByteLength());
			uint8_t * size = CreateBlock(newDataSize, segment.GetElementDataSizeByteLength(), true);
			fileStream.write((char *)size, segment.GetElementDataSizeByteLength());
		}
	}

	void EBMLParser::AddElement(EBMLWriteElement & wele)
	{
		if (wele.GetElementLevel() != 1)
			throw std::invalid_argument("EBMLParser::AddElement(). The element being updated is not a level 1 element. (child of segment). This must be the case.");
		if (wele.GetElementName() == "SeekHead")
			throw std::invalid_argument("EBMLParser::AddElement(). SeekHead is automatically updated as elements are updated or added. So you can't use this method to add SeekHead elements.");

		wele.Validate();

		EBMLReadElement segment = GetRootElements(EBMLElement::Find("Segment")).at(0);
		EBMLReadElement * eligableVoid = NULL;
		for (auto &voidChild : segment.Children(EBMLElement::Find("Void")))
		{
			if (voidChild.GetElementByteLength() >= wele.GetElementByteLength())
			{
				eligableVoid = &voidChild;
				break;
			}
		}
		if (eligableVoid != NULL)
		{
			if (firstSeekHead)
				seekHead[eligableVoid->GetElementPosition()] = wele.GetElementId();
			OverwriteElement(*eligableVoid, wele);
		} else {
			if (firstSeekHead)
				seekHead[fileSize] = wele.GetElementId();
			AppendElement(wele);
		}

		if (firstSeekHead)
			UpdateSeekHead();
	}

	void EBMLParser::RawWrite(const EBMLWriteElement & wele)
	{
		uint8_t * id = CreateBlock(wele.id, wele.GetElementIdByteLength(), false);
		fileStream.write((char *)id, wele.GetElementIdByteLength());	// WRITE ID (ID is already pre-encoded)
		delete [] id;
		uint8_t * size = CreateBlock(wele.dataSize, wele.dataSizeByteLength, true);
		fileStream.write((char *)size, wele.dataSizeByteLength);		// WRITE ENCODED SIZE
		delete [] size;

		if (wele.GetElementId() == 0xEC) // if void, write emtpy bytes;
			for (size_t i = 0; i < wele.dataSize; i++)
				fileStream.write((char *)&ZEROBYTE, sizeof(ZEROBYTE));
		else  if (wele.type != Master) // else if Element is NOT a Master element; Has data..
			fileStream.write((char*)wele.data, wele.dataSize);
		else	// Otherwise, recurse through children
			for (auto &child : wele.Children())
				RawWrite(*child);

		if (GetWritePosition() > fileSize)
			fileSize = GetWritePosition();
	}
} 