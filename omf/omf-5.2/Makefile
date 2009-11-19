
#
# A Makefile to build the debian packages
#

default:

debs: aggmgr expctl common resctl

aggmgr:
	cd omf-aggmgr && debuild -i -us -uc -b

expctl:
	cd omf-expctl && debuild -i -us -uc -b

common:
	cd omf-common && debuild -i -us -uc -b

resctl:
	cd omf-common && debuild -i -us -uc -b

.PHONY: clean

clean:
	rm -f *.deb *.build *.changes
