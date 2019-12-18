import { async, ComponentFixture, TestBed } from '@angular/core/testing';

import { J1939FrameComponent } from './j1939-frame.component';

describe('J1939FrameComponent', () => {
  let component: J1939FrameComponent;
  let fixture: ComponentFixture<J1939FrameComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [ J1939FrameComponent ]
    })
    .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(J1939FrameComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
