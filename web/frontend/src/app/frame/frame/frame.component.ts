import { Component, OnInit, Input, ChangeDetectorRef } from '@angular/core';

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
  public interface: string;
  public updated: boolean;

  private inputPriority;
  private inputSource;
  private inputDest;
  private inputPeriod;

  constructor() { }

  ngOnChanges() {
    this.showFrame();
  }

  ngOnInit() {
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
}