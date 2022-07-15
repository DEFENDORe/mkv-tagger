#include <TMDB/API.hpp>

#include <sstream>
#include <iomanip>

namespace TMDB
{
    API::API(std::string APIKEY)
    {
        this->APIKEY = APIKEY;
    }

    std::string API::urlEncode(const std::string &value) 
    {
        std::ostringstream escaped;
        escaped.fill('0');
        escaped << std::hex;
        for (std::string::const_iterator i = value.begin(), n = value.end(); i != n; ++i) {
            std::string::value_type c = (*i);
            if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
                escaped << c;
                continue;
            }
            escaped << std::uppercase;
            escaped << '%' << std::setw(2) << int((unsigned char) c);
            escaped << std::nouppercase;
        }
        return escaped.str();
    }

    Json::Value API::MovieSearch(std::string query)
    {
        std::stringstream str;
        str << BASEURL << "search/movie?api_key=" << APIKEY << "&query=" << urlEncode(query);
        std::string url = str.str();
        Json::Value result = Downloader::GetJSON(url);
        return result;
    }

    Json::Value API::TVSearch(std::string query)
    {
        std::stringstream str;
        str << BASEURL << "search/tv?api_key=" << APIKEY << "&query=" << urlEncode(query);
        std::string url = str.str();
        Json::Value result = Downloader::GetJSON(url);
        return result;
    }

    Json::Value API::Movie(uint32_t movie_id, std::string append_to_response)
    {
        std::stringstream str;
        str << BASEURL << "movie/" << movie_id << "?api_key=" << APIKEY;
        if (append_to_response != "")
            str << "&append_to_response=" << urlEncode(append_to_response);
        std::string url = str.str();
        Json::Value result = Downloader::GetJSON(url);
        return result;
    }

    Json::Value API::MovieKeywords(uint32_t movie_id)
    {
        std::stringstream str;
        str << BASEURL << "movie/" << movie_id << "/keywords?api_key=" << APIKEY;
        std::string url = str.str();
        Json::Value result = Downloader::GetJSON(url);
        return result;
    }

    Json::Value API::MovieImages(uint32_t movie_id)
    {
        std::stringstream str;
        str << BASEURL << "movie/" << movie_id << "/images?api_key=" << APIKEY;
        std::string url = str.str();
        Json::Value result = Downloader::GetJSON(url);
        return result;
    }

    Json::Value API::MovieVideos(uint32_t movie_id)
    {
        std::stringstream str;
        str << BASEURL << "movie/" << movie_id << "/videos?api_key=" << APIKEY;
        std::string url = str.str();
        Json::Value result = Downloader::GetJSON(url);
        return result;
    }

    Json::Value API::MovieCredits(uint32_t movie_id)
    {
        std::stringstream str;
        str << BASEURL << "movie/" << movie_id << "/credits?api_key=" << APIKEY;
        std::string url = str.str();
        Json::Value result = Downloader::GetJSON(url);
        return result;
    }

    Json::Value API::TV(uint32_t tv_id, std::string append_to_response)
    {
        std::stringstream str;
        str << BASEURL << "tv/" << tv_id << "?api_key=" << APIKEY;
        if (append_to_response != "")
            str << "&append_to_response=" << urlEncode(append_to_response);
        std::string url = str.str();
        Json::Value result = Downloader::GetJSON(url);
        return result;
    }

    Json::Value API::TVCredits(uint32_t tv_id)
    {
        std::stringstream str;
        str << BASEURL << "tv/" << tv_id << "/credits?api_key=" << APIKEY;
        std::string url = str.str();
        Json::Value result = Downloader::GetJSON(url);
        return result;
    }

    Json::Value API::TVKeywords(uint32_t tv_id)
    {
        std::stringstream str;
        str << BASEURL << "tv/" << tv_id << "/keywords?api_key=" << APIKEY;
        std::string url = str.str();
        Json::Value result = Downloader::GetJSON(url);
        return result;
    }

    Json::Value API::TVImages(uint32_t tv_id)
    {
        std::stringstream str;
        str << BASEURL << "tv/" << tv_id << "/images?api_key=" << APIKEY;
        std::string url = str.str();
        Json::Value result = Downloader::GetJSON(url);
        return result;
    }

    Json::Value API::TVVideo(uint32_t tv_id)
    {
        std::stringstream str;
        str << BASEURL << "tv/" << tv_id << "/videos?api_key=" << APIKEY;
        std::string url = str.str();
        Json::Value result = Downloader::GetJSON(url);
        return result;
    }

    Json::Value API::TVSeason(uint32_t tv_id, uint32_t season, std::string append_to_response)
    {
        std::stringstream str;
        str << BASEURL << "tv/" << tv_id << "/season/" << season << "?api_key=" << APIKEY;
        if (append_to_response != "")
            str << "&append_to_response=" << urlEncode(append_to_response);
        std::string url = str.str();
        Json::Value result = Downloader::GetJSON(url);
        return result;
    }

    Json::Value API::TVSeasonCredits(uint32_t tv_id, uint32_t season)
    {
        std::stringstream str;
        str << BASEURL << "tv/" << tv_id << "/season/" << season << "/credits?api_key=" << APIKEY;
        std::string url = str.str();
        Json::Value result = Downloader::GetJSON(url);
        return result;
    }

    Json::Value API::TVSeasonImages(uint32_t tv_id, uint32_t season)
    {
        std::stringstream str;
        str << BASEURL << "tv/" << tv_id << "/season/" << season << "/images?api_key=" << APIKEY;
        std::string url = str.str();
        Json::Value result = Downloader::GetJSON(url);
        return result;
    }

    Json::Value API::TVSeasonVideos(uint32_t tv_id, uint32_t season)
    {
        std::stringstream str;
        str << BASEURL << "tv/" << tv_id << "/season/" << season << "/videos?api_key=" << APIKEY;
        std::string url = str.str();
        Json::Value result = Downloader::GetJSON(url);
        return result;
    }

    Json::Value API::TVEpisode(uint32_t tv_id, uint32_t season, uint32_t episode, std::string append_to_response)
    {
        std::stringstream str;
        str << BASEURL << "tv/" << tv_id << "/season/" << season << "/episode/" << episode << "?api_key=" << APIKEY;
        if (append_to_response != "")
            str << "&append_to_response=" << urlEncode(append_to_response);
        std::string url = str.str();
        Json::Value result = Downloader::GetJSON(url);
        return result;
    }

    Json::Value API::TVEpisodeCredits(uint32_t tv_id, uint32_t season, uint32_t episode)
    {
        std::stringstream str;
        str << BASEURL << "tv/" << tv_id << "/season/" << season << "/episode/" << episode << "/credits?api_key=" << APIKEY;
        std::string url = str.str();
        Json::Value result = Downloader::GetJSON(url);
        return result;
    }

    Json::Value API::TVEpisodeImages(uint32_t tv_id, uint32_t season, uint32_t episode)
    {
        std::stringstream str;
        str << BASEURL << "tv/" << tv_id << "/season/" << season << "/episode/" << episode << "/images?api_key=" << APIKEY;
        std::string url = str.str();
        Json::Value result = Downloader::GetJSON(url);
        return result;
    }

    Json::Value API::TVEpisodeVideos(uint32_t tv_id, uint32_t season, uint32_t episode)
    {
        std::stringstream str;
        str << BASEURL << "tv/" << tv_id << "/season/" << season << "/episode/" << episode << "/videos?api_key=" << APIKEY;
        std::string url = str.str();
        Json::Value result = Downloader::GetJSON(url);
        return result;
    }

}