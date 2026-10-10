#! /bin/sh

make -f Makefile.DJ FX_GLIDE_HW=cvg realclean
make -f Makefile.DJ FX_GLIDE_HW=h3 realclean
make -f Makefile.DJ FX_GLIDE_HW=h5 realclean
make -f Makefile.DJ FX_GLIDE_HW=sst1 realclean
make -f Makefile.DJ FX_GLIDE_HW=sst96 realclean
test -L swlibs && rm swlibs

