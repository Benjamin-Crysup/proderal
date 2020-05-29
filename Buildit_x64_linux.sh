rm -rf builds/bin_x64_linux
rm -rf builds/objp_x64_linux
rm -rf builds/objl_x64_linux
rm -rf builds/lib_x64_linux
rm -rf builds/inc_x64_linux

mkdir -p builds
mkdir builds/bin_x64_linux
mkdir builds/objp_x64_linux
mkdir builds/objl_x64_linux
mkdir builds/lib_x64_linux
mkdir builds/inc_x64_linux

cp includes_com/*.h builds/lib_x64_linux
cp multilang/libc/*.h builds/lib_x64_linux
for v in source_com_ml/*.gml
do
	m4 multilang/preproc/c_head.m4 $v > builds/lib_x64_linux/$(basename $v .gml).h
	m4 multilang/preproc/c_body.m4 $v > builds/objl_x64_linux/$(basename $v .gml).c
done
for v in source_com/*.cpp; do g++ -pthread  $v -O2 -Wall -Ibuilds/lib_x64_linux -c -o builds/objl_x64_linux/$(basename $v .cpp).o; done;
for v in source_com_any_linux/*.cpp; do g++ -pthread  $v -O2 -Wall -Ibuilds/lib_x64_linux -c -o builds/objl_x64_linux/$(basename $v .cpp).o; done;
for v in source_com_x64_any/*.cpp; do g++ -pthread  $v -O2 -Wall -Ibuilds/lib_x64_linux -c -o builds/objl_x64_linux/$(basename $v .cpp).o; done;
for v in builds/objl_x64_linux/*.c; do gcc -pthread  $v -O2 -w -Ibuilds/lib_x64_linux -c -o builds/objl_x64_linux/$(basename $v .c).o; done;
for v in multilang/libc/*.c; do gcc -pthread  $v -O2 -Wall -Ibuilds/lib_x64_linux -c -o builds/objl_x64_linux/$(basename $v .c).o; done;
ar rvs builds/lib_x64_linux/libwhodun.a builds/objl_x64_linux/*.o

cp includes_proderal/*.h builds/inc_x64_linux
for v in source_proderal/*.cpp; do g++ -pthread $v -O2 -Wall -Ibuilds/lib_x64_linux -Ibuilds/inc_x64_linux -c -o builds/objp_x64_linux/$(basename $v .cpp).o; done;
g++ -pthread -o builds/bin_x64_linux/proderal builds/objp_x64_linux/*.o -Lbuilds/lib_x64_linux -lwhodun -lz -ldl

cp GUI.py builds/bin_x64_linux/
mkdir builds/bin_x64_linux/example
cp -r example/ builds/bin_x64_linux/

