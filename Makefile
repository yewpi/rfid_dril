KDIR := /lib/modules/$(shell uname -r)/build
PWD  := $(shell pwd)

MODULE_NAME := mfrc522

obj-m := mfrc522.o

all:
	dtc -O dtb -0 mfrc522.dtbo -b 0 -@ mfrc522-overlay.dts
	make -C "$(KDIR)" SUBDIRS="$(PWD)" modules

install:
	make -C "$(KDIR)" SUBDIRS="$(PWD)" modules_install

clean:
	rm *.dtbo
	make -C "$(KDIR)" SUBDIRS="$(PWD)" clean
