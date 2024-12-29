# ccleste-dreamcast
A portable Celeste Classic C source port to Sega Dreamcast

![image](https://github.com/user-attachments/assets/3fa2aaf7-6051-4434-b8bd-60516e535a88)

# About the project

- Port for SEGA Dreamcast

## Controls

- D-pad - Movement
- A Button - Jump
- B Button / X Button - Dash
- Start Button - Pause

## Recurring problems

- The sound sometimes crackles
- Music does not loop and crash (possible kos or dev environment bug)

## Building

First, you need to setup and configure KallistiOS according to your operating system, you can see that [here](https://dreamcast.wiki/Getting_Started_with_Dreamcast_development)

After that, compile using these commands in the terminal

```bash
source /opt/toolchains/dc/kos/environ.sh

git clone --recursive https://github.com/GPF/SDL2

cd ~/SDL2/build-scripts

./dreamcast.sh

git clone --recursive https://github.com/carstene1ns/libmikmod/tree/master

cd ~/lmikmod/dc

make

git clone --recursive https://github.com/Memorix101/ccleste-dc

cd ~/ccelste-dc

make  

```

# Original description

This C source port of the [original Celeste](https://www.lexaloffle.com/bbs/?tid=2145) for the PICO-8 was originally developed by
[lemon32767](https://github.com/lemon32767/ccleste).
  
## Credits

Sound wave files are taken from the
[Celeste-Classic-GBA](https://github.com/JeffRuLz/Celeste-Classic-GBA/tree/master/maxmod_data)
project, music _.ogg files were obtained by converting the _.wav dumps
from PICO-8, which I did using audacity & ffmpeg.

The frame for the N-Gage version is based on [Mountain at Dusk
Background](https://opengameart.org/content/mountain-at-dusk-background)
by ansimuz.

All credit for the original game goes to the original developers (Maddy
Thorson & Noel Berry).
