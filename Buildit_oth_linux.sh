mkdir -p builds
rm -rf builds/bin_oth_linux
rm -rf builds/obj_oth_linux
rm -rf builds/inc_oth_linux

mkdir builds/bin_oth_linux
mkdir builds/obj_oth_linux
mkdir builds/inc_oth_linux

cp includes_com/*.h builds/inc_oth_linux/
for v in source_com/*.cpp; do g++ -pthread $v -g -O2 -Wall -Ibuilds/inc_oth_linux -c -o builds/obj_oth_linux/$(basename $v .cpp).o; done;
for v in source_com_any_linux/*.cpp; do g++ -pthread $v -g -O2 -Wall -Ibuilds/inc_oth_linux -c -o builds/obj_oth_linux/$(basename $v .cpp).o; done;
for v in source_com_oth_any/*.cpp; do g++ -pthread $v -g -O2 -Wall -Ibuilds/inc_oth_linux -c -o builds/obj_oth_linux/$(basename $v .cpp).o; done;
for v in source_proderal/*.cpp; do g++ -pthread $v -g -O2 -Wall -Ibuilds/inc_oth_linux -c -o builds/obj_oth_linux/$(basename $v .cpp).o; done;
g++ -pthread -o builds/bin_oth_linux/proderal builds/obj_oth_linux/*.o -lz -ldl

./builds/bin_oth_linux/proderal --helpdumpgui | python3 utilities/ArgGuiBuilder.py > builds/bin_oth_linux/proderalgui.py
cp GUI.py builds/bin_oth_linux/

./builds/bin_oth_linux/proderal --helpdumpgui | python3 utilities/ManPageBuilder.py proderal 'realign sequences in difficult regions' proderalmandesc.txt > builds/bin_oth_linux/proderal.1
