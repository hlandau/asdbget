
bin_PROGRAMS		= asdbget

bin_SCRIPTS		= asdb2tab

asdbget_SOURCES		= asdbget.c\
			  ftplib.c

noinst_HEADERS		= ftplib.h

EXTRA_DIST		= ASDB.dtd\
 			  ASDB.xsl\
			  asdb2tab

pkgdata_DATA		= ASDB.dtd\
			  ASDB.xsl

clean-local:
	rm -f *~
