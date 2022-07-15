
# mkv-tagger
A CLI program to add metadata and posters fetched from theMovieDB.org to your matroska files.

### Disclaimer

Do not use this tool unless you want to play russian roulette with your matroska files.

The sole purpose of this project was to gain a better understanding of matroska files.

## Build Instructions

### Install Dependancies

```
sudo apt-get install build-essential libcurl4-openssl-dev libjsoncpp-dev
```

### Compiling mkvtagger

```
make
```

## Usage

```
./bin/mkvtagger -h                                               // Display help menu
./bin/mkvtagger -f ./data/test.mkv --search Tags --with-children // Search matroksa file for ebml element(s), display results with any child elements
./bin/mkvtagger -f ./data/test1.mkv                              // Tag matroska file; (REQUIRES INPUT) prompts user to search for movie or tv show
./bin/mkvtagger -f ./data/test1.mkv -m 24428                     // Tag mastroka file; (NO USER INPUT) Adds tags for the movie: "The Avengers"
./bin/mkvtagger -f ./data/test1.mkv -t 60059 -s 1 -e 1           // Tag mastroka file; (NO USER INPUT) Adds tags for season 1, episode 1 of the TV show "Better Call Saul"
```

# Libraries

## ebml-parser
A simple library to parse and manipulate the EBML (Extensive Binary Markup Language) within Matroska (.mkv, mka, mk3d) files. WEBM files should also be supported as it uses a subset of the matroska defined EBML elements.

See the Matroska EBML Spec [here](https://github.com/Matroska-Org/ebml-specification) and [here](https://matroska.org/technical/specs/index.html)

## tmdb-wrapper
A simple tmdb wrapper using libcurl and jsoncpp

### Running Test
Running the EBML Parser Test

```
make ebmltest
```

Running the TMDB API Test

```
make tmdbtest
```

#### Author

Copyright © 2018 Dan Ferguson
