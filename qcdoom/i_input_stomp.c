//
// Copyright(C) 1993-1996 Id Software, Inc.
// Copyright(C) 2005-2014 Simon Howard
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// DESCRIPTION:
//	DOOM keyboard input via linux tty
//


#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <linux/keyboard.h>
#include <linux/kd.h>

#include "config.h"
#include "deh_str.h"
#include "doomtype.h"
#include "doomkeys.h"
#include "i_joystick.h"
#include "i_system.h"
#include "i_swap.h"
#include "i_timer.h"
#include "i_video.h"
#include "i_scale.h"
#include "m_argv.h"
#include "m_config.h"
#include "m_misc.h"
#include "tables.h"
#include "v_video.h"
#include "w_wad.h"
#include "z_zone.h"
#include "stomp.h"
#include "stomp_led.h"

int vanilla_keyboard_mapping = 1;
int stomps_opened = 0;
FILE *files[12];

// This keeps track of the stomp colors
static led_rgb_cfg_t color_config[12];

static const char keymap[12] = {
    KEY_LEFTARROW,
    KEY_UPARROW,
    KEY_RIGHTARROW,
    KEY_FIRE,
    KEY_ENTER,
    KEY_STRAFE_L,
    KEY_DOWNARROW,
    KEY_STRAFE_R,
    KEY_USE,
    KEY_ESCAPE,
    KEY_BACKSPACE,
    'y'
};

void I_GetEvent(void)
{
    event_t event;
    
    // put event-grabbing stuff in here
    if(stomps_opened == 0){
        printf("Waiting for stomps to be opened\n");
        return;
    }

    struct StompMessage **messages = ReadStomps(files);
    for (int i = 1; i < N_STOMPS; i++)
    {
        if (messages[i] != NULL)
        {
            // printf("============= Stomp %d =============\n", i);
            // printf("Action: %d\n", messages[i]->action);
            // printf("Value: %u\n", messages[i]->value);
            // printf("Timestamp: %u\n", messages[i]->timestamp);
            // printf("\n");

            // Use rotary encoders for looking
            // Make sure to also fire a keyup event, else the turn will never stop
            // This does not work for now. I have to find a way to trigger a ev_keyup event
            // This might be able to work by storing the last rotary event and comparing
            
            // else if(messages[i]->value == 2){
            //     event.type = ev_keydown;
            //     event.data1 = KEY_RIGHTARROW;
            //     event.data2 = 0;
            //     D_PostEvent(&event);
            // }
            // else if(messages[i]->value == 3){
            //     event.type = ev_keydown;
            //     event.data1 = KEY_LEFTARROW;
            //     event.data2 = 0;
            //     D_PostEvent(&event);
            // }

            // Determine mapped key and fire event
            event.data1 = keymap[i-1];
            event.data2 = 0;

            if(messages[i]->value == 0){
                event.type = ev_keyup;
                D_PostEvent(&event);
            }
            else if(messages[i]->value == 1){
                event.type = ev_keydown;
                D_PostEvent(&event);
            }

            LedInputEffect(&event, i-1);

            free(messages[i]);
        }
    }
}

void I_InitInput(void)
{
    printf("Initializing stomp listener\n");
    OpenFiles(files);
    OpenZencoderFd();
    InitLedsConfig();
    ApplyDefaultColors();
    stomps_opened = 1;
}

void LedInputEffect(event_t *event, int stomp_index){
    if(event->type == ev_keydown){
        SetLedAtIndexRGB(stomp_index, 0, 0, 255);
    }
    else if(event->type == ev_keyup){
        SetLedAtIndexRGB(stomp_index, color_config[stomp_index].red, color_config[stomp_index].green, color_config[stomp_index].blue);
    }
}

void ApplyDefaultColors() {
    for (int i = 0; i < N_STOMPS-1; i++) {
        SetLedAtIndexRGB(i, color_config[i].red, color_config[i].green, color_config[i].blue);
    }
}

void InitLedsConfig() {
    for (int i = 0; i < N_STOMPS-1; i++) {
        color_config[i].led_index = i+1;
        color_config[i].red = 50;
        color_config[i].green = 50;
        color_config[i].blue = 50;
    }

    // Apply a custom color scheme
    // Look left
    color_config[0].red = 25;
    color_config[0].green = 25;
    color_config[0].blue = 25;

    // Look right
    color_config[2].red = 25;
    color_config[2].green = 25;
    color_config[2].blue = 25;

    // Fire button
    color_config[3].red = 50;
    color_config[3].green = 50;
    color_config[3].blue = 0;
    
    // Enter button
    color_config[4].red = 50;
    color_config[4].green = 0;
    color_config[4].blue = 0;

    // Use button
    color_config[8].red = 0;
    color_config[8].green = 0;
    color_config[8].blue = 50;

    // Escape button
    color_config[9].red = 0;
    color_config[9].green = 10;
    color_config[9].blue = 0;

    // Backspace button
    color_config[10].red = 0;
    color_config[10].green = 10;
    color_config[10].blue = 0;
}
