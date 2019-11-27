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
	int retVal;
	regex_t regex;
	std::string dataRegex("^(\\s{0,1}([0-9a-fA-F][0-9a-fA-F]))+$");

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

	std::basic_string<u8> formattedData;
	std::string token;

	do {
		dataStream >> std::ws >> std::setw(2) >> token;
		formattedData.push_back(
			static_cast<u8>(std::stoul(token, nullptr, 16)));
	} while (!(dataStream.rdstate() & std::ios_base::eofbit));

	return formattedData;
}

u32 decodeID(const std::string &id)
{
	int retVal;
	regex_t regex;

	/* ID regex will match the format XXXXXXXX
	 * where X is an hexadecimal digit.
	 */
	std::string idRegex(
		"^[0-9a-fA-F][0-9a-fA-F][0-9a-fA-F]"
		"[0-9a-fA-F][0-9a-fA-F][0-9a-fA-F][0-9a-fA-F][0-9a-fA-F]$");

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

	u32 formattedId = std::stoul(id, nullptr, 16);
	return formattedId;
}

int processBAM(BamReassembler& reassembler)
{
	string id, data;
	u32 formattedId;
	basic_string<u8> formattedData;
	unique_ptr<J1939Frame> frame;

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
	return 0;
}

int process(string id, string data)
{
	BamReassembler reassembler;

	if (id.empty() || data.empty()) {
		std::cerr << "Id or data of J1939 frame not specified" << endl;
		return -EINVAL;
	}

	if (silent == false)
		std::cout << "Loaded Database: " << DATABASE_PATH << std::endl;

	if (!J1939Factory::getInstance().registerDatabaseFrames(DATABASE_PATH)) {
		std::cerr << "Database not found in " << DATABASE_PATH << std::endl;
		return -EIO;
	}

	/* get a J1939Frame */
	std::unique_ptr<J1939Frame> frame;
	u32 formattedId = decodeID(id);
	std::basic_string<u8> formattedData = decodeData(data);

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

	if (reassembler.toBeHandled(*frame)) {
		cout << "Bam frame detected... Waiting for the rest of frames to "
			"be received..." << endl;

		// First frame should be CM frame, otherwise, it is likely that we
		// remain in the loop forever
		reassembler.handleFrame(*frame);
		processBAM(reassembler);
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
	bool file_mode = false;
	std::string id, data, file;

	if (argc == 1)
		file_mode = true;

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
			file_mode = true;
			file = optarg;
			silent = true;
			break;
		}
	}

	if (file_mode == false) {
		ret = process(id, data);
		cout << "decode " << (ret ? "fail" : "ok") << endl;
	} else {
		/* could be pipe input or read file */
		std::string line;
		std::ifstream fileScript;
		/* input reference to other stream object */
		std::istream& input = file.empty() ? std::cin : fileScript;

		if (file.empty() == false) {
			fileScript.open(file);
			if (fileScript.is_open() == false) {
				cerr << "cannot open file: " << file << endl;
				return -EIO;
			}
		}

		while (std::getline(input, line))
			parseLine(line);
	}
}
