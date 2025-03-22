echo Building
# We get some vague error about openssl_fips if we don't add --openssl_fips='' here. It doesn't seem to cause any issues to add it.


node ./script/js2c.js
npm install 
echo Minifying HTML and obfuscating JS
node ./applyObfuscation.js
npm run keygen --openssl_fips=''
npm run build --openssl_fips=''
npm run asar --openssl_fips=''
node ./postBuild.js

echo Deleting temporary files and folders

rm appbuild -r 
rm tmp_node_modules -r 

# node_modules.asar.unpacked just includes asarmor, which is only used in the build processs, so removing as unnecessary.
cd test 
cd resources
rm node_modules.asar.unpacked -r
cd ../../

echo Done building