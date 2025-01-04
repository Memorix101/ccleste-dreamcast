# ccleste-dreamcast

<p align="center">
<img src="https://github.com/user-attachments/assets/aa154d6b-1b6f-4d66-befe-71da79d00cb6" 
        alt="celeste on dreamcast" 
        style="display: block; margin: auto" />
</p>

# About the project

A portable Celeste Classic C source port for Sega Dreamcast

https://github.com/user-attachments/assets/352f8d25-406d-4d86-8056-36c393177ab3

## Controls

- D-pad - Movement
- A Button - Jump
- B Button / X Button - Dash
- Start Button - Pause

## Recurring problems

- The sound sometimes crackles
- ~~Music does not loop and crash (possible kos or dev environment bug)~~
- ~~Music does not loop exactly~~
  
## Building

First, you need to setup and configure KallistiOS according to your operating system, you can see that [here](https://dreamcast.wiki/Getting_Started_with_Dreamcast_development)

After that, compile using these commands in the terminal

```bash
source /opt/toolchains/dc/kos/environ.sh

git clone --recursive https://github.com/GPF/SDL2
```
Now take the `SDL_dreamcastvideo.c` from the tools folder and overwrite it in `SDL2/src/videos/dreamcast/` 
```bash
cd ~/SDL2/build-scripts

./dreamcast.sh

git clone --recursive https://github.com/carstene1ns/libmikmod/tree/master

cd ~/lmikmod/dc

make

git clone --recursive https://github.com/Memorix101/ccleste-dreamcast

cd ~/ccelste-dc

chmod 755 ./make_cdi.sh

chmod 755 ./tools/mkdcdisc

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
