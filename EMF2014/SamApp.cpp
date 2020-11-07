/*
 TiLDA Mk2

 Task

 The MIT License (MIT)

 Copyright (c) 2014 Electromagnetic Field LTD

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
 */

#include <FreeRTOS_ARM.h>
#include <debug.h>
#include "SamApp.h"
#include "Tilda.h"
#include <glcd.h>
#include <M2tk.h>
#include "GUITask.h"
#include <fonts/allFonts.h>

extern "C" {
#include "sam.h"
#include "sam_opcodes.h"
}

// Use this as a boilerplate app class. If you create a new one, make sure to also register it in the AppManager
// otherwise it won't show up on the HomeScreen

App* SamApp::New() {
    return new SamApp;
}

SamApp::SamApp() {
    mButtonSubscription = Tilda::createButtonSubscription(UP | DOWN); // Define the buttons you're interested in here
}

SamApp::~SamApp() {
    delete mButtonSubscription;
}

String SamApp::getName() const {
        return "SamApp";
}

bool SamApp::keepAlive() const {
        return false; // Should this app be suspended or killed when another app is started?
}

// FIXME: Why does it crash on startup if 'sam_stack' is declared inside
// SamApp::task?
#define SAM_STACK_WORDS 4096
static sam_word_t sam_stack[SAM_STACK_WORDS] = {
#include "sam_program.h"
};

void SamApp::task() {
        // Do some setup in here
        Tilda::getGUITask().clearRoot(); // Clean screen

        GLCD.SetRotation(ROTATION_90);   // Orientation

        GLCD.SelectFont(System5x7);      // Font
        GLCD.CursorToXY(2, 2);           // Position cursor
        GLCD.print("Hello");             // Write text

        int ok = sam_init(&sam_stack[0], SAM_STACK_WORDS);
        sam_word_t res = sam_run();
        sam_float_t real_res = *(sam_float_t *)&res;
        GLCD.CursorToXY(2, 12);
        GLCD.print(real_res);

        Tilda::setLedColor({0, 0, 0});   // LEDs off

        while(true) {  // Make sure this loop goes on forever
            Button button = mButtonSubscription->waitForPress(1000); // Wait for up to a second for a button press
            // Make sure all the buttons you're interested in are defined in the constructor
            if (button == UP) {
                Tilda::setLedColor({255, 0, 0}); // red
            } else if (button == DOWN) {
                Tilda::setLedColor({0, 255, 0}); // green
            } else {
                Tilda::setLedColor({0, 0, 0});   // LEDs off
            }
        }
}

void SamApp::afterSuspension() {} // If keepAlive() is true this is being called when the task is suspended
void SamApp::beforeResume() {} // If keepAlive() is true this is being called when the task is resumed
