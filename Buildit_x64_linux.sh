mkdir -p builds
rm -rf builds/bin_x64_linux
rm -rf builds/obj_x64_linux
rm -rf builds/inc_x64_linux

mkdir builds/bin_x64_linux
mkdir builds/obj_x64_linux
mkdir builds/inc_x64_linux

cp includes_com/*.h builds/inc_x64_linux/
for v in source_com/*.cpp; do g++ -pthread $v -g -O0 -Wall -Ibuilds/inc_x64_linux -c -o builds/obj_x64_linux/$(basename $v .cpp).o; done;
for v in source_com_any_linux/*.cpp; do g++ -pthread $v -g -O0 -Wall -Ibuilds/inc_x64_linux -c -o builds/obj_x64_linux/$(basename $v .cpp).o; done;
for v in source_com_x64_any/*.cpp; do g++ -pthread $v -g -O0 -Wall -Ibuilds/inc_x64_linux -c -o builds/obj_x64_linux/$(basename $v .cpp).o; done;
for v in source_proderal/*.cpp; do g++ -pthread $v -g -O0 -Wall -Ibuilds/inc_x64_linux -c -o builds/obj_x64_linux/$(basename $v .cpp).o; done;
g++ -pthread -o builds/bin_x64_linux/proderal builds/obj_x64_linux/*.o -lz -ldl

./builds/bin_x64_linux/proderal --helpdumpgui | python3 utilities/ArgGuiBuilder.py > builds/bin_x64_linux/proderalgui.py
cp GUI.py builds/bin_x64_linux/

./builds/bin_x64_linux/proderal --helpdumpgui | python3 utilities/ManPageBuilder.py proderal 'realign sequences in difficult regions' proderalmandesc.txt > builds/bin_x64_linux/proderal.1
