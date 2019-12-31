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
        spn: string,
        value: string,
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
        this.spnsArray.push({ spn: "", value: "", status: "" });
    }

    DeleteSpn() {
        this.spnsArray.pop();
    }

    changeSpn(spnObj, isSpn: boolean, value: number) {
        if (isSpn)
            spnObj.spn = value;
        else
            spnObj.value = value;
    }

    Send(spn) {
        if (spn.spn.length == 0 || spn.value.length == 0) {
            alert("Spn or Value is empty");
            return;
        }

        // FIXME
        spn.status = "Success";

        // refetch the user input value
        if (this.isNumber(this.inputPriority))
            this.prio = +(this.inputPriority);

        if (this.isNumber(this.inputSource))
            this.source = +(this.inputSource);

        if (this.isNumber(this.inputDest))
            this.dest = +(this.inputDest);

        this.notify(spn);
    }
}