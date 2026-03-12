REM Insert new header here

@echo off
REM
REM if FX_GLIDE_TEST_DSTIMG is not initiated, the default directory is
REM \3dfx\devel\sst1\glide\images
REM
if [%FX_GLIDE_TEST_DSTIMG%]==[] goto Default

goto Output

:Default
set FX_GLIDE_TEST_DSTIMG=%BUILD_ROOT_SST1%\glide\images

:Output
set tdir=%FX_GLIDE_TEST_DSTIMG%

@echo on
test00 -d %tdir%\test00.tst
sleep 1
test01 -d %tdir%\test01.tst
sleep 1
test02 -d %tdir%\test02.tst
sleep 1
test03 -d %tdir%\test03.tst
sleep 1
test04 -d %tdir%\test04.tst
sleep 1
test05 -d %tdir%\test05.tst
sleep 1
test06 -d %tdir%\test06.tst
sleep 1
test07 -d %tdir%\test07.tst
sleep 1
test08 -d %tdir%\test08.tst
sleep 1
test09 -d %tdir%\test09.tst
sleep 1
test10 -d %tdir%\test10.tst
sleep 1
test13 -d %tdir%\test13.tst
sleep 1
test16 -d %tdir%\test16.tst
sleep 1
test17 -d %tdir%\test17.tst
sleep 1
test18 -d %tdir%\test18.tst
sleep 1
test19 -d %tdir%\test19.tst
sleep 1
