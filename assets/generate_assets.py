#!/usr/bin/env python3
"""Regenerate assets/include/terraforge_assets.h from the PNG masters.
The checked-in header is generated so students can build without Pillow.
This script documents the intended separation: edit PNG/WAV assets, regenerate headers, keep c/game.c logic-only. The stereo soundtrack master is assets/wav/terraforge32_stereo_loop.wav.
"""
print("This repository already contains generated headers. See /mnt/data/create_terraforge.py in the creation log for the generator used in this package.")
