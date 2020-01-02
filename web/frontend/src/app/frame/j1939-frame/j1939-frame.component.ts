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

	// control whether to show frame
	private showFrame: boolean;
	private interface: string;
	@ViewChild(FrameComponent, { static: false })
	private frameComponent: FrameComponent;

	constructor() {
		this.ws = null;
	}

	ngOnInit() {
		this.target = new EventTarget;
		this.target.addEventListener(this.CMD_LIST, this.processListFrames);
		this.target.addEventListener(this.CMD_REQ, this.processReqFrame);
		this.target.addEventListener(this.CMD_BAUD, this.processSetBaud);
		// test
		/* this.add("jay", "111"); */

		/* this.canIf.push({ name: "vcan0", value: 0});
		this.canIf.push({ name: "CAN0", value: 1}); */
	}

	add(_name: string, _pgn: string) {
		// console.log("add: " + _name + ", pgn: " + _pgn);
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

		self.frameComponent = new FrameComponent();
		self.frameComponent.dest = data["dest"];
		self.frameComponent.name = data["name"];
		self.frameComponent.pgn = data["pgn"];
		self.frameComponent.prio = data["priority"];
		self.frameComponent.source = data["source"];
		self.frameComponent.updated = true;
		if (self.interface == null)
			self.frameComponent.interface = "vcan0";  // default VCAN0
		else
			self.frameComponent.interface = self.interface;
		self.showFrame = true;
	}

	processSetBaud(event: CustomEvent) {
		var detail = event.detail;
		if (detail == null)
			return;

		console.log("processSetBaud");
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

		/* var data = {
		  "dest": 254,
		  "name": "TC1",
		  "pgn": 256,
		  "priority": 0,
		  "source": 254,
		  "spns":
			[
			  {
				"name": "Transmission Requested Gear",
				"number": 525,
				"type": 0,
				"units": "gear value",
				"value": 4294967295.0
			  }
			]
		};
	
		var self = this;
		var event = new CustomEvent(this.CMD_REQ, {
		  detail: {
			self,
			data
		  }
		});
		this.target.dispatchEvent(event);
		*/
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
		console.log("receiveEvent: spn: " + spn.spn + " value: " + spn.value);

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
				"value": +(spn.value)
			}
		};

		this.send(cmd);
	}
}