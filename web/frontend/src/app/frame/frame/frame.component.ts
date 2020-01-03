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
	public period: number;

	private inputPriority;
	private inputSource;
	private inputDest;
	private inputPeriod;

	private spnsArray = Array<{
		index: number,
		spn: string,
		value: string,
		status: string
	}>();

	constructor() { }

	ngOnInit() {
	}

	notify(spn) {
		this.frameEvent.emit(spn);
	}

	isNumber(n): boolean {
		return (typeof n != 'undefined') && (!Number.isNaN(n));
	}

	showFrame() {
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
		this.spnsArray.push({ index: 0, spn: "", value: "", status: "" });
	}

	DeleteSpn() {
		this.spnsArray.pop();
	}

	ClearSpn() {
		this.spnsArray.length = 0;
	}

	changeSpn(spnObj, isSpn: boolean, value: number) {
		if (isSpn)
			spnObj.spn = value;
		else
			spnObj.value = value;
	}

	Send(spn, index: number) {
		if (spn.spn.length == 0 || spn.value.length == 0) {
			alert("Spn or Value is empty");
			return;
		}

		spn.status = "Sending";

		spn.index = index;

		// refetch the user input value
		if (this.isNumber(this.inputPriority))
			this.prio = +(this.inputPriority);

		if (this.isNumber(this.inputSource))
			this.source = +(this.inputSource);

		if (this.isNumber(this.inputDest))
			this.dest = +(this.inputDest);

		this.period = this.inputPeriod;
		if (this.period == undefined)
			this.period = 0;

		this.notify(spn);
	}

	updateStatus(index: number, reason: string) {
		console.log("updateStatus: " + index);
		this.spnsArray[index].status = reason;
	}
}