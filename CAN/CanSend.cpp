#include <iostream>
#include <fstream>
#include <getopt.h>
#include <vector>
#include <CanEasy.h>
#include <string.h>
#include <sstream>
#include <iomanip>

using namespace Can;
using namespace std;

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

int parseLine(string line, string &id, string &data)
{
	int ret;

	if (line.empty()) return 0;

	vector<string> tokens = split(line.c_str());
	if (tokens.size() < 4) {
		cerr << "file format is wrong!" << endl;
		return -EINVAL;
	}

	string iface = tokens[0];
	id = tokens[1];

	for (int i = 3; i < tokens.size(); i++)
		data += tokens[i];
	return 0;
}

void send(string _id, string _data)
{
	CanFrame canFrame;
	const int len = _data.length();
	shared_ptr<ICanSender> sender;
	set<string> ifaces;
	string data, tmp, interface;

	ifaces = CanEasy::getCanIfaces();
	interface = *(ifaces.begin());

	sender = CanEasy::getSender(interface);

	u32 id = (u32) std::stoi(_id, nullptr, 16);

	char value;
	for (int pos = 0; pos < len; pos += 2) {
		tmp = _data.substr(pos, 2);
		value = (char) std::stoul(tmp, nullptr, 16);
		data.append(&value, 1);
	}

	canFrame.setId(id);
	canFrame.setData(data);
	sender->sendFrameOnce(canFrame);
}

int main(int argc, char **argv)
{
	int c, baud_rate = 250000;
	string line, file;
	string id, data;
	ifstream fileScript;

	static struct option long_options[] = {
		{"file", required_argument, NULL, 'f'},
		{"baud", required_argument, NULL, 'b'},
		{NULL, 0, NULL, 0}};

	while (1) {
		c = getopt_long(argc, argv, "f:b:", long_options, NULL);

		if (c == -1) break;

		switch (c) {
		case 'f':
			file = optarg;
			break;
		case 'b':
			baud_rate = std::stoul(optarg, nullptr);
		default:
			break;
		}
	}

	CanEasy::initialize(baud_rate);

	/* input reference to other stream object */
	istream& input = file.empty() ? std::cin : fileScript;

	if (file.empty() == false) {
		fileScript.open(file);
		if (fileScript.is_open() == false) {
			cerr << "cannot open file: " << file << endl;
			return -EIO;
		}
	}

	while (std::getline(input, line)) {
		id.clear();
		data.clear();
		parseLine(line, id, data);
		send(id, data);
	}

	return 0;
}
