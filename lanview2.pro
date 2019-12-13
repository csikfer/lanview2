TEMPLATE = subdirs
CONFIG += c++14

lv2.subdir = lv2
SUBDIRS += lv2 \
    clisetup
lv2.subdirs = lv2

!exists(lv2g/miss) {
    SUBDIRS += lv2g lv2gui

    lv2g.subdir = lv2g
    lv2g.depends = lv2

    lv2gui.subdir = lv2gui
    lv2gui.depends = lv2 lv2g
}

!exists(lv2d/miss) {
    SUBDIRS += lv2d
    lv2d.subdirs = lv2d
    lv2d.depends = lv2
}

!exists(updt_oui/miss) {
    SUBDIRS += updt_oui
    updt_oui.subdirs = updt_oui
    updt_oui.depends = lv2
}

unix {

  !exists(portmac/miss) {
    SUBDIRS += portmac
    portmac.subdirs = portmac
    portmac.depends = lv2
  }

  !exists(portstat/miss) {
    SUBDIRS += portstat
    portstat.subdirs = portstat
    portstat.depends = lv2
  }

  !exists(arpd/miss) {
    SUBDIRS += arpd
    arpd.subdirs = arpd
    arpd.depends = lv2
  }

  !exists(import/miss) {
    SUBDIRS += import
    import.subdirs = import
    import.depends = lv2
  }
  !exists(portvlan/miss) {
    SUBDIRS += portvlan
    portvlan.subdirs = portvlan
    portvlan.depends = lv2
  }

  !exists(snmpvars/miss) {
    SUBDIRS += snmpvars
    snmpvars.subdirs = snmpvars
    snmpvars.depends = lv2
  }

  !exists(rrdhelper/miss) {
    SUBDIRS += rrdhelper
    lv2d.subdirs = rrdhelper
    lv2d.depends = lv2
  }

}

!exists(icontsrv/miss) {
  SUBDIRS += icontsrv
  icontsrv.subdirs = icontsrv
  icontsrv.depends = lv2
}

