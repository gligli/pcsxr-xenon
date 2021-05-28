@echo off
xenosc vs_psx.hlsl vs_psx.bin /vs

xenosc ps_psx.hlsl ps_psx.bin /ps

del ..\build\shaders.o

rem pause