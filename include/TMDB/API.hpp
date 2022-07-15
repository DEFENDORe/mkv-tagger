
#include <TMDB/Downloader.hpp>
#include <jsoncpp/json/json.h>

namespace TMDB
{
    class API 
    {
        private:
            const std::string BASEURL = "https://api.themoviedb.org/3/";
            std::string APIKEY = "";
            static std::string urlEncode(const std::string &value) ;
        public:
            API(std::string APIKEY);

            Json::Value MovieSearch(std::string query);
            Json::Value TVSearch(std::string query);

            Json::Value Movie(uint32_t movie_id, std::string append_to_response = "");
            Json::Value MovieCredits(uint32_t movie_id);
            Json::Value MovieKeywords(uint32_t movie_id);
            Json::Value MovieImages(uint32_t movie_id);
            Json::Value MovieVideos(uint32_t movie_id);

            Json::Value TV(uint32_t tv_id, std::string append_to_response = "");
            Json::Value TVCredits(uint32_t tv_id);
            Json::Value TVKeywords(uint32_t tv_id);
            Json::Value TVImages(uint32_t tv_id);
            Json::Value TVVideo(uint32_t tv_id);

            Json::Value TVSeason(uint32_t tv_id, uint32_t season, std::string append_to_response = "");
            Json::Value TVSeasonCredits(uint32_t tv_id, uint32_t season);
            Json::Value TVSeasonImages(uint32_t tv_id, uint32_t season);
            Json::Value TVSeasonVideos(uint32_t tv_id, uint32_t season);

            Json::Value TVEpisode(uint32_t tv_id, uint32_t season, uint32_t episode, std::string append_to_response = "");
            Json::Value TVEpisodeCredits(uint32_t tv_id, uint32_t season, uint32_t episode);
            Json::Value TVEpisodeImages(uint32_t tv_id, uint32_t season, uint32_t episode);
            Json::Value TVEpisodeVideos(uint32_t tv_id, uint32_t season, uint32_t episode);
    };
}