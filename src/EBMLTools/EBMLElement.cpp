#include <EBMLTools/EBMLElement.hpp>
#include <EBMLTools/EBMLReadElement.hpp>

#include <stdexcept>

namespace EBMLTools
{
    // Static
    const EBMLElement & EBMLElement::Find(uint64_t id)
    {
        for (size_t i = 0; i < KNOWN_ELEMENTS_LENGTH; i++)
            if (id == KNOWN_ELEMENTS[i].GetElementId())
                return KNOWN_ELEMENTS[i];
        throw std::invalid_argument("Error in EBMLElement::Find. Param \"id\" does not exist.");
    }
    
    const EBMLElement & EBMLElement::Find(std::string name)
    {   
        for (size_t i = 0; i < KNOWN_ELEMENTS_LENGTH; i++)
            if (name == KNOWN_ELEMENTS[i].GetElementName())
                return KNOWN_ELEMENTS[i];
        throw std::invalid_argument("Error in EBMLElement::Find. Param \"Name\" does not exist.");
    }

    // Constructors
    EBMLElement::EBMLElement(){}
    EBMLElement::EBMLElement(std::string name, ElementType type, uint64_t id, uint64_t parentId, bool manditory, bool multiple) 
        : name(name), type(type), id(id), parentId(parentId), manditory(manditory), multiple(multiple) {}

    // Overloaded Operators
    bool EBMLElement::operator == (const EBMLElement &rhs) const { return id == rhs.id; }
    bool EBMLElement::operator !=(const EBMLElement &rhs) const { return !(*this == rhs); }
    bool EBMLElement::operator ==(const EBMLReadElement &rhs) const { return id == rhs.id;}
    bool EBMLElement::operator !=(const EBMLReadElement &rhs) const { return !(*this == rhs); }

    // Accessors
    bool EBMLElement::isGlobalElement() const { return parentId == 0; }
    bool EBMLElement::isRootElement() const { return id == parentId; }
    std::string EBMLElement::GetElementName() const { return name; }
    ElementType EBMLElement::GetElementType() const { return type; }
    uint64_t EBMLElement::GetElementId() const { return id; }
    uint64_t EBMLElement::GetElementParentId() const { return parentId; }

    size_t EBMLElement::GetElementIdByteLength() const
    {
        size_t byteLength = 0;
        uint64_t mask = 0xFF;
        while ((mask & id) != 0)
        {
            byteLength++;
            mask<<=8;
        }
        return byteLength;
    }

    size_t EBMLElement::GetElementLevel() const
    {
        size_t count = 0;
        
        EBMLElement parent = Find(id);
        while (parent.parentId != 0 && parent.id != Find(parent.id).parentId)
        {
            count++;
            parent = Find(parent.parentId);
        }
        return count;
    }

    std::stack<EBMLElement> EBMLElement::GetElementParentMap() const
    {
        std::stack<EBMLElement> map;
		EBMLElement temp = *this;
		map.push(temp);
		while(temp.GetElementLevel() != 0)
		{
			temp = Find(temp.parentId);
			map.push(temp);
		}
        return map;
    }
    
    const EBMLElement EBMLElement::KNOWN_ELEMENTS[]  = {
        /* Name, Type, ID, ParentID, manditory, multiple */
        { "Void", Binary, 0xEC, 0, false, true },
        { "CRC-32", /*Binary*/ Uint, 0xBF, 0, false, false },
        { "EBML", Master, 0x1A45DFA3, 0x1A45DFA3, true, true },
            { "EBMLVersion", Uint, 0x4286, 0x1A45DFA3, true, false },
            { "EBMLReadVersion", Uint, 0x42F7, 0x1A45DFA3, true, false },
            { "EBMLMaxIDLength", Uint, 0x42F2, 0x1A45DFA3, true, false },
            { "EBMLMaxSizeLength", Uint, 0x42F3, 0x1A45DFA3, true, false },
            { "DocType", String, 0x4282, 0x1A45DFA3, true, false },
            { "DocTypeVersion", Uint, 0x4287, 0x1A45DFA3, true, false },
            { "DocTypeReadVersion", Uint, 0x4285, 0x1A45DFA3, true, false },
        { "Segment", Master, 0x18538067, 0x18538067, true, false },
            { "SeekHead", Master, 0x114D9B74, 0x18538067, false, true },
                { "Seek", Master, 0x4DBB, 0x114D9B74, true, true },
                    { "SeekID", Uint/*Binary*/, 0x53AB, 0x4DBB, true, false },
                    { "SeekPosition", Uint, 0x53AC, 0x4DBB, true, false },
            { "Info", Master, 0x1549A966, 0x18538067, true, true },
                { "SegmentUID", Binary, 0x73A4, 0x1549A966, false, false },
                { "SegmentFilename", UTF8, 0x7384, 0x1549A966, false, false },
                { "PrevUID", Binary, 0x3CB923, 0x1549A966, false, false },
                { "PrevFilename", UTF8, 0x3C83AB, 0x1549A966, false, false },
                { "NextUID", Binary, 0x3EB923, 0x1549A966, false, false },
                { "NextFilename", UTF8, 0x3E83BB, 0x1549A966, false, false },
                { "SegmentFamily", Binary, 0x4444, 0x1549A966, false, true },
                { "ChapterTranslate", Master, 0x6924, 0x1549A966, false, true },
                    { "ChapterTranslateEditionUID", Uint, 0x69FC, 0x6924, false, false },
                    { "ChapterTranslateCodec", Uint, 0x69BF, 0x6924, false, true },
                    { "ChapterTranslateID", Binary, 0x69A5, 0x6924, false, true },
                { "TimecodeScale", Uint, 0x2AD7B1, 0x1549A966, true, false },
                { "Duration", Float, 0x4489, 0x1549A966, false, false },
                { "DateUTC", Date, 0x4461, 0x1549A966, false, false },
                { "Title", UTF8, 0x7BA9, 0x1549A966, false, false },
                { "MuxingApp", UTF8, 0x4D80, 0x1549A966, true, false },
                { "WritingApp", UTF8, 0x5741, 0x1549A966, true, false },
            { "Cluster", Master, 0x1F43B675, 0x18538067, false, true},
                { "Timecode", Uint, 0xE7, 0x1F43B675, true, false },
                { "SilentTracks", Master, 0x5854, 0x1F43B675, false, false },
                    { "SilentTrackNumber", Uint, 0x58D7, 0x5854, false, true },
                { "Position", Uint, 0xA7, 0x1F43B675, false, false },
                { "PrevSize", Uint, 0xAB, 0x1F43B675, false, false },
                { "SimpleBlock", Binary, 0xA3, 0x1F43B675, false, true },
                { "BlockGroup", Master, 0xA0, 0x1F43B675, false, true },
                    { "Block", Binary, 0xA1, 0xA0, true, false },
                    { "BlockAdditions", Master, 0x75A1, 0xA0, false, false },
                        { "BlockMore", Master, 0xA6, 0x75A1, true, true },
                            { "BlockAddID", Uint, 0xEE, 0xA6, true, false },
                            { "BlockAdditional", Binary, 0xA5, 0xA6, true, false },
                    { "BlockDuration", Uint, 0x9B, 0xA0, false, false },
                    { "ReferencePriority", Uint, 0xFA, 0xA0, true, false },
                    { "ReferenceBlock", Int, 0xFB, 0xA0, false, true },
                    { "CodecState", Binary, 0xA4, 0xA0, false, false },
                    { "DiscardPadding", Int, 0x75A2, 0xA0, false, false },
                    { "Slices", Master, 0x8E, 0xA0, false, false },
                        { "TimeSlice", Master, 0xE8, 0x8E, false, true },
                            { "LaceNumber", Uint, 0xCC, 0xE8, false, false },
            { "Tracks", Master, 0x1654AE6B, 0x18538067, false, true },
                { "TrackEntry", Master, 0xAE, 0x1654AE6B, true, true },
                    { "TrackNumber", Uint, 0xD7, 0xAE, true, false },
                    { "TrackUID", Uint, 0x73C5, 0xAE, true, false },
                    { "TrackType", Uint, 0x83, 0xAE, true, false },
                    { "FlagEnabled", Uint, 0xB9, 0xAE, true, false },
                    { "FlagDefault", Uint, 0x88, 0xAE, true, false },
                    { "FlagForced", Uint, 0x55AA, 0xAE, true, false },
                    { "FlagLacing", Uint, 0x9C, 0xAE, true, false },
                    { "MinCache", Uint, 0x6DE7, 0xAE, true, false },
                    { "MaxCache", Uint, 0x6DF8, 0xAE, false, false },
                    { "DefaultDuration", Uint, 0x23E383, 0xAE, false, false },
                    { "DefaultDecodedFieldDuration", Uint, 0x234E7A, 0xAE, false, false },
                    { "TrackTimecodeScale", Float, 0x23314F, 0xAE, true, false },
                    { "TrackOffset", Int, 0x537F, 0xAE, false, false },
                    { "MaxBlockAdditionID", Uint, 0x55EE, 0xAE, true, false },
                    { "Name", UTF8, 0x536E, 0xAE, false, false },
                    { "Language", String, 0x22B59C, 0xAE, false, false },
                    { "CodecID", String, 0x86, 0xAE, true, false },
                    { "CodecPrivate", Binary, 0x63A2, 0xAE, false, false },
                    { "CodecName", UTF8, 0x258688, 0xAE, false, false },
                    { "AttachmentLink", Uint, 0x7446, 0xAE, false, false },
                    { "CodecSettings", UTF8, 0x3A9697, 0xAE, false, false },
                    { "CodecInfoURL", String, 0x3B4040, 0xAE, false, true },
                    { "CodecDownloadURL", String, 0x26B240, 0xAE, false, true },
                    { "CodecDecodeAll", Uint, 0xAA, 0xAE, true, false },
                    { "TrackOverlay", Uint, 0x6FAB, 0xAE, false, true },
                    { "CodecDelay", Uint, 0x56AA, 0xAE, false, false },
                    { "SeekPreRoll", Uint, 0x56BB, 0xAE, true, false },
                    { "TrackTranslate", Master, 0x6624, 0xAE, false, true },
                        { "TrackTranslateEditionUID", Uint, 0x66FC, 0x6624, false, true },
                        { "TrackTranslateCodec", Uint, 0x66BF, 0x6624, true, false },
                        { "TrackTranslateTrackID", Binary, 0x66A5, 0x6624, true, false },
                    { "Video", Master, 0xE0, 0xAE, false, false },
                        { "FlagInterlaced", Uint, 0x9A, 0xE0, true, false },
                        { "FieldOrder", Uint, 0x9D, 0xE0, true, false },
                        { "StereoMode", Uint, 0x53B8, 0xE0, false, false },
                        { "AlphaMode", Uint, 0x53C0, 0xE0, false, false },
                        { "OldStereoMode", Uint, 0x53B9, 0xE0, false, false },
                        { "PixelWidth", Uint, 0xB0, 0xE0, true, false },
                        { "PixelHeight", Uint, 0xBA, 0xE0, true, false },
                        { "PixelCropBottom", Uint, 0x54AA, 0xE0, false, false },
                        { "PixelCropTop", Uint, 0x54BB, 0xE0, false, false },
                        { "PixelCropLeft", Uint, 0x54CC, 0xE0, false, false },
                        { "PixelCropRight", Uint, 0x54DD, 0xE0, false, false },
                        { "DisplayWidth", Uint, 0x54B0, 0xE0, false, false },
                        { "DisplayHeight", Uint, 0x54BA, 0xE0, false, false },
                        { "DisplayUnit", Uint, 0x54B2, 0xE0, false, false },
                        { "AspectRatioType", Uint, 0x54B3, 0xE0, false, false },
                        { "ColourSpace", Binary, 0x2EB524, 0xE0, false, false },
                        { "GammaValue", Float, 0x2FB523, 0xE0, false, false },
                        { "FrameRate", Float, 0x2383E3, 0xE0, false, false },
                        { "Colour", Master, 0x55B0, 0xE0, false, false },
                            { "MatrixCoefficients", Uint, 0x55B1, 0x55B0, false, false },
                            { "BitsPerChannel", Uint, 0x55B2, 0x55B0, false, false },
                            { "ChromaSubsamplingHorz", Uint, 0x55B3, 0x55B0, false, false },
                            { "ChromaSubsamplingVert", Uint, 0x55B4, 0x55B0, false, false },
                            { "CbSubsamplingHorz", Uint, 0x55B5, 0x55B0, false, false },
                            { "CbSubsamplingVert", Uint, 0x55B6, 0x55B0, false, false },
                            { "ChromaSitingHorz", Uint, 0x55B7, 0x55B0, false, false },
                            { "ChromaSitingVert", Uint, 0x55B8, 0x55B0, false, false },
                            { "Range", Uint, 0x55B9, 0x55B0, false, false },
                            { "TransferCharacteristics", Uint, 0x55BA, 0x55B0, false, false },
                            { "Primaries", Uint, 0x55BB, 0x55B0, false, false },
                            { "MaxCLL", Uint, 0x55BC, 0x55B0, false, false },
                            { "MaxFALL", Uint, 0x55BD, 0x55B0, false, false },
                            { "MasteringMetadata", Master, 0x55D0, 0x55B0, false, false },
                                { "PrimaryRChromaticityX", Float, 0x55D1, 0x55D0, false, false },
                                { "PrimaryRChromaticityY", Float, 0x55D2, 0x55D0, false, false },
                                { "PrimaryGChromaticityX", Float, 0x55D3, 0x55D0, false, false },
                                { "PrimaryGChromaticityY", Float, 0x55D4, 0x55D0, false, false },
                                { "PrimaryBChromaticityX", Float, 0x55D5, 0x55D0, false, false },
                                { "PrimaryBChromaticityY", Float, 0x55D6, 0x55D0, false, false },
                                { "WhitePointChromaticityX", Float, 0x55D7, 0x55D0, false, false },
                                { "WhitePointChromaticityY", Float, 0x55D8, 0x55D0, false, false },
                                { "LuminanceMax", Float, 0x55D9, 0x55D0, false, false },
                                { "LuminanceMin", Float, 0x55DA, 0x55D0, false, false },
                    { "Audio", Master, 0xE1, 0xAE, false, false },
                        { "SamplingFrequency", Float, 0xB5, 0xE1, true, false },
                        { "OutputSamplingFrequency", Float, 0x78B5, 0xE1, false, false },
                        { "Channels", Uint, 0x9F, 0xE1, true, false },
                        { "ChannelPositions", Uint, 0x7D7B, 0xE1, false, false },
                        { "BitDepth", Uint, 0x6264, 0xE1, false, false },
                    { "TrackOperation", Master, 0xE2, 0xAE, false, false },
                        { "TrackCombinePlanes", Master, 0xE3, 0xE2, false, false },
                            { "TrackPlane", Master, 0xE4, 0xE3, true, true },
                                { "TrackPlaneUID", Uint, 0xE5, 0xE4, true, true },
                                { "TrackPlaneUID", Uint, 0xE6, 0xE4, true, true },
                        { "TrackJoinBlocks", Master, 0xE9, 0xE2, false, false },
                            { "TrackJoinBlocks", Uint, 0xED, 0xE9, true, true },
                    { "TrickTrackUID", Uint, 0xC0, 0xAE, false, false },
                    { "TrickTrackSegmentUID", Binary, 0xC1, 0xAE, false, false },
                    { "TrickTrackFlag", Uint, 0xC6, 0xAE, false, false },
                    { "TrickMasterTrackUID", Uint, 0xC7, 0xAE, false, false },
                    { "TrickMasterTrackSegmentUID", Binary, 0xC4, 0xAE, false, false },
                    { "ContentEncodings", Master, 0x6D80, 0xAE, false, false },
                        { "ContentEncoding", Master, 0x6240, 0x6D80, true, true },
                            { "ContentEncodingOrder", Uint, 0x5031, 0x6240, true, false },
                            { "ContentEncodingScope", Uint, 0x5032, 0x6240, true, false },
                            { "ContentEncodingType", Uint, 0x5033, 0x6240, true, false },
                            { "ContentCompression", Master, 0x5034, 0x6240, false, false },
                                { "ContentCompAlgo", Uint, 0x5254, 0x5034, true, false },
                                { "ContentCompSettings", Binary, 0x5255, 0x5034, false, false },
                            { "ContentEncryption", Master, 0x5035, 0x6240, false, false },
                                { "ContentEncAlgo", Uint, 0x47E1, 0x5035, false, false },
                                { "ContentEncKeyID", Binary, 0x47E1, 0x5035, false, false },
                                { "ContentSignature", Binary, 0x47E1, 0x5035, false, false },
                                { "ContentSigKeyID", Binary, 0x47E1, 0x5035, false, false },
                                { "ContentSigAlgo", Uint, 0x47E1, 0x5035, false, false },
                                { "ContentSigHashAlgo", Uint, 0x47E1, 0x5035, false, false },
            { "Cues", Master, 0x1C53BB6B, 0x18538067, false, false },
                { "CuePoint", Master, 0xBB, 0x1C53BB6B, true, true },
                    { "CueTime", Uint, 0xB3, 0xBB, true, false },
                    { "CueTrackPositions", Master, 0xB7, 0xBB, true, true },
                        { "CueTrack", Uint, 0xF7, 0xB7, true, false },
                        { "CueClusterPosition", Uint, 0xF1, 0xB7, true, false },
                        { "CueRelativePosition", Uint, 0xF0, 0xB7, false, false },
                        { "CueDuration", Uint, 0xB2, 0xB7, false, false },
                        { "CueBlockNumber", Uint, 0x5378, 0xB7, false, false },
                        { "CueCodecState", Uint, 0xEA, 0xB7, false, false },
                        { "CueReference", Master, 0xDB, 0xB7, false, true },
                            { "CueRefTime", Uint, 0x96, 0xDB, true, false },
                            { "CueRefCluster", Uint, 0x97, 0xDB, true, false },
                            { "CueRefNumber", Uint, 0x535F, 0xDB, false, false },
                            { "CueRefCodecState", Uint, 0xEB, 0xDB, false, false },
            { "Attachments", Master, 0x1941A469, 0x18538067, false, false },
                { "AttachedFile", Master, 0x61A7, 0x1941A469, true, true },
                    { "FileDescription", UTF8, 0x467E, 0x61A7, false, false },
                    { "FileName", UTF8, 0x466E, 0x61A7, true, false },
                    { "FileMimeType", String, 0x4660, 0x61A7, true, false },
                    { "FileData", Binary, 0x465C, 0x61A7, true, false },
                    { "FileUID", Uint, 0x46AE, 0x61A7, true, false },
                    { "FileReferral", Binary, 0x4675, 0x61A7, false, false },
                    { "FileUsedStartTime", Uint, 0x4661, 0x61A7, false, false },
                    { "FileUsedEndTime", Uint, 0x4662, 0x61A7, false, false },
            { "Chapters", Master, 0x1043A770, 0x18538067, false, false },
                { "EditionEntry", Master, 0x45B9, 0x1043A770, true, true },
                    { "EditionUID", Uint, 0x45BC, 0x45B9, false, false },
                    { "EditionFlagHidden", Uint, 0x45BD, 0x45B9, true, false },
                    { "EditionFlagDefault", Uint, 0x45DB, 0x45B9, true, false },
                    { "EditionFlagOrdered", Uint, 0x45DD, 0x45B9, false, false },
                    { "ChapterAtom", Master, 0xB6, 0x45B9, true, true },
                        { "ChapterUID", Uint, 0x73C4, 0xB6, true, false },
                        { "ChapterStringUID", UTF8, 0x5654, 0xB6, false, false },
                        { "ChapterTimeStart", Uint, 0x91, 0xB6, true, false },
                        { "ChapterTimeEnd", Uint, 0x92, 0xB6, false, false },
                        { "ChapterFlagHidden", Uint, 0x98, 0xB6, true, false },
                        { "ChapterFlagEnabled", Uint, 0x4598, 0xB6, true, false },
                        { "ChapterSegmentUID", Binary, 0x6E67, 0xB6, false, false },
                        { "ChapterSegmentEditionUID", Uint, 0x6EBC, 0xB6, false, false },
                        { "ChapterPhysicalEquiv", Uint, 0x63C3, 0xB6, false, false },
                        { "ChapterTrack", Master, 0x8F, 0xB6, false, false },
                            { "ChapterTrackNumber", Uint, 0x89, 0x8F, true, true },
                        { "ChapterDisplay", Master, 0x80, 0xB6, false, true },
                            { "ChapString", UTF8, 0x85, 0x80, true, false },
                            { "ChapLanguage", String, 0x437C, 0x80, true, true },
                            { "ChapCountry", String, 0x437E, 0x80, false, true },
                        { "ChapProcess", Master, 0x6944, 0xB6, false, true },
                            { "ChapProcessCodecID", Uint, 0x6955, 0x6944, true, false },
                            { "ChapProcessPrivate", Binary, 0x450D, 0x6944, false, false },
                            { "ChapProcessCommand", Master, 0x6911, 0x6944, false, true },
                                { "ChapProcessTime", Uint, 0x6922, 0x6911, true, false },
                                { "ChapProcessData", Binary, 0x6933, 0x6911, true, false },
            { "Tags", Master, 0x1254C367, 0x18538067, false, true },
                { "Tag", Master, 0x7373, 0x1254C367, true, true },
                    { "Targets", Master, 0x63C0, 0x7373, true, false },
                        { "TargetTypeValue", Uint, 0x68CA, 0x63C0, false, false },
                        { "TargetType", String, 0x63CA, 0x63C0, false, false },
                        { "TagTrackUID", Uint, 0x63C5, 0x63C0, false, true },
                        { "TagEditionUID", Uint, 0x63C9, 0x63C0, false, true },
                        { "TagChapterUID", Uint, 0x63C4, 0x63C0, false, true },
                        { "TagAttachmentUID", Uint, 0x63C6, 0x63C0, false, true },
                    { "SimpleTag", Master, 0x67C8, 0x7373, true, true },
                        { "TagName", UTF8, 0x45A3, 0x67C8, true, false },
                        { "TagLanguage", String, 0x447A, 0x67C8, true, false },
                        { "TagDefault", Uint, 0x4484, 0x67C8, true, false },
                        { "TagString", UTF8, 0x4487, 0x67C8, false, false },
                        { "TagBinary", Binary, 0x4485, 0x67C8, false, false }
    };

}