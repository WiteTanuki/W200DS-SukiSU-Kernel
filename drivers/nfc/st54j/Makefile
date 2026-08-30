#
# Makefile for ST21NFC
#
obj-$(CONFIG_NFC_ST21NFCJ) += zte_nfc.o
zte_nfc-$(CONFIG_NFC_ST21NFCJ) += st21nfc.o

obj-$(CONFIG_NFC_ST54J) += st54jese.o
st54jese-objs += st54spi.o
ccflags-$(CONFIG_NFC_DEBUG) := -DDEBUG
