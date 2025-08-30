wii-loader
================

[![Build Status](https://github.com/WiiIntosh/wii-loader/actions/workflows/main.yml/badge.svg?branch=master)](https://github.com/WiiIntosh/wii-loader/actions)

Fork of MINI for loading OpenBIOS on the Wii as part of [WiiIntosh](https://github.com/WiiIntosh).

MINI -- Mini Is Not IOS
Copyright (c) 2008, 2009 Team Twiizers

mini is our bare-bones replacement for IOS.  It does not have all of
the features that IOS provides -- notably, we have not implemented
USB, networking, write support to the NAND filesystem, DI support or
WC24 support.  However, all hardware is directly accessable via IPC --
see mini_ipc.c/h for details.

Please feel free to add write code to fill in gaps in the
functionality.  As it stands, it's useful for low-level system
recovery tasks, and as a shim/proxy to allow code on the PPC
(e.g. Linux) to have full access to the Wii's hardware.
