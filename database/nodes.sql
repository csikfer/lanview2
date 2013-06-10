CREATE TYPE  linktype AS ENUM ( 'ptp', 'bus', 'patch', 'logical', 'wireless', 'unknown');
COMMENT ON TYPE linktype IS
'A port typus a link alapján :
    ptp         Point to point /pecselhető
    bus         BUS (egyenlőre ugyanúgy kezeljük mint a ptp -t)
    patch       Patch port Frot<-->Back / pecselhető az előlap és a hátlap is
    logical     Nincs link / nem pecselhető
    wireless    Nincs link / nem pecselhető
    unknown     Nincs link / nem pecselhető';

CREATE TYPE portobjtype AS ENUM ('nport', 'pport', 'interface', 'iface_addr', 'unknown');
COMMENT ON TYPE portobjtype IS
'Az API-ban a rekordot reprezentáló port objektumot definiálja_
    nport,        A port objektum típusa cNPort, tábla nports (passzív port)
    pport,        A port objektum típusa cPPort, tábla pport (patch ports)
    interface,    A port objektum típusa cInterface, tábla interfaces (aktív port)
    iface_addr,   A port objektum típusa cIfaceAddr, tábla iface_addrs (aktív port +ip cím)
    unknown       Ismeretlen (az api kizárást fog dobni!)';
CREATE TABLE iftypes (
    iftype_id           serial          PRIMARY KEY,
    iftype_name         varchar(64)     NOT NULL UNIQUE,
    iftype_descr        varchar(255)    DEFAULT NULL,
    iftype_iana_id      integer         NOT NULL DEFAULT 1, -- 'other'
    iftype_link_type    linktype        NOT NULL DEFAULT 'ptp',
    iftype_obj_type     portobjtype[]   NOT NULL
);
ALTER TABLE iftypes OWNER TO lanview2;
COMMENT ON TABLE  iftypes               IS 'Network Interfaces (ports) típus leíró rekord.';
COMMENT ON COLUMN iftypes.iftype_id     IS 'Unique ID for interface''s type';
COMMENT ON COLUMN iftypes.iftype_name   IS 'Interface type''s name';
COMMENT ON COLUMN iftypes.iftype_descr  IS 'Interface type''s description';
COMMENT ON COLUMN iftypes.iftype_iana_id IS 'Protocoll Types id assigned by IANA';
COMMENT ON COLUMN iftypes.iftype_link_type IS 'A porton értelmezhető link típusa';
COMMENT ON COLUMN iftypes.iftype_obj_type IS 'A portot reprezentáló API objektum typusa. Ha két elem van, akkor a második az érvényes, ha cím is van a porthoz rendelve';

INSERT INTO iftypes
    (iftype_id, iftype_name, iftype_descr,           iftype_iana_id, iftype_link_type, iftype_obj_type) VALUES
    ( 0,        'unknown',   'Unknown pseudo type',  1,              'unknown',        '{"unknown"}');
INSERT INTO iftypes
    (iftype_name,               iftype_descr,              iftype_iana_id, iftype_link_type, iftype_obj_type) VALUES
-- Pseudo types
    ( 'attach',                 'Érzékelő kapcsolódási pont',            1,     'ptp',      '{"nport"}' ),
    ( 'bus',                    'hub port',                              1,     'bus',      '{"nport"}' ),
    ( 'sensor',                 'Sensor port',                           1,     'ptp',      '{"interface","iface_addr"}' ),
    ( 'patch',                  'Patch port (UTP)',                      1,     'patch',    '{"pport"}' ),
    ( 'rs485',                  'RS-485 Serial bus interface',           1,     'bus',      '{"interface","iface_addr"}' ),
    ( 'iic',                    'IIC Serial bus interface',              1,     'bus',      '{"interface","iface_addr"}' ),
    ( 'eport',                  'unmanagement ethernet',                 1,     'ptp',      '{"nport"}' ),

-- IANA iftype_id
    ( 'ethernet',               'Ethernet Interface (UTP)',              6,     'ptp',      '{"interface","iface_addr"}' ),
    ( 'wireless',               'Wireless ethernet interface',           6,     'wireless', '{"interface","iface_addr"}' ),
    ( 'ppp',                    'Point to Point protcol interface',     23,     'logical',  '{"iface_addr"}' ),
    ( 'softwareloopback',       'Software loopback interface',          24,     'logical',  '{"iface_addr"}' ),
    ( 'rs232',                  'RS232 Interface',                      33,     'ptp',      '{"interface"}'  ),
    ( 'virtual',                'VLan',                                 53,     'logical',  '{"interface","iface_addr"}' ),
    ( 'multiplexor',            'Trunk',                                54,     'logical',  '{"interface","iface_addr"}' ),
    ( 'adsl',                   'Asymmetric Digital Subscriber Loop',   94,     'logical',  '{"interface","iface_addr"}' ),
    ( 'tunnel',                 'Encapsulation interface',             131,     'logical',  '{"interface","iface_addr"}' ),
    ( 'digitalPowerline',       'IP over Power Lines',                 138,     'bus',      '{"interface","iface_addr"}' ),
    ( 'usb',                    'USB Interface',                       160,     'ptp',      '{"interface"}'  );
-- //// LAN.NPORTS

CREATE TYPE ifstatus AS ENUM (
    -- States for SNMP
    'up','down','testing','unknown','dormant','notpresent','lowerlayerdown',
    -- States for IndaContact
    'invert', 'short',  'broken', 'error'
);
ALTER  TYPE ifstatus OWNER TO lanview2;
COMMENT ON TYPE ifstatus IS
'Port státuszok:
SNMP port statuszok:
    up, down, testing, unknown, dormant, notpresent, lowerlayerdown
States for IndaContact
    invert  Fordított beköés jelzése, riasztási állapot.
    short   Rövidzár
    broken  Szakadt
    error   Egyébb hiba állapot

IndaContact port stáruszok kifelytése:
  Admin status                                      enum eKStat
    down    Port is disabled                            D
    up      Port is enabled                             E
    invert  Port is enabled and reverse connection      R
  Operation status                                  enum eKStat     Node status
    Up      O.K.                                        O           on
    unknown Not measured yet                            N           unknown
    dormat  Disabled                                    D           unknown
    down    Alarm                                       A           critical
    invert  Alarm, inverted                             I           critical
    short   Short                                       S           critical
    broken  Broken                                      C           critical
    unknown value should not be interpreted             U           critical
    error   Error                                       E           unknown';

CREATE TABLE nports (
    port_id     serial          PRIMARY KEY,
    port_name   varchar(32)     NOT NULL,
    port_descr  varchar(255)    DEFAULT NULL,
    port_tag    varchar(32)     DEFAULT NULL,
    iftype_id   integer         DEFAULT 0   -- Default type is 'unknown'
                        REFERENCES iftypes(iftype_id) MATCH FULL ON DELETE RESTRICT ON UPDATE RESTRICT,
    node_id     integer         NOT NULL,   -- REFERENCES (patch, hubs, nodes, hosts, snmp_devs)
    port_index  integer         DEFAULT NULL,   -- added 2011.09.05
    deleted     boolean         DEFAULT false,
    UNIQUE(node_id, port_name)
);
ALTER TABLE nports OWNER TO lanview2;
COMMENT ON TABLE  nports            IS 'Passzív portok táblája, az összes port típus őse';
COMMENT ON COLUMN nports.port_id    IS 'Egyedi port azonosító, az összes port típusra (leszármazottra) egyedi';
COMMENT ON COLUMN nports.port_name  IS 'A port neve, csak egy nodon bellül (azonos node_id) kell egyedinek lennie';
COMMENT ON COLUMN nports.port_descr IS 'Description for network port';
COMMENT ON COLUMN nports.port_tag   IS 'Opcionális cimke ill. név.';
COMMENT ON COLUMN nports.iftype_id  IS 'A típus leíró rekord azonosítója.';
COMMENT ON COLUMN nports.node_id    IS 'Csomópont azonosító, idegen kulcs a tulajdonos nodes vagy bármelyi leszármazottja rekordjára';
COMMENT ON COLUMN nports.port_index IS 'Opcionális port index. Egyes leszármazottaknál kötelező, ha meg van adva, akkor a port_name -hez hasonlóan egyedinek kell lennie.';
COMMENT ON COLUMN nports.deleted    IS 'Ha igaz, akkor a port logikailag törölve lett.';

CREATE TABLE port_params (
    port_param_id       serial          NOT NULL PRIMARY KEY,
    port_param_name     varchar(32)     NOT NULL UNIQUE,
    port_param_descr    varchar(255)    DEFAULT NULL,
    port_param_type     varchar(32)     DEFAULT NULL,
    port_param_dim      varchar(32)     DEFAULT NULL
);
ALTER TABLE port_params OWNER TO lanview2;
COMMENT ON TABLE port_params IS 'Port extra paraméterek deklarálása (név, típus, dimenzió) értékek a port_param_values -ben';
COMMENT ON COLUMN port_params.port_param_id IS 'A port paraméter leíró egyedi azonosítója.';
COMMENT ON COLUMN port_params.port_param_name IS 'A paraméter neve.';
COMMENT ON COLUMN port_params.port_param_descr IS 'A paraméterhez egy magyarázó szöveg';
COMMENT ON COLUMN port_params.port_param_type IS 'Egy opcionális típus azonosító';
COMMENT ON COLUMN port_params.port_param_dim IS 'Egy opcionális dimenzió';

CREATE TABLE port_param_values (
    port_param_value_id serial          PRIMARY KEY,
    port_param_id       integer         NOT NULL
            REFERENCES port_params(port_param_id) MATCH FULL ON DELETE RESTRICT ON UPDATE RESTRICT,
    port_id             integer         NOT NULL,   -- REFERENCES nports(port_id) kivéve pports
    param_value         varchar(255)    DEFAULT NULL,
    UNIQUE (port_param_id, port_id)
);
ALTER TABLE port_param_values OWNER TO lanview2;
COMMENT ON TABLE  port_param_values IS 'Port extra paraméter értékek.';
COMMENT ON COLUMN port_param_values.port_param_value_id IS 'A paraméter érték egyedi azonosítója.';
COMMENT ON COLUMN port_param_values.port_param_id IS 'A paraméter tulajdonságait definiáló port_params rekord azonosítója.';
COMMENT ON COLUMN port_param_values.port_id IS 'A tulajdonos port rekordjának az azonosítója.';
COMMENT ON COLUMN port_param_values.param_value IS 'A parméter érték.';

CREATE TABLE patchs (
    node_id     serial          PRIMARY KEY,    -- Sequence: patchs_node_id_seq
    node_name   varchar(32)     NOT NULL UNIQUE,
    node_descr  varchar(255)    DEFAULT NULL,
    place_id    integer         DEFAULT 0   -- place = 'unknown'
                REFERENCES places(place_id) MATCH FULL ON DELETE SET DEFAULT ON UPDATE RESTRICT
);
ALTER TABLE patchs OWNER TO lanview2;
COMMENT ON TABLE  patchs            IS 'Patch panel/csatlakozók/kapcsolódási pont tábla';
COMMENT ON COLUMN patchs.node_id    IS 'Unique ID for node. Az összes leszármazottra és az ősre is egyedi.';
COMMENT ON COLUMN patchs.node_name  IS 'Unique Name of the node. Az összes leszármazottra és az ősre is egyedi.';
COMMENT ON COLUMN patchs.node_descr IS 'Descrition of the node';
COMMENT ON COLUMN patchs.place_id   IS 'Az eszköz helyét azonosító "opcionális" távoli kulcs. Alapértelmezett hely a ''unknown''.';

CREATE TABLE hubs (
    is_virtual  boolean NOT NULL DEFAULT TRUE,
    is_switch   boolean NOT NULL DEFAULT FALSE,
    PRIMARY KEY (node_id),
    UNIQUE (node_name)
)INHERITS (patchs);
COMMENT ON TABLE  hubs            IS 'Virtual or physical HUBs, unmanaged switchs';
COMMENT ON COLUMN hubs.is_virtual IS 'Ha az eszköz virtuális, akkor értéke igaz.';
COMMENT ON COLUMN hubs.is_switch IS  'Ha az eszköz egy switchs, akkor értéke igaz.';

-- //// LAN.PPORT S  Pach Ports
CREATE TYPE portshare AS ENUM ('',      -- 1,2,3,4,5,6,7,8  / nincs megosztás
                               'A',     -- 1,2,3,6
                               'AA',    -- 1,2
                               'AB',    -- 3.6
                               'B',     -- 4,5,7,8
                               'BA',    -- 4,5
                               'BB',    -- 7,8
                               'C',     -- 3,4
                               'D');    -- 5,6
COMMENT ON TYPE portshare IS
'Port megosztás típusok:
    ''''    1,2,3,4,5,6,7,8  / nincs megosztás
    ''A''   1,2,3,6
    ''AA''  1,2
    ''AB''  3.6
    ''B''   4,5,7,8
    ''BA''  4,5
    ''BB''  7,8
    ''C''   3,4
    ''D''   5,6';
-- A megengedett, egymással nem ütköző konkurens SHARE kombinációk
CREATE OR REPLACE FUNCTION check_shared(portshare, portshare) RETURNS boolean AS $$
BEGIN
    IF $1 = '' OR $2 = '' THEN
        RETURN FALSE;
    END IF;
    RETURN ( $1 = 'A'  AND ($2 = 'B' OR $2 = 'BA' OR $2 = 'BB'))
        OR ( $1 = 'AA' AND ($2 = 'B' OR $2 = 'BA' OR $2 = 'BB' OR $2 = 'AB' OR $2 = 'C' OR $2 = 'D'))
        OR ( $1 = 'AB' AND ($2 = 'B' OR $2 = 'BA' OR $2 = 'BB' OR $2 = 'AA'))
        OR ( $1 = 'B'  AND ($2 = 'A' OR $2 = 'AA' OR $2 = 'AB'))
        OR ( $1 = 'BA' AND ($2 = 'A' OR $2 = 'AA' OR $2 = 'AB' OR $2 = 'BB'))
        OR ( $1 = 'BB' AND ($2 = 'A' OR $2 = 'AA' OR $2 = 'AB' OR $2 = 'BA' OR $2 = 'C' OR $2 = 'D'))
        OR ( $1 = 'C'  AND ($2 = 'D' OR $2 = 'AA' OR $2 = 'BB'))
        OR ( $1 = 'D'  AND ($2 = 'C' OR $2 = 'AA' OR $2 = 'BB'));
END;
$$ LANGUAGE plpgsql;
COMMENT ON FUNCTION check_shared(portshare, portshare) IS
'Megvizsgálja, hogy a megadott két megosztás típus átfedi-e egymást,ha nem akkor igazzal tér vissza.';

-- Legkisebb SHARE meghatározása, ha a két share kombináció nem ad semmilyen összeköttetést, akkor NULL
CREATE OR REPLACE FUNCTION min_shared(portshare, portshare) RETURNS portshare AS $$
BEGIN
    IF $1 > $2 THEN
        RETURN min_shared($2, $1);
    END IF;
    IF ( $1 = $2 ) OR ( $1 = '' )
    OR ( $1 = 'A' AND ( $2 = 'AA' OR $2 = 'AB' ) )
    OR ( $1 = 'B' AND ( $2 = 'BA' OR $2 = 'BB' ) ) THEN
        RAISE INFO 'min_shared(%,%) = %', $1,$2,$2;
        RETURN $2;
    END IF;
    -- RAISE INFO 'min_shared(%,%) = NULL', $1,$2;
    RETURN NULL;
END;
$$ LANGUAGE plpgsql;
COMMENT ON FUNCTION min_shared(portshare, portshare) IS 'Legkisebb SHARE meghatározása, ha a két share kombináció nem ad semmilyen összeköttetést, akkor NULL';

-- Egy portshare tömbnek azon elemeivel tér vissza, melyek nem ütköznek a második paraméterrel
CREATE OR REPLACE FUNCTION shares_filt(shares portshare[], sh portshare) RETURNS portshare[] AS $$
DECLARE
    oshs    portshare[];
    i       integer;
BEGIN
    FOR i IN 0 .. array_length(shares, 1) LOOP
        IF check_shared(shares[i], sh) THEN
            oshs := oshs || shares[i];
        END IF;
    END LOOP;
    RETURN oshs;
END;
$$ LANGUAGE plpgsql;
COMMENT ON FUNCTION shares_filt(portshare[], portshare) IS 'Egy portshare tömbnek azon elemeivel tér vissza, melyek nem ütköznek a második paraméterrel';

CREATE TABLE pports (
    shared_cable    portshare   NOT NULL DEFAULT '',
    shared_port_id  integer     DEFAULT NULL REFERENCES pports(port_id) MATCH SIMPLE,
    PRIMARY KEY (port_id),
    CONSTRAINT patchports FOREIGN KEY (node_id)
        REFERENCES patchs(node_id) MATCH FULL ON DELETE CASCADE ON UPDATE RESTRICT,
    UNIQUE (node_id, port_index),
    UNIQUE (node_id, port_name),
    CONSTRAINT pport_iftype_id_fkey FOREIGN KEY (iftype_id)
        REFERENCES iftypes(iftype_id) MATCH FULL ON DELETE RESTRICT ON UPDATE RESTRICT
)
INHERITS (nports);
ALTER TABLE pports OWNER TO lanview2;
COMMENT ON TABLE pports IS 'Patch panel/ csatlakozók port tábla';
COMMENT ON COLUMN pports.port_id IS 'Egyedi azonosító,az ősre és annak összes leszármazottjára is.';
COMMENT ON COLUMN pports.port_name IS 'Egy eszközön belül (azonos node_id) egyedi név.';
COMMENT ON COLUMN pports.port_index IS 'A port sorszáma a panelon, csatlakozón.';
COMMENT ON COLUMN pports.shared_cable IS 'Ha az UTP fali kábel megosztva van bekötve. Hátlapon!';
COMMENT ON COLUMN pports.shared_port_id IS 'Melyik másik porttal van megosztva a fali kábel bekötése. Hátlapon! Az "A" ill. "AA" megosztások esetén NULL, a többi erre mutat.';


CREATE TABLE nodes (
    node_stat       notifswitch     DEFAULT NULL,
    node_alarm_msg  varchar(255)    DEFAULT NULL,
    PRIMARY KEY (node_id),
    UNIQUE (node_name),
    UNIQUE (port_id),
    CONSTRAINT nodes_place_id_fkey FOREIGN KEY (place_id)
      REFERENCES places (place_id) MATCH FULL
      ON UPDATE CASCADE ON DELETE SET DEFAULT
)
INHERITS(patchs, nports);
ALTER TABLE nodes OWNER TO lanview2;
COMMENT ON TABLE  nodes         IS 'Passzív hálózati elemek táblája';
COMMENT ON COLUMN nodes.node_stat IS 'Az eszköz állapota.';
COMMENT ON COLUMN nodes.node_alarm_msg IS 'Riasztás esetén az eszközhöz rendelt opcionális hiba üzenet.';
-- //// LAN.INTERFACES

CREATE TABLE interfaces (
    hwaddress       macaddr     DEFAULT NULL,
    port_ostat      ifstatus    NOT NULL DEFAULT 'unknown',
    port_astat      ifstatus    DEFAULT NULL,   -- The desired state of the interface
    port_staple_id  integer     DEFAULT NULL,
        -- REFERENCES interfaces(port_id) MATCH SIMPLE ON DELETE SET NULL ON UPDATE RESTRICT,
    dualface_type   integer     DEFAULT NULL
        REFERENCES iftypes(iftype_id) MATCH SIMPLE ON DELETE RESTRICT ON UPDATE RESTRICT,
    PRIMARY KEY (port_id),
    UNIQUE(node_id, port_name),
    UNIQUE(port_name,  hwaddress),	-- EZ ITT NEM JO !!!!! Trigger függvény kell hozzá !!!
    CONSTRAINT interfaces_iftype_id_fkey FOREIGN KEY (iftype_id)
        REFERENCES iftypes(iftype_id) MATCH FULL ON DELETE RESTRICT ON UPDATE RESTRICT
)
INHERITS (nports);
ALTER TABLE interfaces OWNER TO lanview2;
COMMENT ON TABLE  interfaces               IS 'Network Interfaces Table';
COMMENT ON COLUMN interfaces.port_index    IS 'SNMP port id, vagy port index, SNMP eszköz esetén az eszközön bellül egyedinek kell lennie.';
COMMENT ON COLUMN interfaces.port_staple_id IS 'Trönkölt vagy bridzselt port esetén a trönk ill. a bridzs port ID-je';
COMMENT ON COLUMN interfaces.dualface_type IS 'Dualface port esetén a másik típus azonosító.';

-- /////

CREATE TABLE vlans (
    vlan_id     integer         PRIMARY KEY,
    vlan_name   varchar(32)     NOT NULL UNIQUE,
    vlan_descr  varchar(255)    DEFAULT NULL,
    vlan_stat   boolean         NOT NULL DEFAULT 'on'
);
ALTER TABLE vlans OWNER TO lanview2;
COMMENT ON TABLE  vlans            IS 'VLANs Table';
COMMENT ON COLUMN vlans.vlan_id    IS 'Unique ID for vlans. (802,1q ID)';
COMMENT ON COLUMN vlans.vlan_name  IS 'Name of VLAN';
COMMENT ON COLUMN vlans.vlan_descr IS 'Description for VLAN';
COMMENT ON COLUMN vlans.vlan_stat  IS 'State of VLAN (On/Off)';

-- //// SUBNET
CREATE TYPE subnettype AS ENUM ('primary', 'secondary', 'pseudo', 'private');
ALTER TYPE subnettype OWNER TO lanview2;
COMMENT ON TYPE subnettype IS
'Egy subnet típusa
    primary     elsődleges (alapértelmezett)
    secondary   másodlagos tartomány azonos vlan-ban, ill.fizikai hálózatban.
    pseudo      nem valós tartomány
    private     privát nem routolt tartomány (más tartományokkal ütközhet)';

CREATE TABLE subnets (
    subnet_id       serial          PRIMARY KEY,
    subnet_name     varchar(32)     NOT NULL UNIQUE,
    subnet_descr    varchar(255)    DEFAULT NULL,
    netaddr         cidr            NOT NULL,
    vlan_id         integer         DEFAULT NULL
            REFERENCES vlans(vlan_id) MATCH SIMPLE ON DELETE RESTRICT ON UPDATE RESTRICT,
    subnet_type     subnettype      NOT NULL DEFAULT 'primary'
);
ALTER TABLE subnets OWNER TO lanview2;
COMMENT ON TABLE subnets IS 'Alhálózatok táblája.';
COMMENT ON COLUMN subnets.subnet_id IS 'Alhálózat egyedi azonosító.';
COMMENT ON COLUMN subnets.subnet_name IS 'Az alhálózat egyedi neve.';
COMMENT ON COLUMN subnets.subnet_descr IS 'Az alhálózat leírása, ill. megjegyzés';
COMMENT ON COLUMN subnets.netaddr IS 'A hálózati cím és maszk.';
COMMENT ON COLUMN subnets.vlan_id IS 'A VLAN azonosítója, ha az alhálózat VLAN-hoz rendelhető.';
COMMENT ON COLUMN subnets.subnet_type IS 'Az alhálózat típusa.';

CREATE OR REPLACE FUNCTION subnet_check_before_update() RETURNS TRIGGER AS $$
BEGIN
    IF TG_OP = 'UPDATE' THEN
        IF NEW.subnet_id <> OLD.subnet_id THEN
            PERFORM error('Constant', -1, 'subnet_id', 'subnet_check_before_update()', TG_TABLE_NAME, TG_OP);
        END IF;
    END IF;
    RETURN NEW;
END;
$$ LANGUAGE plpgsql;
ALTER FUNCTION subnet_check_before_update()  OWNER TO lanview2;
COMMENT ON FUNCTION subnet_check_before_update() IS
'Trigger függvény, megakadályozza a subnets táblában az ID módosítását.';


CREATE TRIGGER subnets_check_before_update_trigger
BEFORE UPDATE ON subnets
    FOR EACH ROW EXECUTE PROCEDURE subnet_check_before_update();

CREATE TYPE vlantype AS ENUM ('no', 'unknown','forbidden', 'auto', 'tagged', 'untagged', 'virtual', 'hard');
-- Hard = VLAN unsupported ~= Untagged
ALTER TYPE vlantype OWNER TO lanview2;
COMMENT ON TYPE vlantype IS
'Vlan porthoz rendelésének a típusa :
no          nincs hozzárendelés
unknown     ismeretlen
forbidden   Tiltott
autó        autómatikus hozzárendelés
tagged      címkézett
untagged    nem címkézett, közvetlen
virtual     
hard        logikailag egyenértékeű az untagged-del';

CREATE TYPE settype AS ENUM ('auto', 'query', 'manual');
ALTER TYPE settype OWNER TO lanview2;
COMMENT ON TYPE settype IS
'Egy paraméter megállapításának módja:
autó    autómatikus
query   lekérdező program töltötte ki
manual  kézzel megadva';

CREATE TABLE port_vlans (
    port_vlan_id    serial      PRIMARY KEY,
    port_id         integer     NOT NULL, -- REFERENCES interfaces(port_id) ON DELETE CASCADE ON UPDATE RESTRICT,
    vlan_id         integer     REFERENCES vlans(vlan_id) MATCH FULL ON DELETE CASCADE ON UPDATE CASCADE,
    first_time      timestamp   DEFAULT CURRENT_TIMESTAMP,
    last_time       timestamp   DEFAULT CURRENT_TIMESTAMP,
    vlan_type       vlantype NOT NULL DEFAULT 'untagged',
    set_type        settype NOT NULL DEFAULT 'manual',
    UNIQUE (port_id, vlan_id)
);
ALTER TABLE port_vlans OWNER TO lanview2;
COMMENT ON TABLE port_vlans IS 'port és vlan összerendelések táblája';

-- //// IPADDRESS

CREATE TYPE addresstype AS ENUM ('fixip', 'private', 'external', 'dynamic', 'pseudo');
ALTER TYPE addresstype OWNER TO lanview2;


CREATE TABLE ipaddresses (
    ip_address_id   serial      PRIMARY KEY,
    ip_address_descr varchar(255) DEFAULT NULL,
    address         inet        DEFAULT NULL,
    ip_address_type addresstype DEFAULT 'dynamic',
    subnet_id       integer     DEFAULT NULL REFERENCES subnets(subnet_id) MATCH SIMPLE
                                    ON DELETE RESTRICT
                                    ON UPDATE RESTRICT,
    port_id         integer     NOT NULL -- REFERENCES iface_addrs(port_id)
);
ALTER TABLE ipaddresses OWNER TO lanview2;


CREATE TABLE iface_addrs (
    PRIMARY KEY (port_id),
    UNIQUE(node_id, port_name),
    UNIQUE(port_name, hwaddress),
    UNIQUE(ip_address_id),
    CONSTRAINT iface_addrs_iftype_id_fkey FOREIGN KEY (iftype_id)
        REFERENCES iftypes(iftype_id) MATCH FULL ON DELETE RESTRICT ON UPDATE RESTRICT,
    CONSTRAINT iface_addrs_dualface_type_fkey FOREIGN KEY (dualface_type)
        REFERENCES iftypes(iftype_id) MATCH SIMPLE ON DELETE RESTRICT ON UPDATE RESTRICT,
    CONSTRAINT iface_addrs_subnet_id_fkey FOREIGN KEY (subnet_id)
        REFERENCES subnets(subnet_id) MATCH FULL ON DELETE RESTRICT ON UPDATE RESTRICT
)
INHERITS (interfaces, ipaddresses);
ALTER TABLE iface_addrs OWNER TO lanview2;

COMMENT ON TABLE iface_addrs IS 'Interfész egy IP címmel.';

-- //// LAN.HOSTS

CREATE TABLE hosts (
    is_virtual              boolean NOT NULL DEFAULT FALSE,
    alarm_place_group_id    integer DEFAULT NULL
        REFERENCES place_groups(place_group_id) MATCH SIMPLE ON DELETE SET NULL ON UPDATE RESTRICT,
    PRIMARY KEY (node_id),
    UNIQUE (node_name),
    UNIQUE (port_id),
    CONSTRAINT hosts_place_id_fkey FOREIGN KEY (place_id)
        REFERENCES places (place_id) MATCH FULL ON UPDATE RESTRICT ON DELETE SET DEFAULT,
    CONSTRAINT hosts_iftype_id_fkey FOREIGN KEY (iftype_id)
        REFERENCES iftypes(iftype_id) MATCH FULL ON DELETE RESTRICT ON UPDATE RESTRICT,
    CONSTRAINT hosts_subnet_id_fkey FOREIGN KEY (subnet_id)
        REFERENCES subnets(subnet_id) MATCH FULL ON DELETE RESTRICT ON UPDATE RESTRICT,
    CONSTRAINT hosts_dualface_type_fkey FOREIGN KEY (dualface_type)
        REFERENCES iftypes(iftype_id) MATCH SIMPLE ON DELETE RESTRICT ON UPDATE RESTRICT,
    UNIQUE (ip_address_id),
    UNIQUE (hwaddress)
)
INHERITS (nodes,iface_addrs);
ALTER TABLE hosts OWNER TO lanview2;

-- //// LAN.SNMPDEVICES
CREATE TYPE snmpver AS ENUM ('1'/*, '2'*/, '2c'/*, '3'*/);
ALTER TYPE snmpver OWNER TO lanview2;

CREATE TABLE snmpdevices (
    community_rd    varchar(16)    NOT NULL DEFAULT 'public',
    community_wr    varchar(16)    DEFAULT NULL,
    snmp_ver        snmpver        NOT NULL DEFAULT '2c',
    sysdescr        varchar(255),
    sysobjectid     varchar(32),        -- OID tárolása milyen változóban lehetne még?
    sysuptime       integer,    -- ez itt egy integer, ami a másodperceket jelöli
    syscontact      varchar(255),
    sysname         varchar(255),
    syslocation     varchar(255),
    sysservices     smallint,
    vendorname      varchar(255),
    PRIMARY KEY (node_id),
    UNIQUE (node_name),
    UNIQUE (port_id),
    CONSTRAINT snmpdevices_place_id_fkey FOREIGN KEY (place_id)
        REFERENCES places (place_id) MATCH FULL ON UPDATE RESTRICT ON DELETE SET DEFAULT,
    CONSTRAINT snmpdevices_iftype_id_fkey FOREIGN KEY (iftype_id)
        REFERENCES iftypes(iftype_id) MATCH FULL ON DELETE RESTRICT ON UPDATE RESTRICT,
    CONSTRAINT snmpdevices_subnet_id_fkey FOREIGN KEY (subnet_id)
        REFERENCES subnets(subnet_id) MATCH FULL ON DELETE RESTRICT ON UPDATE RESTRICT,
    CONSTRAINT snmpdevices_dualface_type_fkey FOREIGN KEY (dualface_type)
        REFERENCES iftypes(iftype_id) MATCH SIMPLE ON DELETE RESTRICT ON UPDATE RESTRICT,
    UNIQUE (ip_address_id),
    UNIQUE (hwaddress)
)
INHERITS (hosts);
ALTER TABLE snmpdevices OWNER TO lanview2;

-- -------------------------------
-- ----- Functions, TRIGGERs -----
-- -------------------------------
--
-- Néhány node_id -vel kapcsolatos helper föggvény
--

-- A megadott hálózati node névhez visszaadja a node ID-t
-- Az összes patchs tábla leszármazotjában keres (tahát a nodes, hosts, snmpdevices-ben is).
-- Ha nincs ilyen nevű hálózati node akkor dob egy kizárást.
CREATE OR REPLACE FUNCTION node_name2id(varchar(32)) RETURNS integer AS $$
DECLARE
    id integer;
BEGIN
    SELECT node_id INTO id FROM patchs WHERE node_name = $1;
    IF NOT FOUND THEN
        PERFORM error('NameNotFound', -1, $1, 'node_name2id()', 'patchs');
    END IF;
    RETURN id;
END
$$ LANGUAGE plpgsql;

-- A megadott hálózati node ID-hez visszaadja a node nevét
-- Az összes patchs tábla leszármazotjában keres (tahát a nodes, hosts, snmpdevices-ben is).
-- Ha nincs ilyen nevű hálózati node ID akkor dob egy kizárást.
CREATE OR REPLACE FUNCTION node_id2name(integer) RETURNS varchar(32) AS $$
DECLARE
    id varchar(32);
BEGIN
    SELECT node_name INTO id FROM patchs WHERE node_id = $1;
    IF NOT FOUND THEN
        PERFORM error('IdNotFound', $1, 'node_id', 'node_id2name()', 'patchs');
    END IF;
    RETURN id;
END
$$ LANGUAGE plpgsql;

-- A node_id alapján visszaadja a tábla nevet
CREATE OR REPLACE FUNCTION node_id2table_name(integer) RETURNS text AS $$
BEGIN
    RETURN relname FROM patchs JOIN pg_class ON patchs.tableoid = pg_class.oid WHERE node_id = $1;
END
$$ LANGUAGE plpgsql;
--
-- Néhány port_id -vel kapcsolatos helper függvény
--
-- Port neve és a node neve alapján visszaadja az ID-t, ha a név nem létezik, dob egy kizárást+hiba rekord
CREATE OR REPLACE FUNCTION port_nn2id(varchar(32), varchar(32)) RETURNS integer AS $$
DECLARE
    id integer;
BEGIN
/*    id = node_name2id($2);
    SELECT port_id INTO id FROM nports WHERE port_name = $1 AND node_id = id;*/
    SELECT port_id INTO id FROM nports JOIN patchs USING (node_id) WHERE node_name = $2 AND port_name = $1;
    IF NOT FOUND THEN
        PERFORM error('NameNotFound', -1, $2 || ':' || $1, 'port_nn2id()', 'nports');
    END IF;
    RETURN id;
END
$$ LANGUAGE plpgsql;
-- Port neve és a node_id alapján visszaadja az ID-t, ha a név nem létezik, dob egy kizárást+hiba rekord
CREATE OR REPLACE FUNCTION port_name2id(varchar(32), integer) RETURNS integer AS $$
DECLARE
    id integer;
BEGIN
    SELECT port_id INTO id FROM nports WHERE port_name = $1 AND node_id = $2;
    IF NOT FOUND THEN
        PERFORM error('NameNotFound', -1, $1, 'port_name2id()', 'nports');
    END IF;
    RETURN id;
END
$$ LANGUAGE plpgsql;
-- Port ID-je alapján visszaadja az nevét, ha az ID nem létezik, dob egy kizárást+hiba rekord
CREATE OR REPLACE FUNCTION port_id2name(integer) RETURNS varchar(32) AS $$
DECLARE
    id varchar(32);
BEGIN
    SELECT port_name INTO id FROM nports WHERE port_id = $1;
    IF NOT FOUND THEN
        PERFORM error('IdNotFound', $1, 'port_id', 'port_id2name()', 'nports');
    END IF;
    RETURN id;
END
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION port_id2full_name(integer) RETURNS varchar(32) AS $$
DECLARE
    nm varchar(32);
BEGIN
    SELECT port_name || node_name INTO nm FROM nports JOIN patchs USING(node_id) WHERE port_id = $1;
    IF NOT FOUND THEN
        PERFORM error('IdNotFound', $1, 'port_id', 'port_id2full_name()', 'nports');
    END IF;
    RETURN nm;
END
$$ LANGUAGE plpgsql;

-- A port_id alapján visszaadja a tábla nevet
CREATE OR REPLACE FUNCTION port_id2table_name(integer) RETURNS text AS $$
BEGIN
    RETURN relname FROM nports JOIN pg_class ON nports.tableoid = pg_class.oid WHERE port_id = $1;
END
$$ LANGUAGE plpgsql;

--
-- TRIGGERs
--
-- **************************************************************************
-- Check port_id, (TRIGGER csak INSERT-re!)
-- A port_id-nak az összes származtatott táblával együtt kell egyedinek lennie.
-- Ha szükséges, módosítja a szekvenciát is, mert annak csak egy tulajdonosa lehet, voszont logikailag az
-- összes leszármazott a tulajdonosa.
-- Port tábla hierarhia:
-- nport +--> pports
--       +--> nodes ----------------------> hosts ---> snmpdevices
--       +--> interfaces ---> iface_addrs --^
-- Egyben ha az iftype_id = 0 ('unknown') ad neki egy jobb(nak tűnő) default értéket.

-- Check unique port_id for all inherited tables and modify sequence if necessary
CREATE OR REPLACE FUNCTION check_port_id_before_insert() RETURNS TRIGGER AS $$
DECLARE
    n integer;
    t text;
BEGIN
    SELECT COUNT(*) INTO n FROM nports WHERE port_id = NEW.port_id;
    IF n > 0 THEN
        PERFORM error('IdNotUni', NEW.port_id, 'port_id', 'port_check_before_insert()', TG_TABLE_NAME, TG_OP);
    END IF;
--    n := currval('nports_port_id_seq');
--  RAISE INFO 'SEQUENCE nports_port_id_seq = %', n;
--    IF NEW.port_id > n THEN
--      RAISE INFO 'Modify nports_port_id_seq from % to %', n, NEW.port_id;
--        PERFORM setval('nports_port_id_seq', NEW.port_id);
--    END IF;
    IF NEW.iftype_id = 0 THEN   -- if iftype is unknown
        t := node_id2table_name(NEW.node_id);
        CASE TG_TABLE_NAME
            WHEN 'pports' THEN
                NEW.iftype_id := iftype_id FROM iftypes WHERE iftype_name = 'patch';
            WHEN 'nports' THEN
                CASE t
                    WHEN 'nodes' THEN
                        NEW.iftype_id := iftype_id FROM iftypes WHERE iftype_name = 'attach';
                    WHEN 'hubs' THEN
                        NEW.iftype_id := iftype_id FROM iftypes WHERE iftype_name = 'bus';
                    ELSE
                        PERFORM error('DataError', 1, t, 'check_port_id_before_insert()', TG_TABLE_NAME, TG_OP);
                END CASE;
            WHEN 'nodes' THEN
                NEW.iftype_id := iftype_id FROM iftypes WHERE iftype_name = 'attach';
            WHEN 'interfaces', 'iface_addrs', 'hosts', 'snmpdevices'  THEN
                NEW.iftype_id := iftype_id FROM iftypes WHERE iftype_name = 'ethernet';
            ELSE
                PERFORM error('DataError', 2, 'TG_TABLE_NAME', 'check_port_id_before_insert()', TG_TABLE_NAME, TG_OP);
        END CASE;
    END IF;
    RETURN NEW;
END;
$$ LANGUAGE plpgsql;

-- The change in the prevention of port_id
CREATE OR REPLACE FUNCTION restrict_modfy_port_id_before_update() RETURNS TRIGGER AS $$
BEGIN
    IF NEW.port_id <> OLD.port_id THEN
        PERFORM error('Constant', -1, 'port_id', 'check_port_id_before_update()', TG_TABLE_NAME, TG_OP);
    END IF;
    RETURN NEW;
END;
$$ LANGUAGE plpgsql;

-- Ellenőrzi, hogy a port_id mező valóban egy port rekordra mutat-e.
-- Ha az opcionális első paraméter true, akkor megengedi a NULL értéket is.
-- Ha megadjuk a második paramétert, akkor az a tábla neve, amelyikben és amelyik leszármazottai között szerepelnie kell a port rekordnak
-- Ha a paraméter nincs megadva, akkor az összes port táblában keres (nport)
-- Ha mega van adva egy harmadik paraméter, akkor az egy tábla név, melyben nem szerpelhet a rekord (csak 'pport' lehet)
CREATE OR REPLACE FUNCTION check_reference_port_id() RETURNS TRIGGER AS $$
    ($null, $inTable, $exTable) = @{$_TD->{args}};
    if ((defined($null) && $null && !defined($_TD->{new}{port_id}))
     || ($_TD->{new}{port_id} == $_TD->{old}{port_id})) { return; } # O.K.
    if (!defined($inTable)) { $inTable = 'nports'; }
    $rv = spi_exec_query('SELECT COUNT(*) FROM ' . $inTable .' WHERE port_id = ' . $_TD->{new}{port_id});
    if( $rv->{status} ne SPI_OK_SELECT) {
        spi_exec_query("SELECT error('DataError', 'Status not SPI_OK_SELECT, but $rv->{status}', '$_TD->{table_name}', '$_TD->{event}');");
    }
    $nn  = $rv->{rows}[0]->{count};
    if ($nn == 0) {
        spi_exec_query("SELECT error('InvRef', $_TD->{new}{port_id}, '$inTable.port_id', 'check_reference_port_id()', '$_TD->{table_name}', '$_TD->{event}');");
        return "SKIP";
    }
    if ($nn != 1) {
        spi_exec_query("SELECT error('DataError', $_TD->{new}{port_id}, '$inTable.port_id', 'check_reference_port_id()', '$_TD->{table_name}', '$_TD->{event}');");
        return "SKIP";
    }
    if (defined($exTable) && $exTable) {
        $rv = spi_exec_query("SELECT COUNT(*) FROM $exTable WHERE port_id = $_TD->{new}{port_id}");
        $nn = $rv->{rows}[0]->{count};
        if ($nn != 0) {
            $cmd = "SELECT error('InvRef', $_TD->{new}{port_id}, '$exTable' ,'check_reference_port_id()', '$_TD->{table_name}', '$_TD->{event}');";
            # elog(NOTICE, $cmd);
            spi_exec_query($cmd);
            return "SKIP";
        }
    }
    return;
$$ LANGUAGE plperl;

-- Járulékos törlések egy nports rekord (és leszármazottai) tőrlése után
-- A pports, és patchs típusú node-k most nem játszanak
CREATE OR REPLACE FUNCTION delete_port_post() RETURNS TRIGGER AS $$
BEGIN
    DELETE FROM port_param_values WHERE port_id = OLD.port_id;
    DELETE FROM host_services     WHERE port_id = OLD.port_id;
    DELETE FROM ipaddresses       WHERE port_id = OLD.port_id;
    DELETE FROM port_vlans        WHERE port_id = OLD.port_id;
    DELETE FROM mactab            WHERE port_id = OLD.port_id;
    DELETE FROM phs_links_table   WHERE port_id1 = OLD.port_id OR port_id2 = OLD.port_id;
    RETURN OLD;
END;
$$ LANGUAGE plpgsql;


-- ///// TRIGGERS AND RULES for port_id key
-- port_id egyediség ellenörzése az összes nports leszármazottban
CREATE TRIGGER nports_check_port_id_before_insert               BEFORE INSERT ON nports      FOR EACH ROW EXECUTE PROCEDURE check_port_id_before_insert();
CREATE TRIGGER pports_check_port_id_before_insert               BEFORE INSERT ON pports      FOR EACH ROW EXECUTE PROCEDURE check_port_id_before_insert();
CREATE TRIGGER interfaces_check_port_id_before_insert           BEFORE INSERT ON interfaces  FOR EACH ROW EXECUTE PROCEDURE check_port_id_before_insert();
CREATE TRIGGER iface_addrs_check_port_id_before_insert          BEFORE INSERT ON iface_addrs FOR EACH ROW EXECUTE PROCEDURE check_port_id_before_insert();
CREATE TRIGGER nodes_check_port_id_before_insert                BEFORE INSERT ON nodes       FOR EACH ROW EXECUTE PROCEDURE check_port_id_before_insert();
CREATE TRIGGER hosts_check_port_id_before_insert                BEFORE INSERT ON hosts       FOR EACH ROW EXECUTE PROCEDURE check_port_id_before_insert();
CREATE TRIGGER snmpdevices_check_port_id_before_insert          BEFORE INSERT ON snmpdevices FOR EACH ROW EXECUTE PROCEDURE check_port_id_before_insert();
-- port_id módosításának a megelőzése az összes nports leszármazottban
CREATE TRIGGER nports_restrict_modfy_port_id_before_update      BEFORE UPDATE ON nports      FOR EACH ROW EXECUTE PROCEDURE restrict_modfy_port_id_before_update();
CREATE TRIGGER pports_restrict_modfy_port_id_before_update      BEFORE UPDATE ON pports      FOR EACH ROW EXECUTE PROCEDURE restrict_modfy_port_id_before_update();
CREATE TRIGGER interfaces_restrict_modfy_port_id_before_update  BEFORE UPDATE ON interfaces  FOR EACH ROW EXECUTE PROCEDURE restrict_modfy_port_id_before_update();
CREATE TRIGGER iface_addrs_restrict_modfy_port_id_before_update BEFORE UPDATE ON iface_addrs FOR EACH ROW EXECUTE PROCEDURE restrict_modfy_port_id_before_update();
CREATE TRIGGER nodes_restrict_modfy_port_id_before_update       BEFORE UPDATE ON nodes       FOR EACH ROW EXECUTE PROCEDURE restrict_modfy_port_id_before_update();
CREATE TRIGGER hosts_restrict_modfy_port_id_before_update       BEFORE UPDATE ON hosts       FOR EACH ROW EXECUTE PROCEDURE restrict_modfy_port_id_before_update();
CREATE TRIGGER snmpdevices_restrict_modfy_port_id_before_update BEFORE UPDATE ON snmpdevices FOR EACH ROW EXECUTE PROCEDURE restrict_modfy_port_id_before_update();
-- port_id idegen kulcs hivatkozások ellenözése (létrehozás, és módosítás)
CREATE TRIGGER port_param_values_check_reference_port_id BEFORE UPDATE OR INSERT ON port_param_values FOR EACH ROW EXECUTE PROCEDURE check_reference_port_id();
CREATE TRIGGER port_vlans_check_reference_port_id        BEFORE UPDATE OR INSERT ON port_vlans        FOR EACH ROW EXECUTE PROCEDURE check_reference_port_id('false', 'interfaces');
CREATE TRIGGER ipaddresses_check_reference_port_id       BEFORE UPDATE OR INSERT ON ipaddresses       FOR EACH ROW EXECUTE PROCEDURE check_reference_port_id('false', 'iface_addrs');
CREATE TRIGGER port_param_value_check_reference_port_id  BEFORE UPDATE OR INSERT ON port_param_values FOR EACH ROW EXECUTE PROCEDURE check_reference_port_id('false', 'nports', 'pports');
-- Port rekord törlésekor szükséges  kaszkád törlések !!!!!!!
CREATE TRIGGER pports_delete_port_post      AFTER DELETE ON pports      FOR EACH ROW EXECUTE PROCEDURE delete_port_post();
CREATE TRIGGER nports_delete_port_post      AFTER DELETE ON nports      FOR EACH ROW EXECUTE PROCEDURE delete_port_post();
CREATE TRIGGER interfaces_delete_port_post  AFTER DELETE ON interfaces  FOR EACH ROW EXECUTE PROCEDURE delete_port_post();
CREATE TRIGGER iface_addrs_delete_port_post AFTER DELETE ON iface_addrs FOR EACH ROW EXECUTE PROCEDURE delete_port_post();

-- Ellenörzi az id egyediségét, nem engedi megváltoztatni.
-- Ellenőrzi, hogy a address (ip cím) mező rendben van-e
-- Nem ütközik más címmel
CREATE OR REPLACE FUNCTION check_ip_address() RETURNS TRIGGER AS $$
DECLARE
    n   cidr;
BEGIN
    -- RAISE INFO 'check_ip_address() %/% NEW = %',TG_TABLE_NAME, TG_OP , NEW;
    IF TG_OP = 'UPDATE' THEN
        IF NEW.ip_address_id <> OLD.ip_address_id THEN
            PERFORM error('Constant', -1, 'ip_address_id', 'check_ip_address()', TG_TABLE_NAME, TG_OP);
        END IF;
    END IF;
--  IF NEW.address = '0.0.0.0' THEN -- A django nem tudd NULL-t átadni, helyette 0.0.0.0-t küld.
--      NEW.address := NULL;
--  END IF;
    -- A dinamikus cím mindenképpen NULL  ???!!!
    -- IF NEW.ip_address_type = 'dynamic' AND NEW.address IS NOT NULL THEN
    --     PERFORM error('DropData', NEW.ip_address_id, 'dynamic IP : ' || CAST(NEW.address AS TEXT), 'check_ip_address()', TG_TABLE_NAME, TG_OP);
    --    NEW.address := NULL;
    -- END IF;
    -- ip cím és subnet ellenörzése
    IF NEW.address IS NOT NULL THEN     -- Az új rekordban van ip cím
        IF NEW.subnet_id IS NULL AND NEW.ip_address_type <> 'external' THEN   -- Nincs subnet (id), keresünk egyet
            BEGIN
                SELECT subnet_id INTO STRICT NEW.subnet_id FROM subnets WHERE netaddr >> NEW.address;
                EXCEPTION
                    WHEN NO_DATA_FOUND THEN     -- nem találtunk
                        PERFORM error('NotFound', -1, 'subnet address for : ' || CAST(NEW.address AS TEXT), 'check_ip_address()', TG_TABLE_NAME, TG_OP);
                    WHEN TOO_MANY_ROWS THEN     -- több találat is van, nem egyértelmű
                        PERFORM error('Ambiguous',-1, 'subnet address for : ' || CAST(NEW.address AS TEXT), 'check_ip_address()', TG_TABLE_NAME, TG_OP);
            END;
            -- RAISE INFO 'Set subnet id : %', NEW.subnet_id;
        ELSIF NEW.ip_address_type <> 'external' THEN                        -- Ha megadtuk a subnet id-t is, akkor a címnek benne kell lennie
            SELECT netaddr INTO n FROM subnets WHERE subnet_id = NEW.subnet_id;
            IF NOT FOUND THEN
                PERFORM error('InvRef', NEW.subnet_id, 'subnet_id', 'check_ip_address()', TG_TABLE_NAME, TG_OP);
            END IF;
            IF NOT n >> NEW.address THEN
                PERFORM error('InvalidNAddr', NEW.subnet_id, CAST(n AS TEXT) || '>>' || CAST(NEW.address AS TEXT), 'check_ip_address()', TG_TABLE_NAME, TG_OP);
            END IF;
        ELSE
            NEW.subnet_id := NULL;
        END IF;
        -- Ha van már ilyen fix vagy pseudo ip cím, az hiba, de privat címneknél nincs ütközés
        IF NEW.address IS NOT NULL AND NEW.ip_address_type <> 'private' THEN
            IF 0 < COUNT(*) FROM ipaddresses WHERE
                         ( ip_address_type = 'fixip' OR ip_address_type = 'pseudo') AND
                         address = NEW.address AND ip_address_id <> NEW.ip_address_id THEN
                PERFORM error('IdNotUni', n, CAST(NEW.address AS TEXT), 'check_ip_address()', TG_TABLE_NAME, TG_OP);
            END IF;
            -- Töröljük az azonos dinamikus címmeket
            IF NEW.ip_address_type = 'dynamic' THEN
                UPDATE ipaddresses SET address = NULL WHERE address = NEW.address AND ip_address_type = 'dynamic' AND ip_address_id <> NEW.ip_address_id;
            END IF;
        END IF;
    ELSE
        -- Cím ként a NULL csak a dynamic típusnál megengedett
        IF NEW.ip_address_type <> 'dynamic' THEN
            PERFORM error('DataError', 0, 'NULL ip as non dynamic type', 'check_ip_address()', TG_TABLE_NAME, TG_OP);
        END IF;
        -- RAISE INFO 'IP address is NULL';
    END IF;
    -- RAISE INFO 'Return, NEW = %', NEW;
    RETURN NEW;
END;
$$ LANGUAGE plpgsql;

CREATE TRIGGER ipaddresses_check_before_modify_trigger       BEFORE INSERT OR UPDATE ON ipaddresses FOR EACH ROW EXECUTE PROCEDURE check_ip_address();
CREATE TRIGGER iface_addrs_check_before_modify_trigger       BEFORE INSERT OR UPDATE ON iface_addrs FOR EACH ROW EXECUTE PROCEDURE check_ip_address();
CREATE TRIGGER hosts_addrs_check_before_modify_trigger       BEFORE INSERT OR UPDATE ON hosts       FOR EACH ROW EXECUTE PROCEDURE check_ip_address();
CREATE TRIGGER snmpdevices_addrs_check_before_modify_trigger BEFORE INSERT OR UPDATE ON snmpdevices FOR EACH ROW EXECUTE PROCEDURE check_ip_address();

-- **************************************************************************
-- Check host_id
-- A host_id-nak az összes származtatott táblával együtt kell egyedinek lennie.
-- Ha szükséges, módosítja a szekvenciát is, mert annak csak egy tulajdonosa lehet, voszont logikailag az
-- összes leszármazott a tulajdonosa.
-- host tábla hierarhia:
-- patchs+--> nodes --> hosts --> snmpdevices
--       +--> hubs
-- Check unique host_id for all inherited tables and modify sequence if necessary

CREATE OR REPLACE FUNCTION node_check_before_insert() RETURNS TRIGGER AS $$
DECLARE
    n integer;
BEGIN
--  RAISE INFO 'Insert NODE, new id = %', NEW.node_id;
    SELECT COUNT(*) INTO n FROM patchs WHERE node_id = NEW.node_id;
    IF n > 0 THEN
        PERFORM error('IdNotUni', NEW.node_id, 'node_id', 'node_check_before_insert()', TG_TABLE_NAME, TG_OP);
    END IF;
--    n := currval('patchs_node_id_seq');
--  RAISE INFO 'SEQUENCE patchs_node_id_seq = %', n;
--    IF NEW.node_id > n THEN
--        RAISE INFO 'Modify patchs_node_id_seq from % to %', n, NEW.node_id;
--        PERFORM setval('patchs_node_id_seq', NEW.node_id);
--    END IF;
    SELECT COUNT(*) INTO n FROM patchs WHERE node_name = NEW.node_name;
    IF n > 0 THEN
        PERFORM error('NameNotUni', -1, 'node_name = ' || NEW.node_name, 'node_check_before_insert()', TG_TABLE_NAME, TG_OP);
    END IF;
    RETURN NEW;
END;
$$ LANGUAGE plpgsql;

-- The change in the prevention of node_id
CREATE OR REPLACE FUNCTION restrict_modfy_node_id_before_update() RETURNS TRIGGER AS $$
BEGIN
    IF NEW.node_id <> OLD.node_id THEN
        PERFORM error('Constant', -1, 'node_id', 'restrict_modfy_node_id_before_update()', TG_TABLE_NAME, TG_OP);
    END IF;
    RETURN NEW;
END;
$$ LANGUAGE plpgsql;

-- Ellenőrzi, hogy a node_id mező valóban egy node rekordra mutat-e.
-- Ha az opcionális első paraméter true, akkor megengedi a NULL értéket is.
-- Ha megadjuk a második paramétert, akkor az a tábla neve, amelyikben és amelyik leszármazottai között
-- szerepelnie kell a node rekordnak
-- Ha a paraméter nincs megadva, akkor az összes node táblában keres (patchs)
-- Ha megadjuk a harmadik paramétert akkor az itt megadott táblában nem szerepelhet a node_id (ONLY !!)
CREATE OR REPLACE FUNCTION check_reference_node_id() RETURNS TRIGGER AS $$
    ($null, $inTable, $outTable) = @{$_TD->{args}};
    if ((defined($null) && $null && !defined($_TD->{new}{node_id}))
     || ($_TD->{new}{node_id} == $_TD->{old}{node_id})) { return; } # O.K.
    if (!defined($inTable)) { $inTable = 'patchs'; }
    $rv = spi_exec_query('SELECT COUNT(*) FROM ' . $inTable .' WHERE node_id = ' . $_TD->{new}{node_id});
    $nn  = $rv->{rows}[0]->{count};
    if ($nn == 0) {
        spi_exec_query("SELECT error('InvRef', $_TD->{new}{node_id}, '$inTable', 'check_reference_node_id()', '$_TD->{table_name}', '$_TD->{event}');");
        return "SKIP";
    }
    if ($nn != 1) {
        spi_exec_query("SELECT error('DataError', $_TD->{new}{node_id}, '$inTable', 'check_reference_node_id()', '$_TD->{table_name}', '$_TD->{event}');");
        return "SKIP";
    }
    if (defined($outTable) && $outTable) {
        $rv = spi_exec_query('SELECT COUNT(*) FROM ONLY ' . $outTable .' WHERE node_id = ' . $_TD->{new}{node_id});
        $nn  = $rv->{rows}[0]->{count};
        if ($nn != 0) {
            spi_exec_query("SELECT error('DataError', $_TD->{new}{node_id}, '$inTable', 'check_reference_node_id()', '$_TD->{table_name}', '$_TD->{event}');");
            return "SKIP";
        }
    }
    return;
$$ LANGUAGE plperl;

CREATE OR REPLACE FUNCTION delete_patch_post() RETURNS TRIGGER AS $$
BEGIN
    DELETE FROM nports            WHERE node_id = OLD.node_id;
    RETURN OLD;
END;
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION delete_node_post() RETURNS TRIGGER AS $$
BEGIN
    DELETE FROM nports            WHERE node_id = OLD.node_id;
    DELETE FROM host_services     WHERE node_id = OLD.node_id;
    DELETE FROM port_param_values WHERE port_id = OLD.port_id;
    DELETE FROM ipaddresses       WHERE port_id = OLD.port_id;
    DELETE FROM port_vlans        WHERE port_id = OLD.port_id;
    DELETE FROM mactab            WHERE port_id = OLD.port_id;
    DELETE FROM phs_links_table   WHERE port_id1 = OLD.port_id OR port_id2 = OLD.port_id;
    RETURN OLD;
END;
$$ LANGUAGE plpgsql;

-- ///// TRIGGERS AND RULES for node_id key
-- node_id egyediség ellenörzése az összes patchs leszármazottban
CREATE TRIGGER patchs_check_node_id_before_insert               BEFORE INSERT ON patchs      FOR EACH ROW EXECUTE PROCEDURE node_check_before_insert();
CREATE TRIGGER hubs_check_node_id_before_insert                 BEFORE INSERT ON hubs        FOR EACH ROW EXECUTE PROCEDURE node_check_before_insert();
CREATE TRIGGER nodes_check_node_id_before_insert                BEFORE INSERT ON nodes       FOR EACH ROW EXECUTE PROCEDURE node_check_before_insert();
CREATE TRIGGER hosts_check_node_id_before_insert                BEFORE INSERT ON hosts       FOR EACH ROW EXECUTE PROCEDURE node_check_before_insert();
CREATE TRIGGER snmpdevices_check_node_id_before_insert          BEFORE INSERT ON snmpdevices FOR EACH ROW EXECUTE PROCEDURE node_check_before_insert();
-- node_id módosításának a megelőzése az összes patchs leszármazottban
CREATE TRIGGER patchs_restrict_modfy_node_id_before_update      BEFORE UPDATE ON patchs      FOR EACH ROW EXECUTE PROCEDURE restrict_modfy_node_id_before_update();
CREATE TRIGGER hubs_restrict_modfy_node_id_before_update        BEFORE UPDATE ON hubs        FOR EACH ROW EXECUTE PROCEDURE restrict_modfy_node_id_before_update();
CREATE TRIGGER nodes_restrict_modfy_node_id_before_update       BEFORE UPDATE ON nodes       FOR EACH ROW EXECUTE PROCEDURE restrict_modfy_node_id_before_update();
CREATE TRIGGER hosts_restrict_modfy_node_id_before_update       BEFORE UPDATE ON hosts       FOR EACH ROW EXECUTE PROCEDURE restrict_modfy_node_id_before_update();
CREATE TRIGGER snmpdevices_restrict_modfy_node_id_before_update BEFORE UPDATE ON snmpdevices FOR EACH ROW EXECUTE PROCEDURE restrict_modfy_node_id_before_update();
-- node_id idegen kulcs hivatkozások ellenözése (létrehozás, és módosítás)
CREATE TRIGGER nports_check_reference_node_id        BEFORE UPDATE OR INSERT ON nports        FOR EACH ROW EXECUTE PROCEDURE check_reference_node_id('false', 'patchs', 'patchs');
CREATE TRIGGER interfaces_check_reference_node_id    BEFORE UPDATE OR INSERT ON interfaces    FOR EACH ROW EXECUTE PROCEDURE check_reference_node_id('false', 'nodes');
CREATE TRIGGER iface_addrs_check_reference_node_id   BEFORE UPDATE OR INSERT ON iface_addrs   FOR EACH ROW EXECUTE PROCEDURE check_reference_node_id('false', 'hosts');
-- Kaszkád törlések
CREATE TRIGGER patchs_delete_patch_post     AFTER DELETE ON patchs      FOR EACH ROW EXECUTE PROCEDURE delete_patch_post();
CREATE TRIGGER hubs_delete_patch_post       AFTER DELETE ON hubs        FOR EACH ROW EXECUTE PROCEDURE delete_patch_post();
CREATE TRIGGER nodes_delete_node_post       AFTER DELETE ON nodes       FOR EACH ROW EXECUTE PROCEDURE delete_node_post();
CREATE TRIGGER hosts_delete_node_post       AFTER DELETE ON hosts       FOR EACH ROW EXECUTE PROCEDURE delete_node_post();
CREATE TRIGGER snmpdevices_delete_node_post AFTER DELETE ON snmpdevices FOR EACH ROW EXECUTE PROCEDURE delete_node_post();

-- // Nézetek

--------------------------------------------------------
-- Egy nézet, ami csak a patch típusú nod-okat mutatja
CREATE OR REPLACE VIEW only_patchs AS SELECT * FROM ONLY patchs;

ALTER VIEW only_patchs OWNER TO lanview2;

CREATE OR REPLACE RULE only_patchs_ins AS ON INSERT TO only_patchs DO INSTEAD
    INSERT INTO patchs(node_name,     node_descr,     place_id )
          VALUES ( NEW.node_name, NEW.node_descr, NEW.place_id );

CREATE OR REPLACE RULE only_patchs_del AS ON DELETE TO only_patchs DO INSTEAD
    DELETE FROM patchs WHERE node_id = OLD.node_id;

CREATE OR REPLACE RULE only_patchs_upd AS ON UPDATE TO only_patchs DO INSTEAD
    UPDATE patchs SET
        node_id    = NEW.node_id,
        node_name  = NEW.node_name,
        node_descr = NEW.node_descr,
        place_id   = NEW.place_id
    WHERE node_id = OLD.node_id;

CREATE OR REPLACE VIEW pports_named AS SELECT * FROM pports JOIN only_patchs USING(node_id);

--------------------------------------------------------
-- A DJangónak kell egy nézet az interfaces -ről, amiből kiderül, hogy valójában melyik táblában van a rekord.
CREATE OR REPLACE VIEW only_interfaces AS
    SELECT port_id, port_name, port_descr, iftype_id, node_id, port_index, hwaddress, port_ostat, port_astat
    FROM ONLY interfaces;

ALTER VIEW only_interfaces OWNER TO lanview2;
ALTER TABLE only_interfaces ALTER COLUMN port_ostat SET DEFAULT 'unknown';

CREATE OR REPLACE RULE only_interfaces_ins AS ON INSERT TO only_interfaces DO INSTEAD
    INSERT INTO interfaces (port_name, port_descr, iftype_id, node_id, port_index, hwaddress, port_ostat, port_astat)
        VALUES (NEW.port_name, NEW.port_descr, NEW.iftype_id, NEW.node_id, NEW.port_index, NEW.hwaddress, NEW.port_ostat, NEW.port_astat);


CREATE OR REPLACE RULE only_interfaces_del AS ON DELETE TO only_interfaces DO INSTEAD
    DELETE FROM interfaces WHERE port_id = OLD.port_id;

CREATE OR REPLACE RULE only_interfaces_upd AS ON UPDATE TO only_interfaces DO INSTEAD
    UPDATE interfaces SET
        port_id     = NEW.port_id,
        port_name   = NEW.port_name,
        port_descr  = NEW.port_descr,
        iftype_id   = NEW.iftype_id,
        node_id     = NEW.node_id,
        port_index  = NEW.port_index,
        hwaddress   = NEW.hwaddress,
        port_ostat  = NEW.port_ostat,
        port_astat  = NEW.port_astat
    WHERE port_id = OLD.port_id;

--------------------------------------------------------
-- A DJangónak kell egy nézet az iface_addrs -ről, amiből kiderül, hogy valójában melyik táblában van a rekord.
CREATE OR REPLACE VIEW iface_addrs_tn AS
    SELECT port_id, port_name, port_descr, iftype_id, node_id, port_index, hwaddress, port_ostat, port_astat,
            ip_address_descr, address, ip_address_type, subnet_id, relname
    FROM iface_addrs JOIN pg_class ON iface_addrs.tableoid = pg_class.oid;

ALTER VIEW iface_addrs_tn OWNER TO lanview2;
ALTER TABLE iface_addrs_tn ALTER COLUMN port_ostat SET DEFAULT 'unknown';

CREATE OR REPLACE RULE iface_addrs_tn_ins AS ON INSERT TO iface_addrs_tn DO INSTEAD
    INSERT INTO iface_addrs (port_name, port_descr, iftype_id, node_id, port_index, hwaddress, port_ostat, port_astat,
            ip_address_descr, address, ip_address_type, subnet_id)
        VALUES (NEW.port_name, NEW.port_descr, NEW.iftype_id, NEW.node_id, NEW.port_index, NEW.hwaddress, NEW.port_ostat, NEW.port_astat,
            NEW.ip_address_descr, NEW.address, NEW.ip_address_type, NEW.subnet_id);

CREATE OR REPLACE RULE iface_addrs_tn_del AS ON DELETE TO iface_addrs_tn DO INSTEAD
    DELETE FROM iface_addrs WHERE port_id = OLD.port_id;

CREATE OR REPLACE RULE iface_addrs_tn_upd AS ON UPDATE TO iface_addrs_tn DO INSTEAD
    UPDATE iface_addrs SET
        port_id     = NEW.port_id,
        port_name   = NEW.port_name,
        port_descr  = NEW.port_descr,
        iftype_id   = NEW.iftype_id,
        node_id     = NEW.node_id,
        port_index  = NEW.port_index,
        hwaddress   = NEW.hwaddress,
        port_ostat  = NEW.port_ostat,
        port_astat  = NEW.port_astat,
        ip_address_descr = NEW.ip_address_descr,
        address     = NEW.address,
        ip_address_type = NEW.ip_address_type,
        subnet_id   = NEW.subnet_id
    WHERE port_id = OLD.port_id;

--------------------------------------------------------
-- Egy nézet, ami csak a node típusú nod-okat mutatja
CREATE OR REPLACE VIEW only_nodes AS
    SELECT node_id, node_name, node_descr, place_id, port_name, port_descr, node_stat, node_alarm_msg
    FROM ONLY nodes;

ALTER VIEW only_nodes OWNER TO lanview2;

CREATE OR REPLACE RULE only_nodes_ins AS ON INSERT TO only_nodes DO INSTEAD
    INSERT INTO nodes( node_name, node_descr, place_id, port_name, port_descr, node_stat, node_alarm_msg,
                    iftype_id, port_index )
          VALUES (NEW.node_name, NEW.node_descr, NEW.place_id, NEW.port_name, NEW.port_descr, NEW.node_stat, NEW.node_alarm_msg,
            (SELECT iftype_id FROM iftypes WHERE iftype_name = 'attach'), 1);

CREATE OR REPLACE RULE only_nodes_del AS ON DELETE TO only_nodes DO INSTEAD
    DELETE FROM nodes WHERE node_id = OLD.node_id;

CREATE OR REPLACE RULE only_nodes_upd AS ON UPDATE TO only_nodes DO INSTEAD
    UPDATE nodes SET
        node_id    = NEW.node_id,
        node_name  = NEW.node_name,
        node_descr = NEW.node_descr,
        place_id   = NEW.place_id,
        port_name  = NEW.port_name,
        port_descr = NEW.port_descr,
        node_stat  = NEW.node_stat,
        node_alarm_msg = NEW.node_alarm_msg
    WHERE node_id = OLD.node_id;

--------------------------------------------------------

CREATE OR REPLACE VIEW only_hosts AS
 SELECT node_id, node_name, node_descr, place_id,
        port_name, port_descr, iftype_id, port_index,
        node_stat, node_alarm_msg,
        hwaddress, port_ostat, port_astat,
        -- port_staple_id, hosts.dualface_type,
        ip_address_descr, address, ip_address_type, subnet_id
   FROM ONLY hosts;

ALTER TABLE only_hosts OWNER TO lanview2;

CREATE OR REPLACE RULE only_hosts_ins AS ON INSERT TO only_hosts DO INSTEAD
    INSERT INTO hosts(node_name, node_descr, place_id,
            port_name, port_descr, iftype_id, port_index,
            node_stat, node_alarm_msg,
            hwaddress, port_ostat, port_astat,
            ip_address_descr, address, ip_address_type, subnet_id)
          VALUES (NEW.node_name, NEW.node_descr, NEW.place_id,
            NEW.port_name, NEW.port_descr, NEW.iftype_id, NEW.port_index,
            NEW.node_stat, NEW.node_alarm_msg,
            NEW.hwaddress, NEW.port_ostat, NEW.port_astat,
            NEW.ip_address_descr, NEW.address, NEW.ip_address_type, NEW.subnet_id);

CREATE OR REPLACE RULE only_hosts_del AS ON DELETE TO only_hosts DO INSTEAD
    DELETE FROM hosts WHERE node_id = OLD.node_id;

CREATE OR REPLACE RULE only_hosts_upd AS ON UPDATE TO only_hosts DO INSTEAD
    UPDATE hosts SET
        node_id     = NEW.node_id,
        node_name   = NEW.node_name,
        node_descr  = NEW.node_descr,
        place_id    = NEW.place_id,
        port_name   = NEW.port_name,
        port_descr  = NEW.port_descr,
        iftype_id   = NEW.iftype_id,
        port_index  = NEW.port_index,
        node_stat   = NEW.node_stat,
        node_alarm_msg = NEW.node_alarm_msg,
        hwaddress   = NEW.hwaddress,
        port_ostat  = NEW.port_ostat,
        port_astat  = NEW.port_astat,
        ip_address_descr = NEW.ip_address_descr,
        address     = NEW.address,
        ip_address_type = NEW.ip_address_type,
        subnet_id   = NEW.subnet_id
    WHERE node_id = OLD.node_id;

--------------------------------------------------------

CREATE OR REPLACE VIEW only_hubs AS
 SELECT hubs.node_id, hubs.node_name, hubs.node_descr, hubs.place_id, hubs.is_virtual
   FROM hubs WHERE is_switch = FALSE;

ALTER TABLE only_hubs OWNER TO lanview2;

--------------------------------------------------------
CREATE OR REPLACE VIEW hports AS
    SELECT port_id, port_name, port_descr, node_id, port_index
     FROM ONLY nports JOIN iftypes USING(iftype_id)
     WHERE iftype_name = 'bus';
ALTER VIEW hports OWNER TO lanview2;

CREATE OR REPLACE RULE hports_ins AS ON INSERT TO hports DO INSTEAD
    INSERT INTO nports(port_name, port_descr, node_id, port_index, iftype_id )
          VALUES ( NEW.port_name,  NEW.port_descr,  NEW.node_id,  NEW.port_index,
            (SELECT iftype_id FROM iftypes WHERE iftype_name = 'bus')  );

CREATE OR REPLACE RULE hports_del AS ON DELETE TO hports DO INSTEAD
    DELETE FROM nports WHERE port_id = OLD.port_id;

CREATE OR REPLACE RULE hports_upd AS ON UPDATE TO hports DO INSTEAD
    UPDATE nports SET
        port_id    = NEW.port_id,
        port_name  = NEW.port_name,
        port_descr = NEW.port_descr,
        node_id    = NEW.node_id,
        port_index = NEW.port_index
    WHERE port_id = OLD.port_id;

--------------------------------------------------------
-- Egy nézet, ami csak a nports típusú port-okat mutatja
CREATE OR REPLACE VIEW only_nports AS
   SELECT port_id, port_name, port_descr, iftype_id, node_id, port_index
        FROM ONLY nports JOIN iftypes USING(iftype_id)
     WHERE iftype_name <> 'bus';
ALTER TABLE only_nports OWNER TO lanview2;

CREATE OR REPLACE RULE only_nports_ins AS ON INSERT TO only_nports DO INSTEAD
    INSERT INTO nports (port_name, port_descr, iftype_id, node_id, port_index)
        VALUES (NEW.port_name, NEW.port_descr, NEW.iftype_id, NEW.node_id, NEW.port_index);

CREATE OR REPLACE RULE only_nports_del AS ON DELETE TO only_nports DO INSTEAD
    DELETE FROM nports WHERE port_id = OLD.port_id;

CREATE OR REPLACE RULE only_nports_upd AS ON UPDATE TO only_nports DO INSTEAD
    UPDATE nports SET
        port_id    = NEW.port_id,
        port_name  = NEW.port_name,
        port_descr = NEW.port_descr,
        node_id    = NEW.node_id,
        port_index = NEW.port_index
    WHERE port_id = OLD.port_id;
--------------------------------------------------------

CREATE OR REPLACE VIEW only_snmpdevices AS
    SELECT node_id, node_name, node_descr, place_id, port_name, port_descr, iftype_id,
               port_index, node_stat, node_alarm_msg, hwaddress, port_ostat, port_astat, ip_address_descr, address,
               ip_address_type, subnet_id,
               community_rd, community_wr, snmp_ver, sysdescr, sysobjectid, sysuptime, syscontact, sysname, syslocation, sysservices, vendorname
FROM snmpdevices;

ALTER TABLE only_snmpdevices OWNER TO lanview2;


CREATE OR REPLACE RULE only_snmpdevices_ins AS
    ON INSERT TO only_snmpdevices DO INSTEAD
    INSERT INTO snmpdevices
        (node_name, node_descr, place_id, port_name, port_descr, iftype_id, port_index, node_stat, node_alarm_msg, hwaddress, port_ostat, port_astat, ip_address_descr, address,
                 ip_address_type, subnet_id, community_rd, community_wr, snmp_ver, sysdescr, sysobjectid, sysuptime, syscontact, sysname, syslocation, sysservices, vendorname)
    VALUES (NEW.node_name, NEW.node_descr, NEW.place_id, NEW.port_name, NEW.port_descr, NEW.iftype_id, NEW.port_index, NEW.node_stat, NEW.node_alarm_msg, NEW.hwaddress, NEW.port_ostat, NEW.port_astat, NEW.ip_address_descr, NEW.address,
             NEW.ip_address_type, NEW.subnet_id, NEW.community_rd, NEW.community_wr, NEW.snmp_ver, NEW.sysdescr, NEW.sysobjectid, NEW.sysuptime, NEW.syscontact, NEW.sysname, NEW.syslocation, NEW.sysservices, NEW.vendorname);

CREATE OR REPLACE RULE only_snmpdevices_del AS
    ON DELETE TO only_snmpdevices DO INSTEAD
    DELETE FROM snmpdevices WHERE snmpdevices.node_id = old.node_id;

CREATE OR REPLACE RULE only_snmpdevices_upd AS
    ON UPDATE TO only_snmpdevices DO INSTEAD
    UPDATE snmpdevices SET
        node_name = NEW.node_name, node_descr = NEW.node_descr, place_id = NEW.place_id, port_name = NEW.port_name, port_descr = NEW.port_descr,
        iftype_id = NEW.iftype_id, port_index = NEW.port_index, node_stat = NEW.node_stat, hwaddress = NEW.hwaddress, port_ostat = NEW.port_ostat,
        port_astat = NEW.port_astat, node_alarm_msg = NEw.node_alarm_msg, ip_address_descr = NEW.ip_address_descr, address = NEW.address, ip_address_type = NEW.ip_address_type, subnet_id = NEW.subnet_id, community_rd = NEW.community_rd, community_wr =          NEW.community_wr, snmp_ver = NEW.snmp_ver, sysdescr = NEW.sysdescr, sysobjectid = NEW.sysobjectid, sysuptime = NEW.sysuptime, syscontact = NEW.syscontact, sysname = NEW.sysname,
        syslocation = NEW.syslocation, sysservices = NEW.sysservices, vendorname = NEW.vendorname
    WHERE snmpdevices.node_id = old.node_id;


create or replace view porstat as
 select n.node_name, n.node_stat, i.port_name, i.port_index, i.port_astat, i.port_ostat
 FROM interfaces i JOIN nodes n USING(node_id)
 ORDER BY n.node_name, i.port_name;

-- Az összes nodes, hosts, snmpdevices rekord lekérdezése, az összes lehetséges paraméterével
-- A mezőkhöz hozáadbva a tablename és tableoid mezőket.
CREATE OR REPLACE VIEW all_nodes AS
 SELECT node_id, node_name, node_descr, place_id,				-- patchs
	port_id, port_name, port_descr, iftype_id, port_index,			-- nports
	node_stat, node_alarm_msg,						-- nodes
	NULL::macaddr AS hwaddress, NULL::ifstatus AS port_ostat, NULL::ifstatus AS port_astat, NULL::integer AS port_staple_id, NULL::integer AS dualface_type,		-- interfaces
	NULL::integer AS ip_address_id, NULL AS ip_address_descr, NULL::inet AS address, NULL::addresstype AS ip_address_type, NULL::integer AS subnet_id,		-- addresses
	NULL AS community_rd, NULL AS community_wr, NULL::snmpver AS snmp_ver, NULL AS sysdescr, NULL AS sysobjectid,			-- snmpdevices
	NULL::integer AS sysuptime, NULL AS syscontact, NULL AS sysname, NULL AS syslocation, NULL::smallint AS sysservices, NULL AS vendorname,	-- snmpdevices
	tableoid, 'nodes' AS tablename,
	deleted									-- nports
  FROM ONLY nodes
UNION
 SELECT node_id, node_name, node_descr, place_id,				-- patchs
	port_id, port_name, port_descr, iftype_id, port_index,			-- nports
	node_stat, node_alarm_msg,						-- nodes
	hwaddress, port_ostat, port_astat, port_staple_id, dualface_type,	-- interfaces
	ip_address_id, ip_address_descr, address, ip_address_type, subnet_id,	-- addresses
	NULL AS community_rd, NULL AS community_wr, NULL::snmpver AS snmp_ver, NULL AS sysdescr, NULL AS sysobjectid,			-- snmpdevices
	NULL::integer AS sysuptime, NULL AS syscontact, NULL AS sysname, NULL AS syslocation, NULL::smallint AS sysservices, NULL AS vendorname,	-- snmpdevices
	tableoid, 'hosts' AS tablename,
	deleted									-- nports
  FROM ONLY hosts
UNION
 SELECT node_id, node_name, node_descr, place_id,				-- patchs
	port_id, port_name, port_descr, iftype_id, port_index,			-- nports
	node_stat, node_alarm_msg,						-- nodes
	hwaddress, port_ostat, port_astat, port_staple_id, dualface_type,	-- interfaces
	ip_address_id, ip_address_descr, address, ip_address_type, subnet_id,	-- addresses
	community_rd, community_wr, snmp_ver, sysdescr, sysobjectid,		-- snmpdevices
	sysuptime, syscontact, sysname, syslocation, sysservices, vendorname,	-- snmpdevices
	tableoid, 'snmpdevices' AS tablename,
	deleted									-- nports
  FROM ONLY snmpdevices;

-- Az összes nodes, hosts, snmpdevices -rekordhoz papcsolódó port rekord lekérdezése, az összes lehetséges paraméterével
-- A mezőkhöz hozáadbva a tablename, tableoid és is_node mezőket.
CREATE OR REPLACE VIEW all_nports AS
 SELECT port_id, port_name, port_descr, iftype_id, port_index, node_id,		-- nports
	NULL::macaddr AS hwaddress, NULL::ifstatus AS port_ostat, NULL::ifstatus AS port_astat, NULL::integer AS port_staple_id, NULL::integer AS dualface_type,	-- interfaces
	NULL::integer AS ip_address_id, NULL AS ip_address_descr, NULL::inet AS address, NULL::addresstype AS ip_address_type, NULL::integer AS subnet_id,	-- addresses
	tableoid, 'nports' AS tablename, FALSE AS is_node,
	deleted									-- nports
  FROM ONLY nports WHERE NOT node_id IN (SELECT node_id FROM hubs)
UNION
 SELECT port_id, port_name, port_descr, iftype_id, port_index, node_id,		-- nports
	NULL::macaddr AS hwaddress, NULL::ifstatus AS port_ostat, NULL::ifstatus AS port_astat, NULL::integer AS port_staple_id, NULL::integer AS dualface_type,	-- interfaces
	NULL::integer AS ip_address_id, NULL AS ip_address_descr, NULL::inet AS address, NULL::addresstype AS ip_address_type, NULL::integer AS subnet_id,	-- addresses
	tableoid, 'nodes' AS tablename, TRUE AS is_node,
	deleted									-- nports
  FROM ONLY nodes
UNION
 SELECT port_id, port_name, port_descr, iftype_id, port_index, node_id,		-- nports
	hwaddress, port_ostat, port_astat, port_staple_id, dualface_type,	-- interfaces
	NULL::integer AS ip_address_id, NULL AS ip_address_descr, NULL::inet AS address, NULL::addresstype AS ip_address_type, NULL::integer AS subnet_id,	-- addresses
	tableoid, 'iterfaces' AS tablename, FALSE AS is_node,
	deleted									-- nports
  FROM ONLY interfaces
UNION
 SELECT port_id, port_name, port_descr, iftype_id, port_index, node_id,		-- nports
	hwaddress, port_ostat, port_astat, port_staple_id, dualface_type,	-- interfaces
	ip_address_id, ip_address_descr, address, ip_address_type, subnet_id,	-- addresses
	tableoid, 'iface_addrs' AS tablename, FALSE AS is_node,
	deleted									-- nports
  FROM ONLY iface_addrs
UNION
 SELECT port_id, port_name, port_descr, iftype_id, port_index, node_id,		-- nports
	hwaddress, port_ostat, port_astat, port_staple_id, dualface_type,	-- interfaces
	ip_address_id, ip_address_descr, address, ip_address_type, subnet_id,	-- addresses
	tableoid, 'hosts' AS tablename, TRUE AS is_node,
	deleted									-- nports
  FROM ONLY hosts
UNION
 SELECT port_id, port_name, port_descr, iftype_id, port_index, node_id,		-- nports
	hwaddress, port_ostat, port_astat, port_staple_id, dualface_type,	-- interfaces
	ip_address_id, ip_address_descr, address, ip_address_type, subnet_id,	-- addresses
	tableoid, 'snmpdevices' AS tablename, TRUE AS is_node,
	deleted									-- nports
  FROM ONLY snmpdevices;



