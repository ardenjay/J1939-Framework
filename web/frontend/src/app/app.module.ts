import { BrowserModule } from '@angular/platform-browser';
import { NgModule } from '@angular/core';

import { AppComponent } from './app.component';
import { J1939FrameComponent } from './frame/j1939-frame/j1939-frame.component';

@NgModule({
  declarations: [
    AppComponent,
    J1939FrameComponent
  ],
  imports: [
    BrowserModule
  ],
  providers: [],
  bootstrap: [AppComponent]
})
export class AppModule { }
