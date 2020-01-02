extern "C" {
#include <libwebsockets.h>
}

#include <string>
#include <iostream>
#include <queue>
#include <functional>

#include <json/json.h>

#include <CanEasy.h>
#include <J1939Factory.h>
#include <GenericFrame.h>
#include <SPN/SPNNumeric.h>
#include <SPN/SPNStatus.h>
#include <SPN/SPNString.h>

#define CMD_LIST			"list frames"
#define CMD_REQ_FRAME		"req frame"
#define CMD_SET_BAUD		"set baud rate"
#define CMD_CREATE_FRAME	"create frame"

using namespace std;
using namespace J1939;
using namespace Can;

static std::queue<Json::Value> respQueue;

class EventHandler {
private:
	typedef std::function<bool(const Json::Value&, Json::Value&)> Handler;
	std::map<string, Handler> m_handler;

	bool bad_handler(const Json::Value&, Json::Value&) {
		cerr << "bad_handler: the event is not registered yet" << endl;
		return false;
	}
public:
	int addListener(string event, Handler handler) {
		m_handler[event] = handler;
		return 0;
	}

	void removeListener(string event) {
		m_handler.erase(event);
	}

	bool emitEvent(string event, const Json::Value& cmd, Json::Value& reply) {
		Handler h = m_handler[event];
		if (h == NULL)
			bad_handler(cmd, reply);
		else
			return h(cmd, reply);
	}
};

static EventHandler eventHandler;

static int callback_http(struct lws *wsi, enum lws_callback_reasons reason,
                         void *user, void *in, size_t len) {
  switch (reason) {
  case LWS_CALLBACK_HTTP: {
    printf("http\n");
    break;
  }
  default:
    // printf("reason: %d\n", reason);
    break;
  }

  return 0;
}

static bool checkKey(Json::Value data, const char *str, ...)
{
	va_list ap;
	string key;

	va_start(ap, str);

	while (str) {
		if (data.isMember(str) == false) {
			cerr << __func__ << " failed: " << endl;
			cerr << key << " is not present" << endl;
			return false;
		}
		str = va_arg(ap, const char *);
	}
	va_end(ap);
	return true;
}

static Json::Value
spnToJson(const SPN *spn)
{
	Json::Value val;

	val["name"] = spn->getName();
	val["number"] = spn->getSpnNumber();
	val["type"] = spn->getType();

	switch(spn->getType()) {
	case SPN::SPN_NUMERIC: {
		const SPNNumeric *spnNum = static_cast<const SPNNumeric *>(spn);

		val["value"] = spnNum->getFormattedValue();
		val["units"] = spnNum->getUnits();
		break;
	}
	case SPN::SPN_STATUS: {
		const SPNStatus *spnStat = static_cast<const SPNStatus *>(spn);
		val["value"] = spnStat->getValue();

		SPNStatus::DescMap descriptions = spnStat->getValueDescriptionsMap();
		for(auto desc = descriptions.begin(); desc != descriptions.end(); ++desc)
			val["descriptions"][desc->first] = desc->second;
		break;
	}
	case SPN::SPN_STRING: {
		const SPNString *spnStr = static_cast<const SPNString *>(spn);
		val["value"] = spnStr->getValue();
		break;
	}
	defalut:
		break;
	}
	return val;
}

static void frameToJson(string name, Json::Value& jsonVal)
{
	Json::Value jsonSpn;
	std::unique_ptr<J1939Frame> frame;

	frame = J1939Factory::getInstance().getJ1939Frame(name);
	if (frame == nullptr)
		return;

	jsonVal["pgn"] = (u32)(frame->getPGN());
	jsonVal["name"] = frame->getName();
	jsonVal["priority"] = frame->getPriority();
	jsonVal["source"] = frame->getSrcAddr();

	if (frame->getPDUFormatGroup() == PDU_FORMAT_1)
		jsonVal["dest"] = frame->getDstAddr();

	if (frame->isGenericFrame()) {
		const GenericFrame *genFrame = (const GenericFrame *) frame.get();
		std::set<u32> spnNumbers = genFrame->getSPNNumbers();

		for (auto s : spnNumbers) {
			const SPN *spn = genFrame->getSPN(s);
			Json::Value val = spnToJson(spn);
			jsonSpn.append(val);
		}
	}
	jsonVal["spns"] = jsonSpn;
}

static bool process_cmd_list(const Json::Value& cmd, Json::Value& reply)
{
	int index = 0;
	set<u32> pgns = J1939Factory::getInstance().getAllRegisteredPGNs();

	reply["command"] = CMD_LIST;

	for (auto p : pgns) {
		unique_ptr<J1939Frame> frame = J1939Factory::getInstance().getJ1939Frame(p);
		if (frame->isGenericFrame()) {
			reply["data"][index]["name"] = frame->getName();
			reply["data"][index]["pgn"] = std::to_string(p);
			index++;
		}
	}
	return true;
}

static bool process_req_frame(const Json::Value& cmd, Json::Value& reply)
{
	u32 pgn;
	Json::Value data;

	string name = cmd["data"].asString();

	if (name.empty()) {
		cerr << __func__ << "data is null" << endl;
		return false;
	}

	reply["command"] = CMD_REQ_FRAME;
	frameToJson(name, data);
	reply["data"] = data;
	return true;
}

/* client sets the baud rate
 *
 * server does:
 * - initialized CAN interface
 * - returns initialized interfaces
 */
static bool process_set_baud(const Json::Value& cmd, Json::Value& reply)
{
	u32 rate;
	Json::Value data;

	rate = cmd["data"].asUInt();
	if (rate == 0)
		return false;

	rate *= 1000;
	cout << "process_set_baud: " << rate << endl;
	CanEasy::initialize(rate);

	set<string> interfaces = CanEasy::getCanIfaces();
	for (auto iface : interfaces)
		data.append(iface);
	
	reply["command"] = CMD_SET_BAUD;
	reply["data"] = data;
	cout << __func__ << ": " << reply << endl;
	return true;
}

static SPN* setFrameSPN(J1939Frame *frame, u32 spnNumber)
{
	GenericFrame *genFrame = static_cast<GenericFrame *>(frame);
	SPN *spn = nullptr;

	if (!genFrame->hasSPN(spnNumber)) {
		cerr << "This spn does not belong to the given frame" << endl;
		return (SPN*) nullptr;
	}
	spn = genFrame->getSPN(spnNumber);

	return spn;
}

void setFrameValue(J1939Frame *frame, SPN *spn, const std::string &value)
{
    try {
		switch (spn->getType()) {
		case SPN::SPN_NUMERIC: {
			double valueNumber = std::stod(value);
			SPNNumeric *spnNum = static_cast<SPNNumeric *>(spn);
			if (spnNum->setFormattedValue(valueNumber)) {
				cout << "Spn " << spn->getSpnNumber()
					<< " from frame " << frame->getName()
					<< " set to value "
					<< spnNum->getFormattedValue() << std::endl;
			}
			break;
		}
		case SPN::SPN_STATUS: {
			u32 valueNumber = std::stoul(value);
			u8 status = static_cast<u8>(valueNumber);
			if ((status & 0xFF) == valueNumber) {
				SPNStatus *spnStat = static_cast<SPNStatus *>(spn);
				if (!spnStat->setValue(status)) {
					cerr << "Value out of range" << endl;
				} else {
					cout << "SPN " << spn->getSpnNumber()
						<< " set to (" << valueNumber << ") "
						<< spnStat->getValueDescription(status)
						<< endl;
				}
			} else
				cerr << "Value out of range" << endl;
			break;
		}
		case SPN::SPN_STRING: {
			SPNString *spnStr = static_cast<SPNString *>(spn);
			spnStr->setValue(value);
			break;
        }
        default:
			break;
        }
	} catch (std::invalid_argument &e) {
		std::cerr << "value is not a number..." << std::endl;
	}
}

static void
encode(const J1939Frame *j1939Frame, CanFrame &frame)
{
	u32 id;
	size_t length = j1939Frame->getDataLength();
	u8 *buff = new u8[length];

	j1939Frame->encode(id, buff, length);

	frame.setId(id);

	std::string data;
	data.append((char *)buff, length);
	frame.setData(data);
	delete[] buff;
}

static void
sendFrame(const J1939Frame *j1939Frame, const string &interface, u32 period)
{
	CanFrame canFrame;
	std::shared_ptr<ICanSender> sender;

	sender = CanEasy::getSender(interface);

	encode(j1939Frame, canFrame);

	if (period == 0)
		sender->sendFrameOnce(canFrame);
	else
		sender->sendFrame(canFrame, period);
}

static bool process_create_frame(const Json::Value& cmd, Json::Value& reply)
{
	SPN* spn;
	Json::Value data, reply_data;
	u32 pgn, spnNumber, period;
	string reason, interface, value;
	std::unique_ptr<J1939Frame> j1939Frame(nullptr);

	data = cmd["data"];
	cout << data << endl;

	bool result = checkKey(data,
			"pgn",
			"spn",
			"interface",
			"value",
			"period",
			NULL);
	if (result == false) {
		reason = "checkKey failed";
		goto fail;
	}

	pgn = data["pgn"].asUInt();
	if (pgn == 0) {
		reason = "invalid pgn";
		goto fail;
	}

	spnNumber = data["spn"].asUInt();
	if (spnNumber == 0) {
		reason = "invalid spn";
		goto fail;
	}

	interface = data["interface"].asString();
	if (interface.length() == 0) {
		reason = "invalid interface";
		goto fail;
	}

	j1939Frame = J1939Factory::getInstance().getJ1939Frame(pgn);
	if (!j1939Frame.get()) {
		reason = "Frame not recognized";
		goto fail;
	}

	spn = setFrameSPN(j1939Frame.get(), spnNumber);
	if (spn == nullptr) {
		reason = "This spn does not belong to the given frame";
		goto fail;
	}

    value = data["value"].asString();
    setFrameValue(j1939Frame.get(), spn, value);

	period = data["period"].asUInt();
	sendFrame(j1939Frame.get(), interface, period);

	/* reply */
	reply["command"] = CMD_CREATE_FRAME;
	reply_data["reason"] = "Success";
	reply_data["pgn"] = pgn;
	reply_data["spn"] = spn;
	reply_data["index"] = data["index"];
	reply["data"] = reply_data;
	return true;

fail:
	reply["command"] = CMD_CREATE_FRAME;
	reply_data["reason"] = reason;
	reply_data["index"] = data["index"];
	reply["data"] = reply_data;
	return true;	// send the error back
}

static bool process_cmd(const Json::Value& cmd, Json::Value& reply)
{
	string event = cmd["command"].asString();

	cout << "process_cmd: " << event << endl;
	return eventHandler.emitEvent(event, cmd, reply);
}

static void send(struct lws *wsi, string s)
{
	unsigned char *buf;

	//cout << __func__ << ":" << endl << s << endl;

	int len = LWS_SEND_BUFFER_PRE_PADDING + s.size() +
	   		LWS_SEND_BUFFER_POST_PADDING;
	buf = new unsigned char[len];
	memcpy(buf + LWS_SEND_BUFFER_PRE_PADDING,
			s.c_str(),
			s.size());

	lws_write(wsi, buf + LWS_SEND_BUFFER_PRE_PADDING,
			s.size(), LWS_WRITE_TEXT);
	delete[] buf;
}

static bool sendQueue(struct lws *wsi)
{
	if (respQueue.empty())
		return true;

	stringstream ss;

	ss << respQueue.front();
	send(wsi, ss.str());
	respQueue.pop();

	return true;
}

static int callback_j1939(struct lws *wsi, enum lws_callback_reasons reason,
		void *user, void *in, size_t len) {
	switch (reason) {
	case LWS_CALLBACK_ESTABLISHED:
		printf("established\n");
		break;
	case LWS_CALLBACK_RECEIVE: {
		Json::CharReaderBuilder builder;
		Json::CharReader *jSonReader = builder.newCharReader();
		Json::Value cmd;
		Json::Value reply;
		string req, errs;

		req.append((char *)in, len);

		if (jSonReader->parse(req.c_str(), req.c_str() + req.size(),
					&cmd, &errs)) {
			if (cmd.isMember("command")) {

				if (process_cmd(cmd, reply))
					respQueue.push(reply);

				// request a callback
				lws_callback_on_writable(wsi);
			} else {
				lwsl_err("unknown command: %s\n", req.c_str());
			}
		} else
			lwsl_err("%s: json parse failed: %s\n",
					__func__, req.c_str());
		break;
	}
	case LWS_CALLBACK_SERVER_WRITEABLE:
		sendQueue(wsi);
		break;
	default:
		break;
	}

	return 0;
}

/* list of supported protocols and callbacks */

static struct lws_protocols protocols[] = {
    /* first protocol must always be HTTP handler */

    {
        "http-only",   /* name */
        callback_http, /* callback */
        0,              /* per_session_data_size */
    },
    {
        "j1939-protocol",
        callback_j1939, /* callback */
        1024,
    },
    {
        NULL, NULL, 0 /* End of list */
    }
};

static void registerEvent()
{
	eventHandler.addListener(CMD_LIST, process_cmd_list);
	eventHandler.addListener(CMD_REQ_FRAME, process_req_frame);
	eventHandler.addListener(CMD_SET_BAUD, process_set_baud);
	eventHandler.addListener(CMD_CREATE_FRAME, process_create_frame);
}

int main(void) {
	struct lws_context *context;
	struct lws_context_creation_info info;
	struct lws_http_mount mount;

	memset(&mount, 0, sizeof mount);
	mount.mountpoint = "/";
	mount.origin = ".";
	mount.def = "index.html";
	mount.extra_mimetypes = NULL;
	mount.origin_protocol = LWSMPRO_FILE;
	mount.mountpoint_len = 1;

	memset(&info, 0, sizeof info);
	info.port = 8000;
	info.protocols = protocols;
	info.mounts = &mount;
	info.gid = -1;
	info.uid = -1;

	lws_set_log_level(
			LLL_ERR | LLL_WARN | LLL_NOTICE | LLL_INFO /*  | LLL_DEBUG */, NULL);

	context = lws_create_context(&info);

	if (context == NULL) {
		fprintf(stderr, "libwebsocket init failed\n");
		return -1;
	}

	lwsl_info("starting server...\n");

	J1939DataBase db;
	if (!db.parseJsonFile(DATABASE_PATH)) {
		cerr << "Database not found in " << DATABASE_PATH << endl;
		return -EIO;
	}

	const vector<GenericFrame>& dbFrames = db.getParsedFrames();
	for (auto f : dbFrames)
		J1939Factory::getInstance().registerFrame(f);

	registerEvent();

	// infinite loop, the only option to end this serer is
	// by sending SIGTERM. (CTRL+C)
	while (1) {
		lws_service(context, 50);
		// libwebsocket_service will process all waiting events with their
		// callback functions and then wait 50 ms.
		// (this is single threaded webserver and this will keep
		// our server from generating load while there are not
		// requests to process)
	}

	lws_context_destroy(context);
}
