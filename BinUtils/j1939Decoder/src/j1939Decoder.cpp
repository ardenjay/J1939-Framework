//============================================================================
// Name        : j1939Encoder.cpp
// Author      :
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <iterator>
#include <regex.h>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string.h>
#include <Types.h>
#include <memory>
#include <list>
#include <fstream>

// J1939 libraries
#include <GenericFrame.h>
#include <J1939DataBase.h>
#include <J1939Factory.h>

#include <Transport/BAM/BamReassembler.h>

#ifndef DATABASE_PATH
#define DATABASE_PATH "/etc/j1939/frames.json"
#endif

using namespace std;
using namespace J1939;

bool silent = false;

std::basic_string<u8> decodeData(const std::string &data)
{
	// At this point we have the options, we build regular expressions to
	// validate them

	// Data regex will match the format XXXXXXXXXXXXXXXX where X is an
	// hexadecimal digit.
	std::string dataRegex("^(\\s{0,1}([0-9a-fA-F][0-9a-fA-F]))+$");

	regex_t regex;
	int retVal;

	retVal = regcomp(&regex, dataRegex.c_str(), REG_EXTENDED);
	if (retVal) {
		std::cerr << "Problem compiling reg expression for data" << std::endl;
		exit(2);
	}

	retVal = regexec(&regex, data.c_str(), 0, NULL, 0);
	if (retVal == REG_NOMATCH) {
		std::cerr << "The introduced data has wrong format..." << std::endl;
		exit(3);
	} else if (retVal) {
		std::cerr << "Problem executing reg expression for ID" << std::endl;
		exit(2);
	}

	regfree(&regex);

	std::stringstream dataStream;
	dataStream << std::hex << data;

	// String where to store the data of the frame ready to be passed to the
	// frame factory with the correct format.
	std::basic_string<u8> formattedData;
	std::string token;

	do {
		dataStream >> std::ws >> std::setw(2) >> token;
		formattedData.push_back(
			static_cast<u8>(std::stoul(token, nullptr, 16)));
	} while (!(dataStream.rdstate() & std::ios_base::eofbit)); // End of file

	return formattedData;
}

u32 decodeID(const std::string &id)
{
	// ID regex will match the format XXXXXXXX where X is an hexadecimal digit.
	// The id in extended format in CAN has a length of 29 bits. It must be at
	// least 8 digits.
	std::string idRegex(
		"^[0-9a-fA-F][0-9a-fA-F][0-9a-fA-F]"
		"[0-9a-fA-F][0-9a-fA-F][0-9a-fA-F][0-9a-fA-F][0-9a-fA-F]$");

	regex_t regex;
	int retVal;

	retVal = regcomp(&regex, idRegex.c_str(), 0);
	if (retVal) {
		std::cerr << "Problem compiling reg expression for ID" << std::endl;
		exit(2);
	}

	retVal = regexec(&regex, id.c_str(), 0, NULL, 0);
	if (retVal == REG_NOMATCH) {
		std::cerr << "The introduced ID has wrong format..." << std::endl;
		exit(3);
	} else if (retVal) {
		std::cerr << "Problem executing reg expression for ID" << std::endl;
		exit(2);
	}

	regfree(&regex);

	// Convert the introduced string to a number to be interpreted by the frame
	// factory when using it.
	u32 formattedId = std::stoul(id, nullptr, 16);

	return formattedId;
}

int process(string id, string data)
{
	BamReassembler reassembler;

	if (id.empty() || data.empty()) {
		std::cerr << "Id or data of J1939 frame not specified" << endl;
		return -EINVAL;
	}

	// At this point we have the options, we build regular expressions to
	// validate them

	u32 formattedId = decodeID(id);

	std::basic_string<u8> formattedData = decodeData(data);

	if (silent == false)
		std::cout << "Loaded Database: " << DATABASE_PATH << std::endl;

	if (!J1939Factory::getInstance().registerDatabaseFrames(DATABASE_PATH)) {
		std::cerr << "Database not found in " << DATABASE_PATH << std::endl;
		return -EIO;
	}

	// The rest is easy
	std::unique_ptr<J1939Frame> frame;

	try {
		frame = J1939Factory::getInstance().getJ1939Frame(
			formattedId, formattedData.c_str(), formattedData.size());

	} catch (J1939DecodeException &e) {
		std::cerr << "Error decoding frame: " << e.getMessage() << std::endl;
		return -EIO;
	}

	if (!frame) {
		u32 pgn = ((formattedId >> J1939_PGN_OFFSET) & J1939_PGN_MASK);
		std::cerr << "Frame " << id << "(PGN:" << pgn << ")" <<
			" not identified" << std::endl;
		return -EINVAL;
	}

	// Check if it is part of the BAM protocol

	if (reassembler.toBeHandled(*frame)) {
		std::cout << "Bam frame detected... Waiting for the rest of frames to "
					 "be received..."
				  << std::endl;

		// First frame should be CM frame, otherwise, it is likely that we
		// remain in the loop forever
		reassembler.handleFrame(*frame);

		while (!reassembler.reassembledFramesPending()) {
			std::cout << "Id: " << std::endl;
			std::getline(std::cin, id);
			formattedId = decodeID(id);

			std::cout << "Data: " << std::endl;
			std::getline(std::cin, data);
			formattedData = decodeData(data);

			try {
				frame = J1939Factory::getInstance().getJ1939Frame(
					formattedId, formattedData.c_str(), formattedData.size());

				if (frame) {
					reassembler.handleFrame(*frame);
				} else {
					std::cerr << "Frame not identified" << std::endl;
					return -EINVAL;
				}

			} catch (J1939DecodeException &e) {
				std::cerr << "Error decoding frame: " << e.getMessage()
						  << std::endl;
				return -EINVAL;
			}
		}

		frame = reassembler.dequeueReassembledFrame();

		// Check again if the frame is well decoded
		if (!frame) {
			std::cerr << "Frame not identified" << std::endl;
			return -EINVAL;
		}
	}

	std::cout << frame->toString();
	return 0;
}

vector<string> split(const char *str, char c = ' ')
{
	vector<string> result;

	do
	{
		const char *begin = str;

		while (*str != c && *str)
			str++;

		/* skip the space */
		if (begin != str)
			result.push_back(string(begin, str));
	} while (0 != *str++);

	return result;
}

int parseLine(string line)
{
	int ret;

	if (line.empty()) return 0;

	vector<string> tokens = split(line.c_str());
	if (tokens.size() < 4) {
		cerr << "file format is wrong!" << endl;
		return -EINVAL;
	}

	string iface = tokens[0];
	string id = tokens[1];
	string data;

	for (int i = 3; i < tokens.size(); i++)
		data += tokens[i];

	return process(id, data);
}

int main(int argc, char **argv)
{
	int c, ret;
	std::string id, data, file;

	static struct option long_options[] = {
		{"id", required_argument, NULL, 'i'},
		{"data", required_argument, NULL, 'd'},
		{"file", required_argument, NULL, 'f'},
		{NULL, 0, NULL, 0}};

	while (1) {
		c = getopt_long(argc, argv, "i:d:f:", long_options, NULL);

		/* Detect the end of the options. */
		if (c == -1)
			break;

		switch (c) {
		case 'i':
			id = optarg;
			break;
		case 'd':
			optind--;
			for (; optind < argc && *argv[optind] != '-'; optind++)
				data += argv[optind];
			break;
		case 'f':
			file = optarg;
			silent = true;
			break;
		}
	}

	if (file.empty()) {
		ret = process(id, data);
		cout << "decode " << (ret ? "fail" : "ok") << endl;
	} else {
		std::string line;
		std::ifstream fileScript;

		fileScript.open(file);

		if (fileScript.is_open()) {
			while (std::getline(fileScript, line))
				parseLine(line);
		}
	}
}
