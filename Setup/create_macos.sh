rm IFStile.dmg

rm -R temp
mkdir temp
cp -R ../bin/IFStile.app ./temp/IFStile.app

create-dmg \
  --volname "IFStile Installer" \
  --background "./background.png" \
  --eula "./license.rtf" \
  --window-pos 400 100 \
  --window-size 640 480 \
  --icon-size 72 \
  --icon "IFStile.app" 220 255 \
  --hide-extension "IFStile.app" \
  --app-drop-link 410 255 \
  "IFStile.dmg" \
  "./temp"

rm -R temp