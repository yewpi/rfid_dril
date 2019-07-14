KDIR := /lib/modules/$(shell uname -r)/build
PWD  := $(shell pwd)

MODULE_NAME := mfrc522

obj-m := mfrc522.o

all:
	dtc -O dtb -o mfrc522.dtbo -b 0 -@ mfrc522-overlay.dts
	make -C "$(KDIR)" SUBDIRS="$(PWD)" modules

install:
	make -C "$(KDIR)" SUBDIRS="$(PWD)" modules_install

clean:
	make -C "$(KDIR)" SUBDIRS="$(PWD)" clean
	$(RM) *.dtbo
