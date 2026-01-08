del *.exe
del *.zip
rmdir IFStile /s /q

"C:\Program Files (x86)\Inno Setup 6\iscc.exe" IFStile.iss

mkdir IFStile
echo F|xcopy ..\bin\IFStile.exe IFStile\IFStile_x64.exe
echo F|xcopy ..\bin\IFStileARM.exe IFStile\IFStile_arm.exe
"c:\Program Files\7-Zip\7z.exe"  a -mx "IFStile.zip" "IFStile\*"
rmdir IFStile /s /q
