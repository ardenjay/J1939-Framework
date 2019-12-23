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

  // cmd list
  readonly CMD_LIST = "list frames";
  readonly CMD_REQ = "req frame";

  // class private member
  private framelists = Array<{ name: string, pgn: string }>();
  private frameChosen;
  private inputAddr = "";

  private frameComponent: FrameComponent;

  constructor() {
    this.ws = null;
    this.frameComponent = new FrameComponent();
  }

  ngOnInit() {
    this.target = new EventTarget;
    this.target.addEventListener(this.CMD_LIST, this.processListFrames);
    this.target.addEventListener(this.CMD_REQ, this.processReqFrame);

    // test
    // this.add("jay", "111");
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

    self.frameComponent.dest = data["dest"];
    self.frameComponent.name = data["name"];
    self.frameComponent.pgn = data["pgn"];
    self.frameComponent.prio = data["priority"];
    self.frameComponent.source = data["source"];
  }

  ConnectServer() {
    var self = this;

    if (this.inputAddr.length == 0)
      this.inputAddr = "127.0.0.1";

    let host = "ws://" + this.inputAddr + ":8000"
    console.log("Connection to " + host);

    if (this.ws == null)
      this.ws = new WebSocket(host, "j1939-protocol");

    this.ws.onopen = function (evt) {
      var cmd = { "command": self.CMD_LIST };
      var req = JSON.stringify(cmd);

      self.ws.send(req);
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

    this.ws.onclose = function (evt) {
      console.log("Connection closed.");
    };
  }

  send(cmd) {
    if (this.ws == null) {
      console.log("No websocket connection");
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
    this.target.dispatchEvent(event); */
  }
}
