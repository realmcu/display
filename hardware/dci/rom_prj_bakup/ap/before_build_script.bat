@echo off
::set keil_compiler_include_floder=%1
::set full_name_path=%2
::set linker_output_file_name=%3
::@echo on
::echo %keil_compiler_include_floder%
::echo %full_name_path%
::echo %linker_output_file_name%

REM uuid.exe        ..\..\..\kernel\staging\rtl87x3j\inc\ap\symboltable_uuid.h
REM 1>NUL del       ..\ap_lib\ROM.lib
REM 1>NUL del       ..\..\..\kernel\staging\rtl87x3j\inc\ap\rom_uuid.h
REM 1>NUL rename    ..\..\..\kernel\staging\rtl87x3j\inc\ap\symboltable_uuid.h   rom_uuid.h
REM python generate_steering.py
echo %TIME%