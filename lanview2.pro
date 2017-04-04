TEMPLATE = subdirs

SUBDIRS = \
    lv2 \
    lv2g \
    lv2gui \
    lv2d \
    portmac \
    portstat \
    arpd \
    import \
    updt_oui

lv2.subdir = lv2

lv2g.subdir = lv2g
lv2g.depends = lv2

lv2gui.subdir = lv2gui
lv2gui.depends = lv2 lv2g

lv2d.subdirs = lv2d
lv2d.depends = lv2

portmac.subdirs = portmac
portmac.depends = lv2

portstat.subdirs = portstat
portstat.depends = lv2

arpd.subdirs = arpd
arpd.depends = lv2

import.subdirs = import
import.depends = lv2

updt_oui.subdirs = updt_oui
updt_oui.depends = lv2
