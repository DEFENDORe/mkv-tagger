#include <iostream>
#include <iomanip>

#include <cxxopts.hpp>
#include <EBMLTools/EBMLParser.hpp>
#include <TMDB/API.hpp>
#include <web++.hpp>

int searchEBML(EBMLTools::EBMLParser &ebmlParser, cxxopts::ParseResult &result);
int displayInfo(EBMLTools::EBMLParser &ebmlParser, cxxopts::ParseResult &result);
int FindMediaThenTag(TMDB::API &tmdbApi, EBMLTools::EBMLParser &ebmlParser, cxxopts::ParseResult &result);
Json::Value searchForMovie(TMDB::API &tmdbApi);
Json::Value searchForTVShow(TMDB::API &tmdbApi);
uint32_t searchForTVShowSeason(TMDB::API &tmdbApi, Json::Value &tv);
uint32_t searchForTVShowEpisode(TMDB::API &tmdbApi, Json::Value &tv, uint32_t tvseason);

std::unique_ptr<EBMLTools::EBMLWriteElement> CreateNewTagsElementForMovie(Json::Value &movie);
std::unique_ptr<EBMLTools::EBMLWriteElement> CreateNewTagsElementForTVShow(TMDB::API &tmdbApi, Json::Value &tv, uint32_t season, uint32_t episode);

std::unique_ptr<EBMLTools::EBMLWriteElement> CreateNewAttachmentsElementForMovie(Json::Value movie);
std::unique_ptr<EBMLTools::EBMLWriteElement> CreateNewAttachmentsElementForTVShow(Json::Value tv, uint32_t season);

void WriteMovieTags(Json::Value &movie, EBMLTools::EBMLParser &ebmlParser);
void WriteTVShowTags(Json::Value &tv, uint32_t season, uint32_t episode, EBMLTools::EBMLParser &ebmlParser, TMDB::API &tmdbApi);

using namespace WPP;

std::map<std::string, std::pair<size_t, uint8_t*>> hostedImages;
void web(Request* req, Response* res) {
    std::string fileName = req->path.substr(1).append(req->path.substr(0,1));
    fileName.pop_back();
    if (hostedImages.size())
    {
        res->type = "image/jpeg";
        for (size_t i = 0; i < hostedImages[fileName].first; i++)
            res->body << hostedImages[fileName].second[i];
    }
}

int main(int argc, char * argv[])
{
    const std::string TMDB_API_KEY = "a6311cf3670eced44b40b55a703ef8f7";

    TMDB::API tmdbApi(TMDB_API_KEY);

    cxxopts::Options options("mkv-tagger", "Author: Dan Ferguson\n\nSummary:\n  mkv-tagger allows you to view existing EBML data and add movie and tv show metadata from theMovieDB.org to your matroska files.\n");

    options.add_options("Generic")
        ("h,help", "Displays this help menu");
    options.add_options("EBML Parser")
        ("f,file", "Matroska file to parse (manditory)", cxxopts::value<std::string>())
        ("i,info", "Display information about matroska file")
        ("search", "Search EBML elements and display all matches (case-sensitive)", cxxopts::value<std::string>())
        ("show-children", "Display nested children when searching")
        ("p,port", "Http server port number for viewing/downloading attachments", cxxopts::value<uint32_t>()->default_value("5000"));
    options.add_options("TheMovieDB.org")
        ("t,tvid", "theMovieDB.org TV Show ID", cxxopts::value<uint32_t>())
        ("s,tvseason", "theMovieDB.org TV Season #", cxxopts::value<uint32_t>())
        ("e,tvepisode", "theMovieDB.org TV Episode #", cxxopts::value<uint32_t>())
        ("m,movieid", "theMovieDB.org Movie ID", cxxopts::value<uint32_t>());

    try 
    {
        auto result = options.parse(argc, argv);
        
        if (result["help"].as<bool>())
        {
            std::cout << options.help({ "Generic", "EBML Parser", "TheMovieDB.org" });
            return 0;
        }
        else if (result["file"].count())
        {
            EBMLTools::EBMLParser ebmlParser(result["file"].as<std::string>());
            if (result["info"].count())
                return displayInfo(ebmlParser, result);
            else if (result["search"].count())
                return searchEBML(ebmlParser, result);
            else
                return FindMediaThenTag(tmdbApi, ebmlParser, result);
        }
        else
        {
            std::cerr << "A Matroska file must be specified.\n\nRun with -h or --help for more information." << std::endl;
            return 1;
        }

    }
    catch (std::exception &ex)
    {
        std::cerr << ex.what()
                  << "\n\nRun with -h or --help for more information."
                  << std::endl;
        return 1;
    }
}

// TMDB SEARCH FUNCTIONS

int searchEBML(EBMLTools::EBMLParser &ebmlParser, cxxopts::ParseResult &result)
{
    try
    {
        auto query = EBMLTools::EBMLElement::Find(result["search"].as<std::string>());
        auto fsResults = ebmlParser.FastSearch(query);
        for (auto & fsr : fsResults)
            std::cout << fsr.ToString(result["show-children"].as<bool>());
        return 0;
    }
    catch (std::invalid_argument &ex)
    {
        std::cerr << "The EBML element you searched for does not exist."
                  << "\n\nRun with -h or --help for more information."
                  << std::endl;
        return 1;
    }
}

int displayInfo(EBMLTools::EBMLParser &ebmlParser, cxxopts::ParseResult &result)
{
    
    auto info = ebmlParser.FastSearch(EBMLTools::EBMLElement::Find("Info")).at(0);
    float duration = info.Children(EBMLTools::EBMLElement::Find("Duration")).at(0).GetFloatData();
    auto dateUtc = info.Children(EBMLTools::EBMLElement::Find("DateUTC")).at(0).GetDateData();
    std::string muxingApp = info.Children(EBMLTools::EBMLElement::Find("MuxingApp")).at(0).GetStringData();
    std::string writingApp = info.Children(EBMLTools::EBMLElement::Find("WritingApp")).at(0).GetStringData();
    std::stringstream durationStr;
    if ((int)duration / 1000 / 60 / 60 > 0)
        durationStr << int(duration / 1000 / 60 / 60) << " hours ";
    if ((int)duration / 1000 / 60 > 0)
        durationStr << int((int)duration / 1000 / 60 % 60) << " minutes ";
    durationStr << int((int)duration / 1000 % 60) << " seconds";

    auto tracks = ebmlParser.FastSearch(EBMLTools::EBMLElement::Find("Tracks")).at(0);

    std::cout << "File: " << ebmlParser.GetFilename()
              << "\nInfo:"
              << "\n\t" << std::setw(18) << std::left << "Duration: " << durationStr.str()
              << "\n\t" << std::setw(18) << std::left << "Date: " << std::put_time(dateUtc, "%Y-%m-%d %H:%M:%S")
              << "\n\t" << std::setw(18) << std::left << "WritingApp: " << writingApp
              << "\n\t" << std::setw(18) << std::left << "MuxingApp: " << muxingApp
              << std::endl;
    std::cout << "\nTracks:" << std::string(20,' ') << "Total Tracks: " << tracks.Children().size();

    for (auto & trackentry : tracks.Children())
    {
        if (trackentry.GetElementId() == 0xEC)
            continue;
        int trackNum = trackentry.Children(EBMLTools::EBMLElement::Find("TrackNumber")).at(0).GetUintData();
        int trackType = trackentry.Children(EBMLTools::EBMLElement::Find("TrackType")).at(0).GetUintData();
        std::string codec = trackentry.Children(EBMLTools::EBMLElement::Find("CodecID")).at(0).GetStringData();
        auto lang = trackentry.Children(EBMLTools::EBMLElement::Find("Language"));
        auto name = trackentry.Children(EBMLTools::EBMLElement::Find("Name"));
        
        std::cout << "\n  Track Entry:"
                  << "\n\t" << std::setw(18) << "Track Number: " << trackNum
                  << "\n\t" << std::setw(18) << "Track Type: ";
        switch (trackType)
        {
            case 1:
                std::cout << "Video";
                break;
            case 2:
                std::cout << "Audio";
                break;
            case 17:
                std::cout << "Subtitle";
                break;
            default:
                std::cout << "Unknown";
                break;
        }
        std::cout << "\n\t" << std::setw(18) << "Codec: " << codec;
        if (name.size() > 0)
            std::cout << "\n\t" << std::setw(18) << "Name: " << name.at(0).GetStringData();
        if (lang.size() > 0)
            std::cout << "\n\t" << std::setw(18) << "Language: " << lang.at(0).GetStringData();
        
        auto audio = trackentry.Children(EBMLTools::EBMLElement::Find("Audio"));
        auto video = trackentry.Children(EBMLTools::EBMLElement::Find("Video"));
        if (audio.size() > 0)
        {
            float samplingFrequency = audio.at(0).Children(EBMLTools::EBMLElement::Find("SamplingFrequency")).at(0).GetFloatData();
            int channels = audio.at(0).Children(EBMLTools::EBMLElement::Find("Channels")).at(0).GetUintData();
            std::cout << "\n\t" << std::setw(18) << "Audio: "
                      << "\n\t\t" << std::setw(18) << "Frequency: " << samplingFrequency
                      << "\n\t\t" << std::setw(18) << "Channels: " << channels;
        }
        if (video.size() > 0)
        {
            int width = video.at(0).Children(EBMLTools::EBMLElement::Find("PixelWidth")).at(0).GetUintData();
            int height = video.at(0).Children(EBMLTools::EBMLElement::Find("PixelHeight")).at(0).GetUintData();
            std::cout << "\n\t" << "Video: "
                      << "\n\t\t" << std::setw(18) << "PixelWidth: " << width
                      << "\n\t\t" << std::setw(18) << "PixelHeight: " << height;
        }
    }
    std::cout << std::endl;
    auto tags = ebmlParser.FastSearch(EBMLTools::EBMLElement::Find("Tags"));
    auto attachments = ebmlParser.FastSearch(EBMLTools::EBMLElement::Find("Attachments"));
    if (tags.size() > 0)
    {
        std::cout << "\nTags:";
        for (auto &tag : tags.at(0).Children())
        {
            auto targets = tag.Children(EBMLTools::EBMLElement::Find("Targets")).at(0);
            if (targets.GetElementDataSize() == 0)
                std::cout << "\n  Tag:" << std::string(20, ' ') << "Targets: All";
            else
            {
                auto targetValue = targets.Children(EBMLTools::EBMLElement::Find("TargetTypeValue"));
                auto targetType = targets.Children(EBMLTools::EBMLElement::Find("TargetType"));
                std::cout << "\n  Tag:" << std::string(20, ' ') << "Targets: " << ((targetType.size() > 0) ? targetType.at(0).GetStringData() : "") << " (" << targetValue.at(0).GetUintData() << ")";
            }
            auto simpleTags = tag.Children(EBMLTools::EBMLElement::Find("SimpleTag"));

            for (auto & st : simpleTags)
            {
                auto children = st.Children();
                for (auto & child : children)
                {
                    if (child.GetElementName() == "TagName")
                        std::cout << "\n\t" << child.GetStringData() << ": ";
                    else if (child.GetElementName() == "TagString")
                        std::cout << child.GetStringData();
                    else if (child.GetElementName() == "SimpleTag")
                        std::cout << "\n\t  " << child.Children(EBMLTools::EBMLElement::Find("TagName")).at(0).GetStringData() << ": " << child.Children(EBMLTools::EBMLElement::Find("TagString")).at(0).GetStringData();
                }
            }
        }
    }
    if (attachments.size() > 0)
    {
        std::cout << "\nAttachments:";
        
        for (auto &attachedFile : attachments.at(0).Children())
        {
            std::cout << "\n\tAttachedFile:";
            auto children = attachedFile.Children();
            std::string fileName, mimeType;
            uint32_t fileId;
            std::pair<size_t, uint8_t*> file;
            for (auto & child : children)
            {
                if (child.GetElementName() == "FileUID")
                    fileId = child.GetUintData();
                else if (child.GetElementName() == "FileName")
                    fileName = child.GetStringData();
                else if (child.GetElementName() == "FileMimeType")
                    mimeType = child.GetStringData();
                else if (child.GetElementName() == "FileData")
                {
                    file.first = child.GetElementDataSize();
                    file.second = child.GetData();
                }
            }
            hostedImages[fileName] = file;
            std::cout << "\n\t\tFileID: " << fileId 
                      << "\n\t\tMimeType: " << mimeType
                      << "\n\t\tFileName: " << fileName
                      << "\n\t\tDownload Link: http://127.0.0.1:" << result["port"].as<uint32_t>() << "/" << fileName;
        }
        std::cout << std::endl;
        try {
            WPP::Server server;
                server.get("^.*\.[^\\$]", &web);
                server.start(result["port"].as<uint32_t>());
            } catch(WPP::Exception e) {
                std::cerr << "WebServer: " << e.what() << std::endl;
            }
    }
    std::cout << std::endl;
    return 0;
}

int FindMediaThenTag(TMDB::API &tmdbApi, EBMLTools::EBMLParser &ebmlParser, cxxopts::ParseResult &result)
{
    if (result["tvid"].count() || result["tvseason"].count() || result["tvepisode"].count())
    {
        Json::Value tv = result["tvid"].count() ? tmdbApi.TV(result["tvid"].as<uint32_t>(), "credits,keywords") : searchForTVShow(tmdbApi);
        uint32_t tvseason = result["tvseason"].count() ? result["tvseason"].as<uint32_t>() : searchForTVShowSeason(tmdbApi, tv);
        uint32_t tvepisode = result["tvepisode"].count() ? result["tvepisode"].as<uint32_t>() : searchForTVShowEpisode(tmdbApi, tv, tvseason);
        WriteTVShowTags(tv, tvseason, tvepisode, ebmlParser, tmdbApi);
        return 0;
    }

    if (result["movieid"].count())
    {
        Json::Value movie = tmdbApi.Movie(result["movieid"].as<uint32_t>(), "credits,keywords");
        WriteMovieTags(movie, ebmlParser);
        ebmlParser.CloseFile();
        return 0;
    }

    std::string choiceString = "NULL";
    while (true)
    {
        std::stringstream ss(choiceString);
        uint32_t choice;
        ss >> choice;
        switch (choice)
        {
            case 1:
            {
                Json::Value movie = searchForMovie(tmdbApi);
                WriteMovieTags(movie, ebmlParser);
                ebmlParser.CloseFile();
                return 0;
                break;
            }
            case 2:
            {
                Json::Value tv = searchForTVShow(tmdbApi);
                uint32_t tvseason = searchForTVShowSeason(tmdbApi, tv);
                uint32_t tvepisode = searchForTVShowEpisode(tmdbApi, tv, tvseason);
                WriteTVShowTags(tv, tvseason, tvepisode, ebmlParser, tmdbApi);
                return 0;
                break;
            }
            default:
                std::cout << std::string(50, '\n')
                          << "What type of media is this file?\n\n"
                          << "1. Movie\n"
                          << "2. TV Show\n\n"
                          << "Selection: ";
                std::getline(std::cin, choiceString);
                break;
        }
    }
}

Json::Value searchForMovie(TMDB::API &tmdbApi)
{
    std::string query = "";
    Json::Value results;
    while (results["results"].size() == 0 || query == "")
    {
        std::cout << std::string(50, '\n')
                  << "Search for a movie by title: ";
        std::getline(std::cin, query);
        std::cout << std::string(50, '\n') << "Searching theMovieDB.org for movie: " << query << std::string(5, '\n') << std::endl;
        results = tmdbApi.MovieSearch(query);
    }

    std::string choiceString = "NULL";
    while (true) {
        std::stringstream ss(choiceString);
        uint32_t choice;
        ss >> choice;
        switch (choice)
        {
            case 0:
                if (!ss.fail())
                    return searchForMovie(tmdbApi);
            default:
            {
                if (!ss.fail() && choice <= results["results"].size())
                {
                    std::cout << std::string(50, '\n')
                              << "Downloading movie metadata for: "
                              << results["results"][choice - 1]["title"].asString()
                              << " (" << results["results"][choice - 1]["id"].asUInt() << ")"
                              << std::endl;
                    return tmdbApi.Movie(results["results"][choice - 1]["id"].asUInt(), "credits,keywords");
                }
                
                std::cout << std::string(50, '\n')
                          << "Results from query: " << query << "\n\n"
                          << "  0. Go Back To Movie Search\n";
                size_t i = 0;
                for (auto &r : results["results"])
                    std::cout << std::setw(3) << std::right << ++i << ". "
                              << std::left << std::setw(50) << r["title"].asString()
                              << "(" << r["id"].asUInt() << ") - "
                              << r["release_date"].asString()
                              << "\n";
                std::cout << std::endl << "Selection: ";
                std::getline(std::cin, choiceString);
            }
        }
    }
}

Json::Value searchForTVShow(TMDB::API &tmdbApi)
{
    std::string query = "";
    Json::Value results;
    while (results["results"].size() == 0 || query == "")
    {
        std::cout << std::string(50, '\n')
                  << "Search for a TV Show by name: ";
        std::getline(std::cin, query);
        std::cout << std::string(50, '\n')
                  << "Searching theMovieDB.org for TV series: "
                  << query
                  << std::string(5, '\n')
                  << std::endl;
        results = tmdbApi.TVSearch(query);
    }

    std::string choiceString = "NULL";
    while (true) 
    {
        std::stringstream ss(choiceString);
        uint32_t choice;
        ss >> choice;
        if (!ss.fail() && choice == 0)
            return searchForTVShow(tmdbApi);
        if (!ss.fail() && choice <= results["results"].size()) 
        {
            std::cout << std::string(50, '\n')
                      << "Downloading TV series metadata for: "
                      << results["results"][choice - 1]["name"].asString()
                      << " (" << results["results"][choice - 1]["id"].asUInt() << ")"
                      << std::endl;
            return tmdbApi.TV(results["results"][choice - 1]["id"].asUInt(), "credits,keywords");
        }
        
        std::cout << std::string(50, '\n')
                  << "Results from query: " << query << "\n\n"
                  << "  0. Go Back To TV Show Search\n";
        size_t i = 0;
        for (auto &r : results["results"])
            std::cout << std::setw(3) << std::right << ++i << ". "
                      << std::left << std::setw(50) << r["name"].asString()
                      << "(" << r["id"].asUInt() << ") - "
                      << r["first_air_date"].asString()
                      << "\n";
        std::cout << std::endl << "Selection: ";
        std::getline(std::cin, choiceString);
    }
}

uint32_t searchForTVShowSeason(TMDB::API &tmdbApi, Json::Value &tv)
{
    std::string choiceString = "NULL";
    while (true) 
    {
        std::stringstream ss(choiceString);
        uint32_t choice;
        ss >> choice;
        if (!ss.fail() && choice < tv["seasons"].size())
            return tv["seasons"][choice]["season_number"].asUInt();
        std::cout << std::string(50, '\n')
                  << "Please select a season from " << tv["name"].asString() << "...\n\n";
        size_t i = 0;
        for (auto &r : tv["seasons"])
            std::cout << std::setw(3) << std::right << i++ << ". Season " << std::setw(2) << std::setfill('0') << r["season_number"].asUInt() << std::setfill(' ') << " - " << r["air_date"].asString() << "\n";
        std::cout << std::endl << "Selection: ";
        std::getline(std::cin, choiceString);
    }
}

uint32_t searchForTVShowEpisode(TMDB::API &tmdbApi, Json::Value &tv, uint32_t tvseason)
{
    std::string choiceString = "NULL";
    if (tv["seasons"][0]["season_number"].asUInt() != 0)
        tvseason--;
    while (true)
    {
        std::stringstream ss(choiceString);
        uint32_t choice;
        ss >> choice;
        if (!ss.fail() && choice > 0 && choice <= tv["seasons"][tvseason]["episode_count"].asUInt())
            return choice;
        std::cout << std::string(50, '\n')
                  << "Please select a episode from: " << tv["name"].asString() << " - Season " << tv["seasons"][tvseason]["season_number"].asUInt() << "\n\n";
        for (size_t i = 1; i <= tv["seasons"][tvseason]["episode_count"].asUInt(); i++)
            std::cout << std::setw(3) << std::right << i << ". Episode " << std::setw(2) << std::setfill('0') << i << std::setfill(' ') << "\n";
        std::cout << std::endl << "Selection: ";
        std::getline(std::cin, choiceString);
    }
}

// EBML TAG HELPER FUNCTIONS

std::unique_ptr<EBMLTools::EBMLWriteElement> CreateBasicSimpleTag(std::string tagName, std::string tagString)
{
    using namespace EBMLTools;
    auto SimpleTag = std::make_unique<EBMLWriteElement>(EBMLElement::Find("SimpleTag"));
    auto TagName = std::make_unique<EBMLWriteElement>(EBMLElement::Find("TagName"));
    auto TagString = std::make_unique<EBMLWriteElement>(EBMLElement::Find("TagString"));
    TagName->SetStringData(tagName);
    TagString->SetStringData(tagString);
    SimpleTag->Children().push_back(std::move(TagName));
    SimpleTag->Children().push_back(std::move(TagString));
    return SimpleTag;
}
std::unique_ptr<EBMLTools::EBMLWriteElement> CreateTag(uint32_t targetTypeValue, std::string targetType)
{
    using namespace EBMLTools;
    auto Tag = std::make_unique<EBMLWriteElement>(EBMLElement::Find("Tag"));
    auto Targets = std::make_unique<EBMLWriteElement>(EBMLElement::Find("Targets"));
    auto TargetTypeValue = std::make_unique<EBMLWriteElement>(EBMLElement::Find("TargetTypeValue"));
    auto TargetType = std::make_unique<EBMLWriteElement>(EBMLElement::Find("TargetType"));
    TargetTypeValue->SetUintData(targetTypeValue);
    TargetType->SetStringData(targetType);
    Targets->Children().push_back(std::move(TargetTypeValue));
    Targets->Children().push_back(std::move(TargetType));
    Tag->Children().push_back(std::move(Targets));
    return Tag;
}
std::vector<std::unique_ptr<EBMLTools::EBMLWriteElement>> CreateSimpleTagsFromCast(Json::Value &castCredits)
{
    std::vector<std::unique_ptr<EBMLTools::EBMLWriteElement>> results;
    for (auto &cast : castCredits)
    {
        auto stActor = CreateBasicSimpleTag("ACTOR", cast["name"].asString());

        std::stringstream str;
        str << "https://www.themoviedb.org/person/" << cast["id"].asUInt();
        auto stURL = CreateBasicSimpleTag("URL", str.str());
        stActor->Children().push_back(std::move(stURL));

        std::string delimitedCharacterList = cast["character"].asString();
        std::string delimiter = " / ";
        std::vector<std::string> characters;
        size_t start = delimitedCharacterList.find_first_not_of(delimiter), end=start;
        while (start != std::string::npos){
            end = delimitedCharacterList.find(delimiter, start);
            characters.push_back(delimitedCharacterList.substr(start, end-start));
            start = delimitedCharacterList.find_first_not_of(delimiter, end);
        }
        for (auto &character : characters)
        {
            auto stCharacter = CreateBasicSimpleTag("CHARACTER", character);
            stActor->Children().push_back(std::move(stCharacter));
        }
        results.push_back(std::move(stActor));
    }
    return results;
}
std::vector<std::unique_ptr<EBMLTools::EBMLWriteElement>> CreateSimpleTagsFromCrew(Json::Value &crewCredits)
{
    std::vector<std::unique_ptr<EBMLTools::EBMLWriteElement>> results;
    for (auto &crew : crewCredits)
    {
        const std::string job = crew["job"].asString();
        if (job == "Director")
        {
            auto stDirector = CreateBasicSimpleTag("DIRECTOR", crew["name"].asString());
            results.push_back(std::move(stDirector));
        }
        else if (job == "Co-Director" || job == "Assistant Director" || job == "First Assistant Director" || job == "Second Assistant Director" || job == "Third Assistant Director")
        {
            auto stAssistantDirector = CreateBasicSimpleTag("ASSISTANT_DIRECTOR", crew["name"].asString());
            results.push_back(std::move(stAssistantDirector));
        }
        else if (job == "Screenplay")
        {
            auto stScreenplay = CreateBasicSimpleTag("SCREENPLAY_BY", crew["name"].asString());
            results.push_back(std::move(stScreenplay));
        }
        else if (job == "Story" || job == "Writer" || job == "Co-Writer" || job == "Novel" || job == "Original Story" || job == "Book" || job == "Author" || job == "Comic Book")
        {
            auto stWriter = CreateBasicSimpleTag("WRITTEN_BY", crew["name"].asString());
            results.push_back(std::move(stWriter));
        }
        else if (job == "Editor")
        {
            auto stEditor = CreateBasicSimpleTag("EDITED_BY", crew["name"].asString());
            results.push_back(std::move(stEditor));
        }
        else if (job == "Producer")
        {
            auto stProducer = CreateBasicSimpleTag("PRODUCER", crew["name"].asString());
            results.push_back(std::move(stProducer));
        }
        else if (job == "Executive Producer")
        {
            auto stExecutiveProducer = CreateBasicSimpleTag("EXECUTIVE_PRODUCER", crew["name"].asString());
            results.push_back(std::move(stExecutiveProducer));
        }
        else if (job == "Associate Producer" || job == "Co-Producer")
        {
            auto stCoProducer = CreateBasicSimpleTag("COPRODUCER", crew["name"].asString());
            results.push_back(std::move(stCoProducer));
        }
        else if (job == "Director of Photography")
        {
            auto stDirOfPhotography = CreateBasicSimpleTag("DIRECTOR_OF_PHOTOGRAPHY", crew["name"].asString());
            results.push_back(std::move(stDirOfPhotography));
        }
        else if (job == "Art Direction" || job == "Assistant Art Director" || job == "Co-Art Director")
        {
            auto stArtDirector = CreateBasicSimpleTag("ART_DIRECTOR", crew["name"].asString());
            results.push_back(std::move(stArtDirector));
        }
        else if (job == "Costume Design" || job == "Assistant Costume Designer" || job == "Co-Costume Designer")
        {
            auto stCostumeDesigner = CreateBasicSimpleTag("COSTUME_DESIGNER", crew["name"].asString());
            results.push_back(std::move(stCostumeDesigner));
        }
        else if (job == "Production Design")
        {
            auto stProductionDesigner = CreateBasicSimpleTag("PRODUCTION_DESIGNER", crew["name"].asString());
            results.push_back(std::move(stProductionDesigner));
        }
        else if (job == "Choreographer")
        {
            auto stChoreographer = CreateBasicSimpleTag("CHOREGRAPHER", crew["name"].asString());
            results.push_back(std::move(stChoreographer));
        }
        else if (job == "Sound Engineer")
        {
            auto stSoundEngineer = CreateBasicSimpleTag("SOUND_ENGINEER", crew["name"].asString());
            results.push_back(std::move(stSoundEngineer));
        }
        else if (job == "Conductor")
        {
            auto stConductor = CreateBasicSimpleTag("CONDUCTOR", crew["name"].asString());
            results.push_back(std::move(stConductor));
        }
    }
    return results;
}
std::vector<std::unique_ptr<EBMLTools::EBMLWriteElement>> CreateSimpleTagsFromGenres(Json::Value &genres)
{
    std::vector<std::unique_ptr<EBMLTools::EBMLWriteElement>> results;
    for (auto &genre : genres)
    {
        auto stGenre = CreateBasicSimpleTag("GENRE", genre["name"].asString());
        results.push_back(std::move(stGenre));
    }
    return results;
}
std::vector<std::unique_ptr<EBMLTools::EBMLWriteElement>> CreateSimpleTagsFromProductionStudios(Json::Value &productionCompanies)
{
    std::vector<std::unique_ptr<EBMLTools::EBMLWriteElement>> results;
    for (auto &productionCompany : productionCompanies)
    {
        auto stProductionStudio = CreateBasicSimpleTag("PRODUCTION_STUDIO", productionCompany["name"].asString());
        auto stURL = CreateBasicSimpleTag("URL", "https://www.themoviedb.org/company/" + std::to_string(productionCompany["id"].asUInt()));
        stProductionStudio->Children().push_back(std::move(stURL));
        results.push_back(std::move(stProductionStudio));
    }
    return results;
}
std::unique_ptr<EBMLTools::EBMLWriteElement> CreateSimpleTagFromKeywords(Json::Value &keywords)
{
    std::stringstream str;
    size_t i = 0;
    for (auto &keyword : keywords)
    {
        if (i++ != 0)
            str << ",";
        str << keyword["name"].asString();
    }
    auto stKeywords = CreateBasicSimpleTag("KEYWORDS", str.str());
    return stKeywords;
}
std::unique_ptr<EBMLTools::EBMLWriteElement> CreateNewTagsElementForMovie(Json::Value &movie)
{
    using namespace EBMLTools;
    auto Tags = std::make_unique<EBMLWriteElement>(EBMLElement::Find("Tags"));
    // ************************  COLLECTION  ************************
    if (!movie["belongs_to_collection"].isNull())
    {
        auto collectionTag = CreateTag(70, "COLLECTION");
        auto collectionTitle = CreateBasicSimpleTag("TITLE", movie["belongs_to_collection"]["name"].asString());
        auto collectionURL = CreateBasicSimpleTag("URL", "https://www.themoviedb.org/collection/" + std::to_string(movie["belongs_to_collection"]["id"].asUInt()));
        collectionTitle->Children().push_back(std::move(collectionURL));
        collectionTag->Children().push_back(std::move(collectionTitle));
        Tags->Children().push_back(std::move(collectionTag));
    }

    // ************************  MOVIE  ************************
    auto Tag = CreateTag(50, "MOVIE");
    auto stTitle = CreateBasicSimpleTag("TITLE", movie["title"].asString());
    auto stTitleTMDBURL = CreateBasicSimpleTag("URL", "https://www.themoviedb.org/movie/" + std::to_string(movie["id"].asUInt()));
    stTitle->Children().push_back(std::move(stTitleTMDBURL));
    if (!movie["homepage"].isNull() && movie["homepage"].asString() != "")
    {
        auto stTitleURL = CreateBasicSimpleTag("URL", movie["homepage"].asString());
        stTitle->Children().push_back(std::move(stTitleURL));
    }
    Tag->Children().push_back(std::move(stTitle));
    auto stSubTitle = CreateBasicSimpleTag("SUBTITLE", movie["tagline"].asString());
    Tag->Children().push_back(std::move(stSubTitle));
    auto stDateReleased = CreateBasicSimpleTag("DATE_RELEASED", movie["release_date"].asString());
    Tag->Children().push_back(std::move(stDateReleased));
    auto stSummary = CreateBasicSimpleTag("SUMMARY", movie["overview"].asString());
    Tag->Children().push_back(std::move(stSummary));
    auto stKeywords = CreateSimpleTagFromKeywords(movie["keywords"]["keywords"]);
    Tag->Children().push_back(std::move(stKeywords));

    auto stProductionStudios = CreateSimpleTagsFromProductionStudios(movie["production_companies"]);
    std::move(stProductionStudios.begin(), stProductionStudios.end(), std::back_inserter(Tag->Children()));
    auto stGenres = CreateSimpleTagsFromGenres(movie["genres"]);
    std::move(stGenres.begin(), stGenres.end(), std::back_inserter(Tag->Children()));
    //CAST & CREW
    auto stCast = CreateSimpleTagsFromCast(movie["credits"]["cast"]);
    std::move(stCast.begin(), stCast.end(), std::back_inserter(Tag->Children()));
    auto stCrew = CreateSimpleTagsFromCrew(movie["credits"]["cast"]);
    std::move(stCrew.begin(), stCrew.end(), std::back_inserter(Tag->Children()));
    

    Tags->Children().push_back(std::move(Tag));

    Tags->Validate();
    return Tags;
}
std::unique_ptr<EBMLTools::EBMLWriteElement> CreateNewTagsElementForTVShow(TMDB::API &tmdbApi, Json::Value &tv, uint32_t season, uint32_t episode)
{
    std::cout << "Downloading additional metadata for: "
              << tv["name"].asString() << " (" << tv["id"].asUInt() << ")"
              << " - Season: " << std::setw(2) << std::setfill('0') << season
              << " - Episode: " << std::setw(2) << episode << std::setfill(' ')
              << std::endl;
    Json::Value tvseason = tmdbApi.TVSeason(tv["id"].asUInt(), season, "credits,episode/" + std::to_string(episode) + "/credits");
    
    using namespace EBMLTools;
    auto Tags = std::make_unique<EBMLWriteElement>(EBMLElement::Find("Tags"));

    // ************************  COLLECTION  ************************
    auto showTag = CreateTag(70, "COLLECTION");
    auto showTitle = CreateBasicSimpleTag("TITLE", tv["name"].asString());
    auto showTitleTMDBURL = CreateBasicSimpleTag("URL", "https://www.themoviedb.org/tv/" + std::to_string(tv["id"].asUInt()));
    showTitle->Children().push_back(std::move(showTitleTMDBURL));
    if (!tv["homepage"].isNull() && tv["homepage"].asString() != "")
    {
        auto showTitleURL = CreateBasicSimpleTag("URL", tv["homepage"].asString());
        showTitle->Children().push_back(std::move(showTitleURL));
    }
    showTag->Children().push_back(std::move(showTitle));
    auto showDateReleased = CreateBasicSimpleTag("DATE_RELEASED", tv["first_air_date"].asString());
    showTag->Children().push_back(std::move(showDateReleased));
    auto showSummary = CreateBasicSimpleTag("SUMMARY", tv["overview"].asString());
    showTag->Children().push_back(std::move(showSummary));
    auto showKeywords = CreateSimpleTagFromKeywords(tv["keywords"]["results"]);
    showTag->Children().push_back(std::move(showKeywords));
    if (tv["status"].asString() == "Ended")
    {
        auto showTotalSeasons = CreateBasicSimpleTag("TOTAL_PARTS", std::to_string(tv["number_of_seasons"].asUInt()));
        showTag->Children().push_back(std::move(showTotalSeasons));
    }
    auto showProductionStudios = CreateSimpleTagsFromProductionStudios(tv["production_companies"]);
    std::move(showProductionStudios.begin(), showProductionStudios.end(), std::back_inserter(showTag->Children()));
    auto showGenres = CreateSimpleTagsFromGenres(tv["genres"]);
    std::move(showGenres.begin(), showGenres.end(), std::back_inserter(showTag->Children()));
    //CAST & CREW
    auto showCast = CreateSimpleTagsFromCast(tv["credits"]["cast"]);
    std::move(showCast.begin(), showCast.end(), std::back_inserter(showTag->Children()));
    auto showCrew = CreateSimpleTagsFromCrew(tv["credits"]["crew"]);
    std::move(showCrew.begin(), showCrew.end(), std::back_inserter(showTag->Children()));

    // ************************  SEASON  ************************
    auto seasonTag = CreateTag(60, "SEASON");
    auto seasonTitle = CreateBasicSimpleTag("TITLE", tvseason["name"].asString());
    auto seasonTitleURL = CreateBasicSimpleTag("URL", "https://www.themoviedb.org/tv/" + std::to_string(tv["id"].asUInt()) + "/season/" + std::to_string(season));
    seasonTitle->Children().push_back(std::move(seasonTitleURL));
    seasonTag->Children().push_back(std::move(seasonTitle));
    auto seasonDateReleased = CreateBasicSimpleTag("DATE_RELEASED", tvseason["air_date"].asString());
    seasonTag->Children().push_back(std::move(seasonDateReleased));
    auto seasonSummary = CreateBasicSimpleTag("SUMMARY", tvseason["overview"].asString());
    seasonTag->Children().push_back(std::move(seasonSummary));
    auto seasonNumber = CreateBasicSimpleTag("PART_NUMBER", std::to_string(season));
    seasonTag->Children().push_back(std::move(seasonNumber));
    if (tv["status"].asString() == "Ended" || season != tv["seasons"].size() - 1)
    {
        auto seasonTotalEpisodes = CreateBasicSimpleTag("TOTAL_PARTS", std::to_string(tv["seasons"][season]["episode_count"].asUInt()));
        seasonTag->Children().push_back(std::move(seasonTotalEpisodes));
    }
    //CAST & CREW
    auto seasonCast = CreateSimpleTagsFromCast(tvseason["credits"]["cast"]);
    std::move(seasonCast.begin(), seasonCast.end(), std::back_inserter(seasonTag->Children()));
    auto seasonCrew = CreateSimpleTagsFromCrew(tvseason["credits"]["crew"]);
    std::move(seasonCrew.begin(), seasonCrew.end(), std::back_inserter(seasonTag->Children()));
    

    // ************************  EPISODE  ************************
    auto episodeTag = CreateTag(50, "EPISODE");
    auto episodeTitle = CreateBasicSimpleTag("TITLE", tvseason["episodes"][episode - 1]["name"].asString());
    auto episodeTitleURL = CreateBasicSimpleTag("URL", "https://www.themoviedb.org/tv/" + std::to_string(tv["id"].asUInt()) + "/season/" + std::to_string(season) + "/episode/" + std::to_string(episode));
    episodeTitle->Children().push_back(std::move(episodeTitleURL));
    episodeTag->Children().push_back(std::move(episodeTitle));
    auto episodeDateReleased = CreateBasicSimpleTag("DATE_RELEASED", tvseason["episodes"][episode - 1]["air_date"].asString());
    episodeTag->Children().push_back(std::move(episodeDateReleased));
    auto episodeSummary = CreateBasicSimpleTag("SUMMARY", tvseason["episodes"][episode - 1]["overview"].asString());
    episodeTag->Children().push_back(std::move(episodeSummary));
    auto episodeNumber = CreateBasicSimpleTag("PART_NUMBER", std::to_string(episode));
    episodeTag->Children().push_back(std::move(episodeNumber));
    //CAST & CREW
    auto episodeCast = CreateSimpleTagsFromCast(tvseason["episode/" + std::to_string(episode) + "/credits"]["cast"]);
    std::move(episodeCast.begin(), episodeCast.end(), std::back_inserter(episodeTag->Children()));
    auto episodeSpecialCast = CreateSimpleTagsFromCast(tvseason["episode/" + std::to_string(episode) + "/credits"]["guest_stars"]);
    std::move(episodeSpecialCast.begin(), episodeSpecialCast.end(), std::back_inserter(episodeTag->Children()));
    auto episodeCrew = CreateSimpleTagsFromCrew(tvseason["episode/" + std::to_string(episode) + "/credits"]["crew"]);
    std::move(episodeCrew.begin(), episodeCrew.end(), std::back_inserter(episodeTag->Children()));

    Tags->Children().push_back(std::move(showTag));
    Tags->Children().push_back(std::move(seasonTag));
    Tags->Children().push_back(std::move(episodeTag));

    Tags->Validate();
    return Tags;
}

// EBML ATTACHMENTS HELPER FUNCTIONS

std::unique_ptr<EBMLTools::EBMLWriteElement> CreateAttachedFile(std::vector<uint8_t> &data, std::string mimetype, uint32_t uid, std::string filename, std::string description = "")
{
    using namespace EBMLTools;
    auto AttachedFile = std::make_unique<EBMLWriteElement>(EBMLElement::Find("AttachedFile"));
    auto FileUID = std::make_unique<EBMLWriteElement>(EBMLElement::Find("FileUID"));
    FileUID->SetUintData(uid);
    auto FileData = std::make_unique<EBMLWriteElement>(EBMLElement::Find("FileData"));
    FileData->SetData(data);
    auto FileMimeType = std::make_unique<EBMLWriteElement>(EBMLElement::Find("FileMimeType"));
    FileMimeType->SetStringData(mimetype);
    auto FileName = std::make_unique<EBMLWriteElement>(EBMLElement::Find("FileName"));
    FileName->SetStringData(filename);
    AttachedFile->Children().push_back(std::move(FileUID));
    AttachedFile->Children().push_back(std::move(FileName));
    AttachedFile->Children().push_back(std::move(FileMimeType));
    if (description != "")
    {
        auto FileDescription = std::make_unique<EBMLWriteElement>(EBMLElement::Find("FileDescription"));
        FileDescription->SetStringData(description);
        AttachedFile->Children().push_back(std::move(FileDescription));
    }
    AttachedFile->Children().push_back(std::move(FileData));
    return AttachedFile;
}

std::unique_ptr<EBMLTools::EBMLWriteElement> CreateNewAttachmentsElementForMovie(Json::Value movie)
{
    using namespace EBMLTools;
    std::unique_ptr<EBMLWriteElement> Attachments = std::make_unique<EBMLWriteElement>(EBMLElement::Find("Attachments"));

    std::cout << "Downloading movie poster for: "
              << movie["title"].asString() << " (" << movie["id"].asUInt() << ")"
              << std::endl;
    auto posterLarge = TMDB::Downloader::DownloadFile("https://image.tmdb.org/t/p/h632" + movie["poster_path"].asString());
    std::cout << "Downloading movie backdrop for: "
              << movie["title"].asString() << " (" << movie["id"].asUInt() << ")"
              << std::endl;
    auto backdropLarge = TMDB::Downloader::DownloadFile("https://image.tmdb.org/t/p/w500" + movie["backdrop_path"].asString());
    auto attachedFileCover = CreateAttachedFile(posterLarge, "image/jpeg", 1, "cover.jpg");
    Attachments->Children().push_back(std::move(attachedFileCover));
    auto attachedFileBackdrop = CreateAttachedFile(backdropLarge, "image/jpeg", 2, "cover_land.jpg");
    Attachments->Children().push_back(std::move(attachedFileBackdrop));

    Attachments->Validate();
    return Attachments;
}

std::unique_ptr<EBMLTools::EBMLWriteElement> CreateNewAttachmentsElementForTVShow(Json::Value tv, uint32_t season)
{
    using namespace EBMLTools;
    std::unique_ptr<EBMLWriteElement> Attachments = std::make_unique<EBMLWriteElement>(EBMLElement::Find("Attachments"));

    if (tv["seasons"][0]["season_number"].asUInt() != 0)
        season--;

    std::cout << "Downloading season poster for: "
              << tv["name"].asString() << " (" << tv["id"].asUInt() << ") - Season " << tv["seasons"][season]["season_number"]
              << std::endl;
    auto posterLarge = TMDB::Downloader::DownloadFile("https://image.tmdb.org/t/p/h632" + tv["seasons"][season]["poster_path"].asString());
    auto attachedFileCover = CreateAttachedFile(posterLarge, "image/jpeg", 1, "cover.jpg");
    Attachments->Children().push_back(std::move(attachedFileCover));
    Attachments->Validate();
    return Attachments;
}

// TAG & ATTACHMENT WRITE FUNCTIONS

void WriteTags(EBMLTools::EBMLParser &ebmlParser, std::unique_ptr<EBMLTools::EBMLWriteElement> &Tags, std::unique_ptr<EBMLTools::EBMLWriteElement> &Attachments)
{
    auto existingTags = ebmlParser.FastSearch(EBMLTools::EBMLElement::Find("Tags"));
    auto existingAttachments = ebmlParser.FastSearch(EBMLTools::EBMLElement::Find("Attachments"));

    std::cout << "Writing Tags Element to file..."
              << std::endl;
    if (existingTags.size() > 0)
        ebmlParser.UpdateElement(existingTags[0], *Tags);
    else
        ebmlParser.AddElement(*Tags);

    std::cout << "Writing Attachments Element to file..."
              << std::endl;
    if (existingAttachments.size() > 0)
        ebmlParser.UpdateElement(existingAttachments[0], *Attachments);
    else
        ebmlParser.AddElement(*Attachments);

    std::cout << "Mastroka file has been successfully modified..."
              << std::endl;
}

void WriteMovieTags(Json::Value &movie, EBMLTools::EBMLParser &ebmlParser)
{
    auto Tags = CreateNewTagsElementForMovie(movie);
    auto Attachments = CreateNewAttachmentsElementForMovie(movie);
    WriteTags(ebmlParser, Tags, Attachments);
}

void WriteTVShowTags(Json::Value &tv, uint32_t season, uint32_t episode, EBMLTools::EBMLParser &ebmlParser, TMDB::API &tmdbApi)
{
    auto Tags = CreateNewTagsElementForTVShow(tmdbApi, tv, season, episode);
    auto Attachments = CreateNewAttachmentsElementForTVShow(tv, season);
    WriteTags(ebmlParser, Tags, Attachments);
}