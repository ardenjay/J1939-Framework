extern "C" {
#include <libwebsockets.h>
}

#include <string>
#include <iostream>
#include <queue>
#include <functional>

#include <json/json.h>

#include <J1939Factory.h>
#include <GenericFrame.h>
#include <SPN/SPNNumeric.h>
#include <SPN/SPNStatus.h>
#include <SPN/SPNString.h>

#define CMD_LIST		"list frames"
#define CMD_REQ_FRAME	"req frame"

using namespace std;
using namespace J1939;

static std::queue<Json::Value> respQueue;

class EventHandler {
private:
	typedef std::function<bool(const Json::Value&, Json::Value&)> Handler;
	std::map<string, Handler> m_handler;

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
	string name = cmd["data"].asString();

	if (name.empty()) {
		cerr << __func__ << "data is null" << endl;
		return false;
	}

	reply["command"] = CMD_REQ_FRAME;
	frameToJson(name, reply);
	return true;
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
