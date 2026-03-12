@echo off
REM
REM if FX_GLIDE_TEST_SRCIMG is not initiated, the default directory is
REM \3dfx\devel\sst1\glide\images
REM
if [%FX_GLIDE_TEST_SRCIMG%]==[] goto Default

goto Output

:Default
set FX_GLIDE_TEST_SRCIMG=%BUILD_ROOT_SST1%\glide\images

:Output
set sdir=%FX_GLIDE_TEST_SRCIMG%

@echo on
test00 -d %sdir%\test00.vgm
sleep 1
test01 -d %sdir%\test01.vgm
sleep 1
test02 -d %sdir%\test02.vgm
sleep 1
test03 -d %sdir%\test03.vgm
sleep 1
test04 -d %sdir%\test04.vgm
sleep 1
test05 -d %sdir%\test05.vgm
sleep 1
test06 -d %sdir%\test06.vgm
sleep 1
test07 -d %sdir%\test07.vgm
sleep 1
test08 -d %sdir%\test08.vgm
sleep 1
test09 -d %sdir%\test09.vgm
sleep 1
test10 -d %sdir%\test10.vgm
sleep 1
test13 -d %sdir%\test13.vgm
sleep 1
test16 -d %sdir%\test16.vgm
sleep 1
test17 -d %sdir%\test17.vgm
sleep 1
test18 -d %sdir%\test18.vgm
sleep 1
test19 -d %sdir%\test19.vgm
sleep 1
