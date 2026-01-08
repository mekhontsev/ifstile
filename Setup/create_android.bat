pushd Android
call  gradlew.bat assembleRelease --warning-mode=all
call  gradlew.bat bundleRelease --warning-mode=all
popd

copy .\Android\build\outputs\apk\release\*.apk .\
copy .\Android\build\outputs\bundle\release\*.aab .\

pause

