#!/usr/bin/env python3

# A short python script to play once a video file when button is pressed
# meant to run on Raspberry Pi, with button connected to GPIO 21
# script meant to be used in a commercial exhibition, and button to be pressed by PMMA "coin"
# inserted to slot in arcade-styled enclosure.
# Sample video from https://peach.blender.org/download/
# Requires omxplayer!!

import RPi.GPIO as GPIO
import os
import sys
from omxplayer.player import OMXPlayer
from pathlib import Path
from time import sleep

GPIO.setmode(GPIO.BCM)

# button on GPIO 21
GPIO.setup(21, GPIO.IN, pull_up_down=GPIO.PUD_UP)
# indication LED on GPIO 13
GPIO.setup(13, GPIO.OUT)
# DEBUG button on GPIO 24
#GPIO.setup(24, GPIO.IN, pull_up_down=GPIO.PUD_UP)
    
VIDEO_PATH = Path("/home/pi/mu_code/videos/SampleVideo_1280x720_10mb.mp4")
if len(sys.argv) > 1:
    VIDEO_PATH = Path(sys.argv[1])
    
player = OMXPlayer(VIDEO_PATH, args='--no-osd --no-keys -b -o hdmi')

def playerExit(code):
    player.quit()
    print('exit',code)
    os._exit(0)

last_state1 = True
input_state1 = True
quit_video = False
timer = 0
playing = False

# set variable for length of video
videoLength = player.duration() - 0.1

# after loading pause at end of video
sleep(videoLength)
player.pause()
GPIO.output(13, 0)

while True:
    #Read states of input
    input_state1 = GPIO.input(21)

    #If GPIO(21) is shorted to Ground
    if input_state1 != last_state1:
        if (not playing and not input_state1):
            player.set_position(0)
            player.play()
            GPIO.output(13, 1)
            playing = True
    
    if playing:
        if timer > (videoLength - 0.01):
            print ("Video flie length: %f, Timer: %f" %(videoLength, timer))
            timer = 0
            player.pause()
            GPIO.output(13, 0)
            playing = False
        else:
            timer += player.position() / 1000.0
    
    player.exitEvent += lambda _, exit_code: playerExit(exit_code)
    
    #GPIO(24) to close omxplayer manually - used during debug
    if quit_video == True:
#        pygame.quit()
        player.quit()
        os.system('killall omxplayer.bin')
        playing = False
        sys.exit(0)
        os._exit(-1)
        break

    #Set last_input states
    last_state1 = input_state1