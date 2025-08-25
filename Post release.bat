@echo off
C:\Keil_v5\ARM\ARMCLANG\bin\fromelf.exe --bincombined --output "..\..\Output\Ar4 release\Ar4.bin" "..\..\Output\Ar4 release\Ar4.axf"
..\CalcBINCRC "..\..\Output\Ar4 release\Ar4.bin"
..\ConvertBIN "..\..\Output\Ar4 release\Ar4.bin" "..\..\Output\Ar4 release\Ar4.h" Ar4_Bin
