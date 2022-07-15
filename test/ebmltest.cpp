
#include <iostream>
#include <iomanip>
#include <ctime>
#include <EBMLTools/EBMLParser.hpp>

using namespace EBMLTools;
using namespace std;

int main(int argc, char *argv[])
{
	ios_base::sync_with_stdio(false);

	std::string fileName = std::string(argv[1]);


	clock_t init_begin = std::clock();
	EBMLParser reader(fileName, true);
	clock_t init_end = clock();
	cout << "EBMLParser Constructor: "
		 << std::fixed << std::setprecision(5)
		 << double(init_end - init_begin) / CLOCKS_PER_SEC
		 << " seconds.\n" << std::endl;

	EBMLElement query = EBMLElement::Find("Tags");

	clock_t fast_begin = std::clock();
	auto test = reader.FastSearch(query);
	clock_t fast_end = clock();
	for (auto &tags : test)
		std::cout << tags.ToString(true);
	cout << "EBMLParser FastSearch:  " 
		 << double(fast_end - fast_begin) / CLOCKS_PER_SEC
		 << " seconds. ElementsFound: "
		 << test.size() << std::endl << std::endl;

	clock_t full_begin = std::clock();
	auto test2 = reader.Search(query);
	clock_t full_end = clock();
	for (auto &tags : test2)
		std::cout << tags.ToString(true);
	cout << "EBMLParser FullSearch:  " 
		 << double(full_end - full_begin) / CLOCKS_PER_SEC 
		 << " seconds. ElementsFound: " << test2.size() 
		 << std::endl << std::endl;
	
	
	// ADD A NEW SIMPLETAG TO TAG[0]->TAGS->SEGMENT
	auto simpleTag = make_unique<EBMLWriteElement>(EBMLElement::Find("SimpleTag"));
	auto tagName = make_unique<EBMLWriteElement>(EBMLElement::Find("TagName"));
	auto tagString = make_unique<EBMLWriteElement>(EBMLElement::Find("TagString"));
	tagName->SetStringData("TEST");
	tagString->SetStringData("TESTY MCTESTERSON 1234.");
	simpleTag->Children().push_back(std::move(tagName));
	simpleTag->Children().push_back(std::move(tagString));
	cout << "Created a new SimpleTag with test data." << std::endl
		 << simpleTag->ToString(true) << std::endl;

	EBMLWriteElement ele(test2.at(0));	
	ele.Children(EBMLElement::Find("Tag")).at(0)->Children().push_back(std::move(simpleTag));
	cout << "Created Tags WriteElement from existing Tags ReadElement and added our SimpleTag" << std::endl
		 << ele.ToString(true) << std::endl;

	clock_t write_begin = std::clock();
	reader.UpdateElement(test2.at(0), ele);
	//reader.AddElement(ele);
	clock_t write_end = std::clock();
	cout << "EBMLParser UpdateElement:  " 
		 << double(write_end - write_begin) / CLOCKS_PER_SEC 
		 << " seconds." << std::endl << std::endl;


	clock_t fast2_begin = std::clock();
	auto test3 = reader.FastSearch(query);
	clock_t fast2_end = clock();
	for (auto &tags : test3)
		std::cout << tags.ToString(true);
	cout << "EBMLParser FastSearch:  " 
		 << double(fast2_end - fast2_begin) / CLOCKS_PER_SEC 
		 << " seconds. ElementsFound: " 
		 << test3.size() << std::endl << std::endl;

	/*
	cout << "ENTIRE EBML STRUCTURE" << std::endl;
	auto rootElements = reader.GetRootElements();
	for (auto &ele : rootElements)
		std::cout << ele.ToString(true);
	*/

	return 0;
}