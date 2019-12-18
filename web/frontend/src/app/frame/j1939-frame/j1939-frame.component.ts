import { Component, OnInit, ViewChild, ElementRef } from '@angular/core';

@Component({
  selector: 'app-j1939-frame',
  templateUrl: './j1939-frame.component.html',
  styleUrls: ['./j1939-frame.component.css']
})
export class J1939FrameComponent implements OnInit {
  private ws: WebSocket;
  private framelists = Array<{ name: string, pgn: string }>();

  constructor() {
    this.ws = null;
  }

  ngOnInit() {
  }

  @ViewChild('InputAddr', { static: true }) mInputAddr: ElementRef;

  add(_name: string, _pgn: string) {
    // console.log("add: " + _name + ", pgn: " + _pgn);
    this.framelists.push({ name: _name, pgn: _pgn });
  }

  processCmd(cmd: string, data) {
    if (cmd == "list frames") {
      for (const d in data)
        this.add(data[d].name, data[d].pgn);
    } else {
      console.log("unknown command: " + cmd);
    }
  }

  ConnectServer() {
    var self = this;
    let inputAddr = this.mInputAddr.nativeElement.value;

    if (inputAddr.length == 0)
      inputAddr = "127.0.0.1";

    let host = "ws://" + inputAddr + ":8000"
    console.log("Connection to " + host);

    if (this.ws == null)
      this.ws = new WebSocket(host, "j1939-protocol");

    this.ws.onopen = function (evt) {
      var cmd = { "command": "list frames" };
      var req = JSON.stringify(cmd);

      self.ws.send(req);
    };

    this.ws.onmessage = function (evt) {
      var jsonObj = JSON.parse(evt.data);
      var cmd = jsonObj.command;
      var data = jsonObj.data;

      self.processCmd(cmd, data);
    };

    this.ws.onclose = function (evt) {
      console.log("Connection closed.");
    };
  }
}
