import { Component, OnInit, Input, ChangeDetectorRef, EventEmitter, Output } from '@angular/core';

@Component({
  selector: 'app-frame',
  templateUrl: './frame.component.html',
  styleUrls: ['./frame.component.css']
})
export class FrameComponent implements OnInit {
  @Input() public frameItem: FrameComponent;
  @Output() frameEvent = new EventEmitter<{ spn: number, value: number, status: string }>();

  public dest: number;
  public name: string;
  public pgn: string;
  public prio: number;
  public source: number;
  public spns;
  public interface: string;
  public updated: boolean;

  private inputPriority;
  private inputSource;
  private inputDest;
  private inputPeriod;

  private spnsArray = Array<{
    spn: number,
    value: number,
    status: string
  }>();

  constructor() { }

  ngOnChanges() {
    this.showFrame();
  }

  ngOnInit() {
  }

  notify(spn) {
    this.frameEvent.emit(spn);
  }

  isNumber(n): boolean {
    return (typeof n != 'undefined') && (!Number.isNaN(n));
  }

  showFrame() {
    if (this.frameItem.updated == false)
      return;

    this.name = this.frameItem.name;
    this.pgn = this.frameItem.pgn;
    this.interface = this.frameItem.interface;

    if (this.isNumber(this.frameItem.prio))
      this.prio = this.inputPriority = +(this.frameItem.prio);
    if (this.isNumber(this.frameItem.source))
      this.source = this.inputSource = +(this.frameItem.source);
    if (this.isNumber(this.frameItem.dest))
      this.dest = this.inputDest = +(this.frameItem.dest);
  }

  AddSpn(s) {
    this.spnsArray.push({ spn: 0, value: 0, status: "" });
  }

  DeleteSpn() {
    this.spnsArray.pop();
  }

  changeSpn(spnObj, isSpn: number, value: number) {
    if (isSpn)
      spnObj.spn = value;
    else
      spnObj.value = value;
  }

  Send(spn) {
    console.log("Send, spn: " + spn.spn + " value: " + spn.value);
    spn.status = "Success";

    this.notify(spn);
  }
}