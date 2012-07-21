#echo "Downloading gc-7.2.tar.gz"
#wget http://www.hpl.hp.com/personal/Hans_Boehm/gc/gc_source/gc-7.2.tar.gz
mkdir usr
cd usr
PREFIX="$(pwd)"
cd ..

echo "Downloading gmp-5.0.5.tar.bz"
wget ftp://ftp.gnu.org/gnu/gmp/gmp-5.0.5.tar.bz2
bzip2 -d gmp-5.0.5.tar.bz2
tar xf gmp-5.0.5.tar
cd gmp-5.0.5
echo "Configuring gmp-5.0.5"
./configure --prefix=$PREFIX &>/dev/null
echo "Making gmp-5.0.5"
make -j 8 &>/dev/null
echo "Installing gmp-5.0.5"
make install &>/dev/null
echo
cd ..

echo "Downloading gc-7.2.tar.gz"
wget http://www.hpl.hp.com/personal/Hans_Boehm/gc/gc_source/gc-7.2.tar.gz
tar xfz gc-7.2.tar.gz
cd gc-7.2.tar.gz
echo "Configuring gc-7.2"
./configure --prefix=$PREFIX &>/dev/null
echo "Making gc-7.2"
make -j 8 &>/dev/null
echo "Installing gc-7.2"
make install &>/dev/null
echo
cd ..

echo "Configuring Slash"
./configure --with-gc-dir=$PREFIX --with-gmp-dir=$PREFIX || exit 1
echo "Making Slash"
make sapi[cli] || exit 1
