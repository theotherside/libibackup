AUTOMAKE_OPTIONS = foreign
ACLOCAL_AMFLAGS = -I m4
SUBDIRS = src include tools docs

EXTRA_DIST = \
	docs \
	README.md

docs/html: $(top_builddir)/doxygen.cfg $(top_srcdir)/src/*.c $(top_srcdir)/src/*.h $(top_srcdir)/include/libibackup/*.h
	rm -rf docs/html
	doxygen doxygen.cfg

docs: doxygen.cfg docs/html

indent:
	indent -kr -ut -ts4 -l120 src/*.c src/*.h

