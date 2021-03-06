import { FrameComponent } from './../frame/frame.component';
import { Component, OnInit, ViewChild, ElementRef } from '@angular/core';

@Component({
	selector: 'app-j1939-frame',
	templateUrl: './j1939-frame.component.html',
	styleUrls: ['./j1939-frame.component.css']
})

export class J1939FrameComponent implements OnInit {
	// components
	private ws: WebSocket;
	private target: EventTarget;

	/* cmd list */
	/* get all supported frames */
	readonly CMD_LIST = "list frames";
	/* get frame info according to selected frame */
	readonly CMD_REQ = "req frame";
	/* get baud rate */
	readonly CMD_BAUD = "set baud rate"
	readonly CMD_CREATE_FRAME = "create frame"

	// class private member
	private framelists = Array<{ name: string, pgn: string }>();
	private frameChosen;
	private inputAddr = "";
	private inputBaud = 250;

	private canIf = Array<{
		name: string,
		value: number,
	}>();

	private interface: string;
	@ViewChild(FrameComponent, { static: false })
	private frameComponent: FrameComponent = new FrameComponent();

	constructor() {
		this.ws = null;
	}

	ngOnInit() {
		this.target = new EventTarget;
		this.target.addEventListener(this.CMD_LIST, this.processListFrames);
		this.target.addEventListener(this.CMD_REQ, this.processReqFrame);
		this.target.addEventListener(this.CMD_BAUD, this.processSetBaud);
		this.target.addEventListener(this.CMD_CREATE_FRAME, this.processCreateFrame);
	}

	add(_name: string, _pgn: string) {
		this.framelists.push({ name: _name, pgn: _pgn });
	}

	processListFrames(event: CustomEvent) {
		var detail = event.detail;
		if (detail == null)
			return;

		var self = detail.self;
		var data = detail.data;
		for (const d in data)
			self.add(data[d].name, data[d].pgn);
	}

	processReqFrame(event: CustomEvent) {
		var detail = event.detail;
		if (detail == null)
			return;

		var self = detail.self;
		var data = detail.data;

		self.frameComponent.dest = data["dest"];
		self.frameComponent.name = data["name"];
		self.frameComponent.pgn = data["pgn"];
		self.frameComponent.prio = data["priority"];
		self.frameComponent.source = data["source"];
		if (self.interface == null)
			self.frameComponent.interface = "vcan0";  // default VCAN0
		else
			self.frameComponent.interface = self.interface;

		self.frameComponent.ClearSpn();
		self.frameComponent.showFrame();
	}

	processSetBaud(event: CustomEvent) {
		var detail = event.detail;
		if (detail == null)
			return;

		console.log("-> processSetBaud");
		var self = detail.self;
		var data = detail.data;

		for (const d in data) {
			var radio = {
				name: data[d],
				value: d
			}
			self.canIf.push(radio);
		}

		if (data == null)
			console.log("there is no interface");
	}

	processCreateFrame(event: CustomEvent) {
		var detail = event.detail;
		if (detail == null)
			return;

		console.log("-> processCreateFrame");
		var self = detail.self;
		var data = detail.data;
		if (data["reason"] != "Success")
			self.frameComponent.status = data["reason"];

		let index = data["index"];
		self.frameComponent.updateStatus(index, data["reason"]);
	}

	ConnectServer() {
		var self = this;

		if (this.inputAddr.length == 0)
			this.inputAddr = "127.0.0.1";

		if (this.inputBaud == 0)
			this.inputBaud = 250; // 250k

		let host = "ws://" + this.inputAddr + ":8000"
		console.log("Connection to " + host);

		if (this.ws == null)
			this.ws = new WebSocket(host, "j1939-protocol");

		this.ws.onopen = function (evt) {
			var cmd = {
				"command": self.CMD_BAUD,
				"data": self.inputBaud
			};
			self.send(cmd);
		};

		var printError = function (error, explicit) {
			console.log(`[${explicit ? 'EXPLICIT' : 'INEXPLICIT'}] ${error.name}: ${error.message}`);
		}

		this.ws.onmessage = function (evt) {
			var cmd;
			var data;

			try {
				var jsonObj = JSON.parse(evt.data);
			} catch (e) {
				if (e instanceof SyntaxError) {
					printError(e, true);
					return;
				} else {
					printError(e, false);
					return;
				}
			}

			if (jsonObj == null) {
				console.log("onmessage: jsonObj is null");
				return;
			}

			if (jsonObj.hasOwnProperty("command"))
				cmd = jsonObj.command;
			else {
				console.log("onmessage: missing command");
				return;
			}

			if (jsonObj.hasOwnProperty("data"))
				data = jsonObj.data;

			console.log("onmessage: " + cmd);

			// fire event to our handler
			var event = new CustomEvent(cmd, {
				detail: {
					self,
					data
				}
			});
			self.target.dispatchEvent(event);
		};

		this.ws.onerror = function (evt) {
			console.log("error");
			self.ws = null;
			alert("Server connection fails");
		}

		this.ws.onclose = function (evt) {
			console.log("Connection closed");
			self.ws = null;
		};
	}

	send(cmd) {
		// open: 1
		if ((this.ws == null) || (this.ws.readyState != 1)) {
			console.log("No websocket connection");
			alert("No websocket connection");
			return;
		}
		var req = JSON.stringify(cmd);
		this.ws.send(req);
	}

	chooseFrame(value) {
		console.log("chooseFrame: " + this.frameChosen);

		var cmd = {
			"command": this.CMD_REQ,
			"data": this.frameChosen
		};
		this.send(cmd);
	}

	ClearInput() {
		this.frameChosen = "";
	}

	GetFrames() {
		var cmd = { "command": this.CMD_LIST };
		this.send(cmd);
	}

	changeInterface(value) {
		console.log("changeInterface: " + value);
		this.interface = value;
	}

	receiveEvent(spn) {
		var cmd = {
			"command": this.CMD_CREATE_FRAME,
			"data": {
				"dest": this.frameComponent.dest,
				"name": this.frameComponent.name,
				"pgn": this.frameComponent.pgn,
				"priority": this.frameComponent.prio,
				"source": this.frameComponent.source,
				"interface": this.frameComponent.interface,
				"period": this.frameComponent.period,
				"spn": +(spn.spn),
				"value": +(spn.value),
				"index": +(spn.index)
			}
		};

		this.send(cmd);
	}
}