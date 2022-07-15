.DEFAULT_GOAL := mkvtagger
GPPPARAMS = -g -Wall -Wno-unknown-pragmas --std=c++14 -O0 -I./include 
ARPARAMS = rs

TST_DIR = test
SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin

EBMLLIBRARY = libebmlparser.a
TMDBLIBRARY = libtmdb.a

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	mkdir -p $(@D)
	g++ $(GPPPARAMS) -o $@ -c $<

EBMLSRC = $(wildcard $(SRC_DIR)/EBMLTools/*.cpp)
EBMLOBJ = $(EBMLSRC:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)
$(EBMLLIBRARY): $(BIN_DIR)/$(EBMLLIBRARY)
$(BIN_DIR)/$(EBMLLIBRARY): $(EBMLOBJ)
	mkdir -p $(@D)
	ar $(ARPARAMS) $@ $^

TMDBSRC = $(wildcard $(SRC_DIR)/TMDB/*.cpp)
TMDBOBJ = $(TMDBSRC:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)
$(TMDBLIBRARY): $(BIN_DIR)/$(TMDBLIBRARY)
$(BIN_DIR)/$(TMDBLIBRARY): $(TMDBOBJ)
	mkdir -p $(@D)
	ar $(ARPARAMS) $@ $^

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR) ./data/test.mkv

_install: mkvtagger
	sudo cp ./bin/mkvtagger /usr/bin/mkv-tagger

install: _install clean

uninstall:
	sudo rm /usr/bin/mkv-tagger

ebmltest: $(BIN_DIR)/$(EBMLLIBRARY)
	g++ $(GPPPARAMS) $(TST_DIR)/ebmltest.cpp $(BIN_DIR)/$(EBMLLIBRARY) -o $(BIN_DIR)/ebmltest
	cp ./data/test1.mkv ./data/test.mkv
	./$(BIN_DIR)/ebmltest ./data/test.mkv

tmdbtest: $(BIN_DIR)/$(TMDBLIBRARY)
	g++ $(GPPPARAMS) $(TST_DIR)/tmdbtest.cpp $(BIN_DIR)/$(TMDBLIBRARY) -o $(BIN_DIR)/tmdbtest -ljsoncpp -lcurl
	./$(BIN_DIR)/tmdbtest

mkvtagger: $(BIN_DIR)/$(TMDBLIBRARY) $(BIN_DIR)/$(EBMLLIBRARY)
	g++ $(GPPPARAMS) $(SRC_DIR)/mkvtagger.cpp $(BIN_DIR)/$(TMDBLIBRARY) $(BIN_DIR)/$(EBMLLIBRARY) -o $(BIN_DIR)/mkvtagger -ljsoncpp -lcurl