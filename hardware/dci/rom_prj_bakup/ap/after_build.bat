@echo off
set keil_compiler_include_folder=%1
set full_name_path=%2
set linker_output_file_name=%3
@echo on
::echo %keil_compiler_include_folder%
::echo %full_name_path%
::echo %linker_output_file_name%

%keil_compiler_include_folder%\..\bin\fromelf.exe --bin -o  bin\ "%full_name_path%"
%keil_compiler_include_folder%\..\bin\fromelf.exe -acd --interleave=source -o "bin\%linker_output_file_name%.disasm" "%full_name_path%"

