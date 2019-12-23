import { Component, OnInit, Input } from '@angular/core';

@Component({
  selector: 'app-frame',
  templateUrl: './frame.component.html',
  styleUrls: ['./frame.component.css']
})
export class FrameComponent implements OnInit {
  @Input() public frameItem: FrameComponent;
  public dest: number;
  public name: string;
  public pgn: string;
  public prio: number;
  public source: number;
  public spns;

  private inputPriority;
  private inputSource;
  private inputDest;
  private inputPeriod;

  constructor() { }

  ngOnInit() {
  }

  showFrame() {
    this.name = this.frameItem.name;
    this.pgn = this.frameItem.pgn;
    this.prio = this.inputPriority = +(this.frameItem.prio);
    this.source = this.inputSource = +(this.frameItem.source);
    this.dest = this.inputDest = +(this.frameItem.dest);
  }
}