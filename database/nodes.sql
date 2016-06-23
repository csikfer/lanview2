
CREATE TYPE nodetype AS ENUM ('patch','node', 'host', 'switch', 'hub', 'virtual', 'snmp', 'converter', 'printer', 'gateway');
ALTER TYPE nodetype OWNER TO lanview2;
COMMENT ON TYPE nodetype IS '
Típus azonosítók
"patch"         Patch panel, vagy (fali) csatlakozó.
"node"          Passzív eszköz
"host"          Aktív eszköz
"switch"        Switch
"hub"           HUB
"virtual"       Virtuális eszköz
"snmp"          SNMP képes eszköz
';

CREATE TYPE  linktype AS ENUM ( 'ptp', 'bus', 'patch', 'logical', 'wireless', 'unknown');
COMMENT ON TYPE linktype IS
'A port típus a link alapján :
    ptp         Point to point /pecselhető
    bus         BUS (egyenlőre ugyanúgy kezeljük mint a ptp -t)
    patch       Patch port Frot<-->Back / pecselhető az előlap és a hátlap is
    logical     Nincs link / nem pecselhető
    wireless    Nincs link / nem pecselhető
    unknown     Nincs link / nem pecselhető';

CREATE TYPE portobjtype AS ENUM ('nport', 'pport', 'interface', 'unknown');
COMMENT ON TYPE portobjtype IS
'Az API-ban a rekordot reprezentáló port objektumot definiálja_
    nport,        A port objektum típusa cNPort, tábla nports (passzív port)
    pport,        A port objektum típusa cPPort, tábla pport (patch ports)
    interface,    A port objektum típusa cInterface, tábla interfaces (aktív port)
    unknown       Ismeretlen (az api kizárást fog dobni!)';
    
CREATE TABLE iftypes (
    iftype_id           bigserial       PRIMARY KEY,
    iftype_name         text     NOT NULL UNIQUE,
    iftype_note         text    DEFAULT NULL,
    iftype_iana_id      integer         NOT NULL DEFAULT 1, -- 'other'
    iftype_link_type    linktype        NOT NULL DEFAULT 'unknown',
    iftype_obj_type     portobjtype     NOT NULL DEFAULT 'unknown',
    preferred           bool            DEFAULT false,
    iana_id_link        integer         DEFAULT NULL,
    if_name_prefix      text     DEFAULT NULL
);
ALTER TABLE iftypes OWNER TO lanview2;
COMMENT ON TABLE  iftypes               IS 'Network Interfaces (ports) típus leíró rekord.';
COMMENT ON COLUMN iftypes.iftype_id     IS 'Unique ID for interface''s type';
COMMENT ON COLUMN iftypes.iftype_name   IS 'Interface type''s name';
COMMENT ON COLUMN iftypes.iftype_note  IS 'Interface type''s noteiption';
COMMENT ON COLUMN iftypes.iftype_iana_id IS 'Protocoll Types id assigned by IANA';
COMMENT ON COLUMN iftypes.iftype_link_type IS 'A porton értelmezhető link típusa';
COMMENT ON COLUMN iftypes.iftype_obj_type IS 'A portot reprezentáló API objektum típusa.';
COMMENT ON COLUMN iftypes.preferred     IS 'Ha az iana id alapján keresünk, csak az ''f'' érték esetén van találat a rekordra.';
COMMENT ON COLUMN iftypes.iana_id_link IS 'Elavult ID esetén a helyette használandü iana ID-t tartalmazza';
COMMENT ON COLUMN iftypes.if_name_prefix IS 'Egy opcionális prefix az interfész nevekhez';

INSERT INTO iftypes
    (iftype_id, iftype_name,    iftype_note,              iftype_iana_id) VALUES
    ( 0,        'unknown',      'Unknown pseudo type',    1             );
INSERT INTO iftypes
    (iftype_name,               iftype_note,                iftype_iana_id, iftype_link_type, iftype_obj_type) VALUES
-- Pseudo types
    ( 'attach',                 'Érzékelő kapcsolódási pont',            1,     'ptp',        'nport' ),
    ( 'bus',                    'hub port',                              1,     'bus',        'nport' ),
    ( 'sensor',                 'Sensor port',                           1,     'ptp',        'interface' ),
    ( 'patch',                  'Patch port (UTP)',                      1,     'patch',      'pport' ),
    ( 'rs485',                  'RS-485 Serial bus interface',           1,     'bus',        'interface' ),
    ( 'iic',                    'IIC Serial bus interface',              1,     'bus',        'interface' ),
    ( 'eport',                  'unmanagement ethernet',                 1,     'ptp',        'nport' );
-- IANA iftype_id
INSERT INTO iftypes
    (iftype_name,               iftype_note,                iftype_iana_id, iftype_link_type, iftype_obj_type, preferred) VALUES
    ( 'ethernet',               'Ethernet Interface (UTP)',              6,     'ptp',        'interface',      't' ),
    ( 'veth',                   'Virtual ethernet (brX, tapX, ...)',     6,     'logical',    'interface',      'f' ),
    ( 'wireless',               'Wireless ethernet interface',           6,     'wireless',   'interface',      'f' ),
    ( 'ppp',                    'Point to Point protcol interface',     23,     'logical',    'interface',      't' ),
    ( 'softwareloopback',       'Software loopback interface',          24,     'logical',    'unknown',        'f' ),
    ( 'rs232',                  'RS232 Interface',                      33,     'ptp',        'interface',      'f' ),
    ( 'virtual',                'VLan (ProCurve)',                      53,     'logical',    'interface',      't' ),
    ( 'multiplexor',            'Trunk',                                54,     'logical',    'interface',      't' ),
    ( 'adsl',                   'Asymmetric Digital Subscriber Loop',   94,     'logical',    'interface',      't' ),
    ( 'tunnel',                 'Encapsulation interface',             131,     'logical',    'interface',      'f' ),
    ( 'digitalPowerline',       'IP over Power Lines',                 138,     'bus',        'interface',      'f' ),
    ( 'usb',                    'USB Interface',                       160,     'ptp',        'interface',      'f' );
-- IANA iftype_id obsoloted
INSERT INTO iftypes
    (iftype_name,               iftype_note,                iftype_iana_id, iana_id_link) VALUES
    ( 'gigabitEthernet',        'Obsolote',                            117,          6 ),
    ( 'l2vlan',                 'VLan (SonicWall)',                    135,         53 ),
    ( 'l3vlan',                 'VLan (HP/3com)',                      136,         53 ),
    ( 'ieee8023adLag',          'Bridge Aggregation (HP/3com)',        161,         54 );

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
    port_id     bigserial       PRIMARY KEY,
    port_name   text            NOT NULL,
    port_note   text            DEFAULT NULL,
    port_tag    text            DEFAULT NULL,
    iftype_id   bigint          DEFAULT 0   -- Default type is 'unknown'
                REFERENCES iftypes(iftype_id) MATCH FULL ON DELETE RESTRICT ON UPDATE RESTRICT,
    node_id     bigint          NOT NULL,   -- REFERENCES (patch, nodes, snmp_devs)
    port_index  integer         DEFAULT NULL,
    deleted     boolean         DEFAULT false,
    flag        boolean         DEFAULT false,
    UNIQUE(node_id, port_name)
);
ALTER TABLE nports OWNER TO lanview2;
COMMENT ON TABLE  nports            IS 'Passzív portok táblája, az összes port típus őse';
COMMENT ON COLUMN nports.port_id    IS 'Egyedi port azonosító, az összes port típusra (leszármazottra) egyedi';
COMMENT ON COLUMN nports.port_name  IS 'A port neve, csak egy nodon bellül (azonos node_id) kell egyedinek lennie';
COMMENT ON COLUMN nports.port_note  IS 'Description for network port';
COMMENT ON COLUMN nports.port_tag   IS 'Opcionális cimke ill. név.';
COMMENT ON COLUMN nports.iftype_id  IS 'A típus leíró rekord azonosítója.';
COMMENT ON COLUMN nports.node_id    IS 'Csomópont azonosító, idegen kulcs a tulajdonos nodes vagy bármelyi leszármazottja rekordjára';
COMMENT ON COLUMN nports.port_index IS 'Opcionális port index. Egyes leszármazottaknál kötelező, ha meg van adva, akkor a port_name -hez hasonlóan egyedinek kell lennie.';
COMMENT ON COLUMN nports.deleted    IS 'Ha igaz, akkor a port logikailag törölve lett.';

CREATE TABLE patchs (
    node_id     bigserial       PRIMARY KEY,    -- Sequence: patchs_node_id_seq
    node_name   text            NOT NULL UNIQUE,
    node_note   text            DEFAULT NULL,
    node_type   nodetype[]      DEFAULT NULL,
    place_id    bigint          DEFAULT 0   -- place = 'unknown'
                REFERENCES places(place_id) MATCH FULL ON DELETE SET DEFAULT ON UPDATE RESTRICT,
    features    text            DEFAULT NULL,
    deleted     boolean         NOT NULL DEFAULT false
);
ALTER TABLE patchs OWNER TO lanview2;
COMMENT ON TABLE  patchs            IS 'Patch panel/csatlakozók/kapcsolódási pont tábla';
COMMENT ON COLUMN patchs.node_id    IS 'Unique ID for node. Az összes leszármazottra és az ősre is egyedi.';
COMMENT ON COLUMN patchs.node_name  IS 'Unique Name of the node. Az összes leszármazottra és az ősre is egyedi.';
COMMENT ON COLUMN patchs.node_note  IS 'Descrition of the node';
COMMENT ON COLUMN patchs.place_id   IS 'Az eszköz helyét azonosító "opcionális" távoli kulcs. Alapértelmezett hely a ''unknown''.';
COMMENT ON COLUMN patchs.node_type  IS 'A hálózati elem típusa, a patch típusú rekord esetén mindíg "patch", a trigger állítja be.';

-- //// LAN.PPORT S  Pach Ports
CREATE TYPE portshare AS ENUM ('',      -- 1,2,3,4,5,6,7,8  / nincs megosztás
                               'A',     -- 1,2,3,6
                               'AA',    -- 1,2
                               'AB',    -- 3.6
                               'B',     -- 4,5,7,8
                               'BA',    -- 4,5
                               'BB',    -- 7,8
                               'C',     -- 3,4
                               'D',     -- 5,6
                               'NC');   -- No Connected
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
    ''D''   5,6
    ''NC''  Nincs bekötve';
-- A megengedett, egymással nem ütköző konkurens SHARE kombinációk
CREATE OR REPLACE FUNCTION check_shared(portshare, portshare) RETURNS boolean AS $$
BEGIN
    IF $1 = 'NC' OR $2 = 'NC' THEN
        PERFORM error('DataError', -1, 'shared_cable', 'check_shared()', 'pports');
    END IF;
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
    IF $1 = 'NC' OR $2 = 'NC' THEN
        PERFORM error('DataError', -1, 'shared_cable', 'min_shared()', 'pports');
    END IF;
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
    i       bigint;
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
    shared_port_id  bigint     DEFAULT NULL REFERENCES pports(port_id) MATCH SIMPLE,
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

-- //// LAN.INTERFACES

CREATE TABLE interfaces (
    hwaddress       macaddr     DEFAULT NULL,
    port_ostat      ifstatus    NOT NULL DEFAULT 'unknown',
    port_astat      ifstatus    DEFAULT NULL,   -- The desired state of the interface
    port_staple_id  bigint      DEFAULT NULL
        REFERENCES interfaces(port_id) MATCH SIMPLE ON DELETE SET NULL ON UPDATE RESTRICT,
    dualface_type   bigint      DEFAULT NULL
        REFERENCES iftypes(iftype_id) MATCH SIMPLE ON DELETE RESTRICT ON UPDATE RESTRICT,
    PRIMARY KEY (port_id),
    UNIQUE(node_id, port_name),
    -- UNIQUE(port_name,  hwaddress),	-- EZ ITT NEM JO !!!!! Trigger függvény kell hozzá !!!
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
    vlan_id     bigint          PRIMARY KEY,
    vlan_name   text            NOT NULL UNIQUE,
    vlan_note   text            DEFAULT NULL,
    vlan_stat   boolean         NOT NULL DEFAULT 'on',
    flag        boolean         DEFAULT false
);
ALTER TABLE vlans OWNER TO lanview2;
COMMENT ON TABLE  vlans            IS 'VLANs Table';
COMMENT ON COLUMN vlans.vlan_id    IS 'Unique ID for vlans. (802,1q ID)';
COMMENT ON COLUMN vlans.vlan_name  IS 'Name of VLAN';
COMMENT ON COLUMN vlans.vlan_note  IS 'Description for VLAN';
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
    subnet_id       bigserial   PRIMARY KEY,
    subnet_name     text        NOT NULL UNIQUE,
    subnet_note     text        DEFAULT NULL,
    netaddr         cidr        NOT NULL,
    vlan_id         bigint      DEFAULT NULL
            REFERENCES vlans(vlan_id) MATCH SIMPLE ON DELETE RESTRICT ON UPDATE RESTRICT,
    subnet_type     subnettype  NOT NULL DEFAULT 'primary'
);
ALTER TABLE subnets OWNER TO lanview2;
COMMENT ON TABLE subnets IS 'Alhálózatok táblája.';
COMMENT ON COLUMN subnets.subnet_id IS 'Alhálózat egyedi azonosító.';
COMMENT ON COLUMN subnets.subnet_name IS 'Az alhálózat egyedi neve.';
COMMENT ON COLUMN subnets.subnet_note IS 'Az alhálózat leírása, ill. megjegyzés';
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

CREATE TYPE vlantype AS ENUM ('no', 'unknown','forbidden', 'auto', 'auto_tagged', 'tagged', 'untagged', 'virtual', 'hard');
-- Hard = VLAN unsupported ~= Untagged
ALTER TYPE vlantype OWNER TO lanview2;
COMMENT ON TYPE vlantype IS
'Vlan porthoz rendelésének a típusa :
no          nincs hozzárendelés
unknown     ismeretlen
forbidden   Tiltott
auto        autómatikus hozzárendelés
auto_tagged     Cimkézett, automatikusan
tagged      címkézett
untagged    nem címkézett, közvetlen
virtual     
hard        logikailag egyenértékeű az untagged-del';

CREATE TYPE settype AS ENUM ('auto', 'query', 'config', 'manual');
ALTER TYPE settype OWNER TO lanview2;
COMMENT ON TYPE settype IS
'Egy paraméter megállapításának módja:
auto    autómatikus
query   lekérdező program töltötte ki
cpnfig	Konfigurációs állományból importálva
manual  kézzel megadva';

CREATE TABLE port_vlans (
    port_vlan_id    bigserial      PRIMARY KEY,
    port_id         bigint     REFERENCES interfaces(port_id) MATCH FULL ON DELETE CASCADE ON UPDATE RESTRICT,
    vlan_id         bigint     REFERENCES vlans(vlan_id)      MATCH FULL ON DELETE CASCADE ON UPDATE RESTRICT,
    first_time      timestamp   DEFAULT CURRENT_TIMESTAMP,
    last_time       timestamp   DEFAULT CURRENT_TIMESTAMP,
    vlan_type       vlantype    NOT NULL DEFAULT 'untagged',
    set_type        settype     NOT NULL DEFAULT 'manual',
    flag            boolean     DEFAULT false,
    UNIQUE (port_id, vlan_id)
);
ALTER TABLE port_vlans OWNER TO lanview2;
COMMENT ON TABLE port_vlans IS 'port és vlan összerendelések táblája';

-- //// IPADDRESS

CREATE TYPE addresstype AS ENUM ('fixip', 'private', 'external', 'dynamic', 'pseudo');
ALTER TYPE addresstype OWNER TO lanview2;


CREATE TABLE ipaddresses (
    ip_address_id   bigserial   PRIMARY KEY,
    ip_address_note text        DEFAULT NULL,
    address         inet        DEFAULT NULL,
    ip_address_type addresstype NOT NULL,
    preferred       int         DEFAULT NULL,
    subnet_id       bigint      DEFAULT NULL
        REFERENCES subnets(subnet_id) MATCH SIMPLE ON DELETE RESTRICT ON UPDATE RESTRICT,
    port_id         bigint      NOT NULL
        REFERENCES interfaces(port_id) MATCH FULL ON DELETE CASCADE ON UPDATE RESTRICT,
    flag            boolean     DEFAULT false
);
ALTER TABLE ipaddresses OWNER TO lanview2;
COMMENT ON TABLE  ipaddresses IS 'IP címek';
COMMENT ON COLUMN ipaddresses.address IS 'Az IP cím (nem tartomány)';
COMMENT ON COLUMN ipaddresses.ip_address_type IS 'A cím típusa';
COMMENT ON COLUMN ipaddresses.preferred IS 'Cím keresésnél egy opcionális sorrendet definiál, a kisebb érték az preferált.';
COMMENT ON COLUMN ipaddresses.port_id IS 'A tulajdonos port, melyhez a cím tartozik';
-- //// NODES;

CREATE TABLE nodes (
    node_stat       notifswitch     DEFAULT NULL,
    PRIMARY KEY (node_id),
    UNIQUE (node_name),
    CONSTRAINT nodes_place_id_fkey FOREIGN KEY (place_id)
      REFERENCES places (place_id) MATCH FULL  ON UPDATE CASCADE ON DELETE SET DEFAULT
)
INHERITS(patchs);
ALTER TABLE nodes OWNER TO lanview2;
COMMENT ON TABLE  nodes         IS 'Passzív vagy aktív hálózati elemek táblája';
COMMENT ON COLUMN nodes.node_stat IS 'Az eszköz állapota.';
COMMENT ON COLUMN nodes.node_type IS 'Típus azonosítók. ha NULL, akkor a trigger f. {node}-ra állítja.';

-- //// LAN.SNMPDEVICES
CREATE TYPE snmpver AS ENUM ('1'/*, '2'*/, '2c'/*, '3'*/);
ALTER TYPE snmpver OWNER TO lanview2;

INSERT INTO param_types
    (param_type_name,    param_type_type,   param_type_note)    VALUES
    ('search_domain',    'text',          'System paraméter: Search domain names(s)');

CREATE TABLE snmpdevices (
    community_rd    text    NOT NULL DEFAULT 'public',
    community_wr    text    DEFAULT NULL,
    snmp_ver        snmpver        NOT NULL DEFAULT '2c',
    sysdescr        text,
    sysobjectid     text,        -- OID tárolása milyen változóban lehetne még?
    sysuptime       bigint,    -- ez itt egy bigint, ami a másodperceket jelöli
    syscontact      text,
    sysname         text,
    syslocation     text,
    sysservices     smallint,
    vendorname      text,
    PRIMARY KEY (node_id),
    UNIQUE (node_name),
    CONSTRAINT snmpdevices_place_id_fkey FOREIGN KEY (place_id)
        REFERENCES places (place_id) MATCH FULL ON UPDATE RESTRICT ON DELETE SET DEFAULT
)
INHERITS (nodes);
ALTER TABLE snmpdevices OWNER TO lanview2;

CREATE TABLE dyn_addr_ranges (
    dyn_addr_range_id   bigserial       PRIMARY KEY,
    dyn_addr_range_note text            DEFAULT NULL,
    exclude		boolean		DEFAULT false,
    begin_address       inet            NOT NULL,
    end_address         inet            NOT NULL,
    subnet_id           bigint          REFERENCES subnets(subnet_id) MATCH FULL ON DELETE CASCADE ON UPDATE RESTRICT,
    host_service_id     bigint,
    last_time           timestamp       DEFAULT CURRENT_TIMESTAMP,
    flag                boolean         DEFAULT false,
    UNIQUE (begin_address, exclude),
    UNIQUE (end_address, exclude)
    
);
ALTER TABLE dyn_addr_ranges OWNER TO lanview2;

-- -------------------------------
-- ----- Functions, TRIGGERs -----
-- -------------------------------
--
-- Néhány node_id -vel kapcsolatos helper föggvény
--

-- A megadott hálózati node névhez visszaadja a node ID-t
-- Az összes patchs tábla leszármazotjában keres (tahát a nodes, snmpdevices-ben is).
-- Ha nincs ilyen nevű hálózati node akkor dob egy kizárást.
CREATE OR REPLACE FUNCTION node_name2id(text) RETURNS bigint AS $$
DECLARE
    id bigint;
BEGIN
    SELECT node_id INTO id FROM patchs WHERE node_name = $1 AND deleted = false;
    IF NOT FOUND THEN
        PERFORM error('NameNotFound', -1, $1, 'node_name2id()', 'patchs');
    END IF;
    RETURN id;
END
$$ LANGUAGE plpgsql;

-- A megadott hálózati node ID-hez visszaadja a node nevét
-- Az összes patchs tábla leszármazotjában keres (tahát a nodes, snmpdevices-ben is).
-- Ha nincs ilyen nevű hálózati node ID akkor dob egy kizárást.
CREATE OR REPLACE FUNCTION node_id2name(bigint) RETURNS text AS $$
DECLARE
    id text;
BEGIN
    IF $1 IS NULL THEN
        return NULL;
    END IF;
    SELECT node_name INTO id FROM patchs WHERE node_id = $1;
    IF NOT FOUND THEN
        PERFORM error('IdNotFound', $1, 'node_id', 'node_id2name()', 'patchs');
    END IF;
    RETURN id;
END
$$ LANGUAGE plpgsql;

-- A node_id alapján visszaadja a tábla nevet
CREATE OR REPLACE FUNCTION node_id2table_name(bigint) RETURNS text AS $$
BEGIN
    RETURN relname FROM patchs JOIN pg_class ON patchs.tableoid = pg_class.oid WHERE node_id = $1;
END
$$ LANGUAGE plpgsql;
--
-- Néhány port_id -vel kapcsolatos helper függvény
--
-- Port ID-je alapján visszaadja az nevét, ha az ID nem létezik, dob egy kizárást+hiba rekord
CREATE OR REPLACE FUNCTION port_id2name(bigint) RETURNS text AS $$
DECLARE
    name text;
BEGIN
    IF $1 IS NULL THEN
        return NULL;
    END IF;
    SELECT port_name INTO name FROM nports WHERE port_id = $1;
    IF NOT FOUND THEN
        PERFORM error('IdNotFound', $1, 'port_id', 'port_id2name()', 'nports');
    END IF;
    RETURN name;
END
$$ LANGUAGE plpgsql;

-- Port ID-je alapján visszaadja az nevét, ha az ID nem létezik, dob egy kizárást+hiba rekord
CREATE OR REPLACE FUNCTION port_id2full_name(bigint) RETURNS text AS $$
DECLARE
    name text;
BEGIN
    IF $1 IS NULL THEN
        return NULL;
    END IF;
    SELECT node_id2name(node_id) || ':' || port_name INTO name FROM nports WHERE port_id = $1;
    IF NOT FOUND THEN
        PERFORM error('IdNotFound', $1, 'port_id', 'port_id2full_name()', 'nports');
    END IF;
    RETURN name;
END
$$ LANGUAGE plpgsql;

-- A port_id alapján visszaadja a tábla nevet
CREATE OR REPLACE FUNCTION port_id2table_name(bigint) RETURNS text AS $$
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
-- nport --> pports --> interfaces

-- Check unique port_id for all inherited tables and modify sequence if necessary
CREATE OR REPLACE FUNCTION check_port_id_before_insert() RETURNS TRIGGER AS $$
DECLARE
    n bigint;
    t text;
    node nodes;
BEGIN
    SELECT COUNT(*) INTO n FROM nports WHERE port_id = NEW.port_id;
    IF n > 0 THEN
        PERFORM error('IdNotUni', NEW.port_id, 'port_id', 'port_check_before_insert()', TG_TABLE_NAME, TG_OP);
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

\i portparams.sql

-- Járulékos törlések egy nports rekord (és leszármazottai) tőrlése után
-- A pports, és patchs típusú node-k most nem játszanak
CREATE OR REPLACE FUNCTION delete_port_post() RETURNS TRIGGER AS $$
BEGIN
    DELETE FROM port_params       WHERE port_id = OLD.port_id;
    DELETE FROM host_services     WHERE port_id = OLD.port_id;
    DELETE FROM port_vlans        WHERE port_id = OLD.port_id;
    DELETE FROM mactab            WHERE port_id = OLD.port_id;
    DELETE FROM phs_links_table   WHERE port_id1 = OLD.port_id OR port_id2 = OLD.port_id;
    DELETE FROM lldp_links_table  WHERE port_id1 = OLD.port_id OR port_id2 = OLD.port_id;
    RETURN OLD;
END;
$$ LANGUAGE plpgsql;


CREATE OR REPLACE FUNCTION check_interface() RETURNS TRIGGER AS $$
DECLARE
    n integer;
BEGIN
    IF NEW.hwaddress IS NULL THEN
        RETURN NEW;
    END IF;
    SELECT COUNT(*) INTO n FROM interfaces WHERE node_id <> NEW.node_id AND hwaddress = NEW.hwaddress;
    IF n > 0 THEN
        PERFORM error('IdNotUni', NEW.port_id, NEW.hwaddress, 'check_interface()', TG_TABLE_NAME, TG_OP);
        RETURN NULL;
    END IF;
    RETURN NEW;
END;
$$ LANGUAGE plpgsql;
COMMENT ON FUNCTION check_interface() IS 'Különböző node_id esetén nem lehet két azonos MAC';

-- ///// TRIGGERS AND RULES for port_id key
-- port_id egyediség ellenörzése az összes nports leszármazottban
CREATE TRIGGER nports_check_port_id_before_insert               BEFORE INSERT ON nports      FOR EACH ROW EXECUTE PROCEDURE check_port_id_before_insert();
CREATE TRIGGER pports_check_port_id_before_insert               BEFORE INSERT ON pports      FOR EACH ROW EXECUTE PROCEDURE check_port_id_before_insert();
CREATE TRIGGER interfaces_check_port_id_before_insert           BEFORE INSERT ON interfaces  FOR EACH ROW EXECUTE PROCEDURE check_port_id_before_insert();
-- port_id módosításának a megelőzése az összes nports leszármazottban
CREATE TRIGGER nports_restrict_modfy_port_id_before_update      BEFORE UPDATE ON nports      FOR EACH ROW EXECUTE PROCEDURE restrict_modfy_port_id_before_update();
CREATE TRIGGER pports_restrict_modfy_port_id_before_update      BEFORE UPDATE ON pports      FOR EACH ROW EXECUTE PROCEDURE restrict_modfy_port_id_before_update();
CREATE TRIGGER interfaces_restrict_modfy_port_id_before_update  BEFORE UPDATE ON interfaces  FOR EACH ROW EXECUTE PROCEDURE restrict_modfy_port_id_before_update();
CREATE TRIGGER intergaces_check_interface                       BEFORE INSERT OR UPDATE ON interfaces FOR EACH ROW EXECUTE PROCEDURE check_interface();
-- port_id idegen kulcs hivatkozások ellenözése (létrehozás, és módosítás)
CREATE TRIGGER port_params_check_reference_port_id BEFORE UPDATE OR INSERT ON port_params FOR EACH ROW EXECUTE PROCEDURE check_reference_port_id();
CREATE TRIGGER port_vlans_check_reference_port_id        BEFORE UPDATE OR INSERT ON port_vlans        FOR EACH ROW EXECUTE PROCEDURE check_reference_port_id('false', 'interfaces');
CREATE TRIGGER port_param_value_check_reference_port_id  BEFORE UPDATE OR INSERT ON port_params FOR EACH ROW EXECUTE PROCEDURE check_reference_port_id('false', 'nports', 'pports');
-- Port rekord törlésekor szükséges  kaszkád törlések !!!!!!!
CREATE TRIGGER pports_delete_port_post      AFTER DELETE ON pports      FOR EACH ROW EXECUTE PROCEDURE delete_port_post();
CREATE TRIGGER nports_delete_port_post      AFTER DELETE ON nports      FOR EACH ROW EXECUTE PROCEDURE delete_port_post();
CREATE TRIGGER interfaces_delete_port_post  AFTER DELETE ON interfaces  FOR EACH ROW EXECUTE PROCEDURE delete_port_post();


CREATE OR REPLACE FUNCTION replace_dyn_addr_range(baddr inet, eaddr inet, hsid bigint DEFAULT NULL, excl boolean DEFAULT false, snid bigint DEFAULT NULL) RETURNS reasons AS $$
DECLARE
    brec dyn_addr_ranges;
    erec dyn_addr_ranges;
    bcol  boolean;
    ecol  boolean;
    r     reasons;
    snid2 bigint;
BEGIN
    IF snid IS NULL THEN 
        BEGIN       -- kezdő cím subnet-je
            SELECT subnet_id INTO STRICT snid FROM subnets WHERE netaddr >> baddr;
            EXCEPTION
                WHEN NO_DATA_FOUND THEN     -- nem találtunk
                    RETURN 'notfound';
                WHEN TOO_MANY_ROWS THEN     -- több találat is van, nem egyértelmű
                    RETURN 'ambiguous';
        END;
        BEGIN       -- vég cím subnet-je
            SELECT subnet_id INTO STRICT snid2 FROM subnets WHERE netaddr >> eaddr;
            EXCEPTION
                WHEN NO_DATA_FOUND THEN     -- nem találtunk
                    RETURN 'notfound';
                WHEN TOO_MANY_ROWS THEN     -- több találat is van, nem egyértelmű
                    RETURN 'ambiguous';
        END;
        IF snid <> snid2 THEN
            RETURN 'caveat';
        END IF;
    END IF;
    
    SELECT * INTO brec FROM dyn_addr_ranges WHERE begin_address <= baddr AND baddr <= end_address AND exclude = excl;
    bcol := FOUND;
    SELECT * INTO erec FROM dyn_addr_ranges WHERE begin_address <= eaddr AND eaddr <= end_address AND exclude = excl;
    ecol := FOUND;
    IF bcol AND ecol AND brec.dyn_addr_range_id = erec.dyn_addr_range_id AND baddr = brec.begin_address AND brec.end_address = eaddr THEN
        RAISE INFO '1: % < % ; % = %', baddr, eaddr, brec, erec; 
        UPDATE dyn_addr_ranges
            SET
                last_time = CURRENT_TIMESTAMP,
                host_service_id = hsid,
                flag = false
            WHERE dyn_addr_range_id = brec.dyn_addr_range_id;
        RETURN 'update';
    END IF;
    IF bcol AND ecol THEN
        RAISE INFO '2: % < % ; % <> %', baddr, eaddr, brec, erec;
        if brec.dyn_addr_range_id <> erec.dyn_addr_range_id THEN
            DELETE FROM dyn_addr_ranges WHERE dyn_addr_range_id = erec.dyn_addr_range_id;
            r := 'remove';
        ELSE
            r := 'modify';
        END IF;
        UPDATE dyn_addr_ranges
            SET
                begin_address = baddr,
                end_address = eaddr,
                subnet_id = snid,
                last_time = CURRENT_TIMESTAMP,
                host_service_id = hsid,
                flag = false
            WHERE dyn_addr_range_id = brec.dyn_addr_range_id;
        RETURN r;
    END IF;
    IF bcol OR ecol THEN
        if ecol THEN
            RAISE INFO '3: % < % ; ecol %', baddr, eaddr, erec;
            brec := erec;
        ELSE
            RAISE INFO '3: % < % ; bcol %', baddr, eaddr, brec;
        END IF;
        UPDATE dyn_addr_ranges
            SET
                begin_address = baddr,
                end_address = eaddr,
                subnet_id = snid,
                last_time = CURRENT_TIMESTAMP,
                host_service_id = hsid,
                flag = false
            WHERE dyn_addr_range_id = brec.dyn_addr_range_id;
        RETURN 'modify';
    END IF;
    RAISE INFO '4: % < % ', baddr, eaddr;
    INSERT INTO dyn_addr_ranges (begin_address, end_address, exclude, subnet_id, host_service_id) VALUES(baddr, eaddr, excl, snid, hsid);
    return 'insert';
END;
$$ LANGUAGE plpgsql;

-- Ellenőrzi, hogy a paraméterként megadott IP cím része-e egy dinamikus IP tartománynak,
CREATE OR REPLACE FUNCTION is_dyn_addr(inet) RETURNS bigint AS $$
DECLARE
    id  bigint;
BEGIN
    SELECT dyn_addr_ranges_id INTO id FROM dyn_addr_ranges WHERE exclude = false AND begin_address <= $1 AND $1 <= end_address;
    IF NOT FOUND THEN
        RETURN NULL;
    END IF;
    SELECT dyn_addr_ranges_id         FROM dyn_addr_ranges WHERE exclude = true  AND begin_address <= $1 AND $1 <= end_address;
    IF FOUND THEN
        RETURN NULL;
    END IF;
    RETURN id;
END;
$$ LANGUAGE plpgsql;

-- Ellenőrzi, hogy a address (ip cím) mező rendben van-e
-- Nem ütközik más címmel
CREATE OR REPLACE FUNCTION check_ip_address() RETURNS TRIGGER AS $$
DECLARE
    n   cidr;
BEGIN
 -- RAISE INFO 'check_ip_address() %/% NEW = %',TG_TABLE_NAME, TG_OP , NEW;
    -- Az új rekordban van ip cím
    IF NEW.ip_address_type IS NULL THEN
        IF NEW.address IS NULL OR is_dyn_addr(NEW.address) THEN
            NEW.ip_address_type := 'dynamic';
        ELSE
            NEW.ip_address_type := 'fixip';
        END IF;
    END IF;
    IF NEW.address IS NOT NULL THEN
        -- Nincs subnet (id), keresünk egyet
        IF NEW.subnet_id IS NULL AND NEW.ip_address_type <> 'external' THEN
            BEGIN
                SELECT subnet_id INTO STRICT NEW.subnet_id FROM subnets WHERE netaddr >> NEW.address;
                EXCEPTION
                    WHEN NO_DATA_FOUND THEN     -- nem találtunk
                        PERFORM error('NotFound', -1, 'subnet address for : ' || CAST(NEW.address AS TEXT), 'check_ip_address()', TG_TABLE_NAME, TG_OP);
                    WHEN TOO_MANY_ROWS THEN     -- több találat is van, nem egyértelmű
                        PERFORM error('Ambiguous',-1, 'subnet address for : ' || CAST(NEW.address AS TEXT), 'check_ip_address()', TG_TABLE_NAME, TG_OP);
            END;
            -- RAISE INFO 'Set subnet id : %', NEW.subnet_id;
        -- Ha megadtuk a subnet id-t is, akkor a címnek benne kell lennie
        ELSIF NEW.ip_address_type <> 'external' THEN
            SELECT netaddr INTO n FROM subnets WHERE subnet_id = NEW.subnet_id;
            IF NOT FOUND THEN
                PERFORM error('InvRef', NEW.subnet_id, 'subnet_id', 'check_ip_address()', TG_TABLE_NAME, TG_OP);
            END IF;
            IF NOT n >> NEW.address THEN
                PERFORM error('InvalidNAddr', NEW.subnet_id, CAST(n AS TEXT) || '>>' || CAST(NEW.address AS TEXT), 'check_ip_address()', TG_TABLE_NAME, TG_OP);
            END IF;
        ELSE
            -- external típusnál mindíg NULL a subnet_id
            NEW.subnet_id := NULL;
        END IF;
        -- Ha van már ilyen fix vagy pseudo ip cím, az baj, de privat címneknél nincs ütközés
        IF NEW.address IS NOT NULL AND NEW.ip_address_type <> 'private' THEN
            -- Töröljük az azonos dinamikus címmeketm ha van
            IF NEW.ip_address_type = 'dynamic' THEN
                UPDATE ipaddresses SET address = NULL WHERE address = NEW.address AND ip_address_type = 'dynamic' AND ip_address_id <> NEW.ip_address_id;
            -- minden egyébb ütközés hiba
            ELSIF 0 < COUNT(*) FROM ipaddresses WHERE
                         ( ip_address_type = 'fixip' OR ip_address_type = 'pseudo') AND
                         address = NEW.address AND ip_address_id <> NEW.ip_address_id THEN
                PERFORM error('IdNotUni', n, CAST(NEW.address AS TEXT), 'check_ip_address()', TG_TABLE_NAME, TG_OP);
            END IF;
        END IF;
        -- Ha a preferred nincs megadva, akkor az elsőnek megadott cím a preferált
        IF NEW.preferred IS NULL THEN
            SELECT 1 + COUNT(*) INTO NEW.preferred FROM interfaces JOIN ipaddresses USING(port_id) WHERE port_id = NEW.port_id AND preferred IS NOT NULL AND address IS NOT NULL;
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

-- **************************************************************************
-- Check host_id
-- A host_id-nak az összes származtatott táblával együtt kell egyedinek lennie.
-- Ha szükséges, módosítja a szekvenciát is, mert annak csak egy tulajdonosa lehet, voszont logikailag az
-- összes leszármazott a tulajdonosa.
-- host tábla hierarhia:
-- patchs --> nodes --> snmpdevices
-- Check unique host_id for all inherited tables and modify sequence if necessary

CREATE OR REPLACE FUNCTION node_check_before_insert() RETURNS TRIGGER AS $$
DECLARE
    n bigint;
BEGIN
--  RAISE INFO 'Insert NODE, new id = %', NEW.node_id;
    SELECT COUNT(*) INTO n FROM patchs WHERE node_id = NEW.node_id;
    IF n > 0 THEN
        PERFORM error('IdNotUni', NEW.node_id, 'node_id', 'node_check_before_insert()', TG_TABLE_NAME, TG_OP);
    END IF;
    SELECT COUNT(*) INTO n FROM patchs WHERE node_name = NEW.node_name;
    IF n > 0 THEN
        PERFORM error('NameNotUni', -1, 'node_name = ' || NEW.node_name, 'node_check_before_insert()', TG_TABLE_NAME, TG_OP);
    END IF;
    CASE TG_TABLE_NAME
        WHEN 'patchs' THEN
            NEW.node_type = '{patch}';
        WHEN 'nodes' THEN
            IF NEW.node_type IS NULL THEN
                NEW.node_type = '{node}';
            END IF;
        WHEN 'snmpdevices' THEN
            IF NEW.node_type IS NULL THEN
                NEW.node_type = '{host,snmp}';
            END IF;
        ELSE
            PERFORM error('DataError', NEW.node_id, 'node_id', 'node_check_before_insert()', TG_TABLE_NAME, TG_OP);
    END CASE;
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

\i nodeparams.sql

CREATE OR REPLACE FUNCTION delete_node_post() RETURNS TRIGGER AS $$
BEGIN
    DELETE FROM nports            WHERE node_id = OLD.node_id;
    DELETE FROM host_services     WHERE node_id = OLD.node_id;
    DELETE FROM node_params       WHERE node_id = OLD.node_id;
    RETURN OLD;
END;
$$ LANGUAGE plpgsql;

-- ///// TRIGGERS AND RULES for node_id key
-- node_id egyediség ellenörzése az összes patchs leszármazottban
CREATE TRIGGER patchs_check_node_id_before_insert               BEFORE INSERT ON patchs      FOR EACH ROW EXECUTE PROCEDURE node_check_before_insert();
CREATE TRIGGER nodes_check_node_id_before_insert                BEFORE INSERT ON nodes       FOR EACH ROW EXECUTE PROCEDURE node_check_before_insert();
CREATE TRIGGER snmpdevices_check_node_id_before_insert          BEFORE INSERT ON snmpdevices FOR EACH ROW EXECUTE PROCEDURE node_check_before_insert();
-- node_id módosításának a megelőzése az összes patchs leszármazottban
CREATE TRIGGER patchs_restrict_modfy_node_id_before_update      BEFORE UPDATE ON patchs      FOR EACH ROW EXECUTE PROCEDURE restrict_modfy_node_id_before_update();
CREATE TRIGGER nodes_restrict_modfy_node_id_before_update       BEFORE UPDATE ON nodes       FOR EACH ROW EXECUTE PROCEDURE restrict_modfy_node_id_before_update();
CREATE TRIGGER snmpdevices_restrict_modfy_node_id_before_update BEFORE UPDATE ON snmpdevices FOR EACH ROW EXECUTE PROCEDURE restrict_modfy_node_id_before_update();
-- node_id idegen kulcs hivatkozások ellenözése (létrehozás, és módosítás)
CREATE TRIGGER nports_check_reference_node_id        BEFORE UPDATE OR INSERT ON nports        FOR EACH ROW EXECUTE PROCEDURE check_reference_node_id('false', 'patchs', 'patchs');
CREATE TRIGGER interfaces_check_reference_node_id    BEFORE UPDATE OR INSERT ON interfaces    FOR EACH ROW EXECUTE PROCEDURE check_reference_node_id('false', 'nodes');
CREATE TRIGGER node_param_value_check_reference_node_id BEFORE UPDATE OR INSERT ON node_params FOR EACH ROW EXECUTE PROCEDURE check_reference_node_id('false', 'nodes');
-- Kaszkád törlések
CREATE TRIGGER patchs_delete_patch_post     AFTER DELETE ON patchs      FOR EACH ROW EXECUTE PROCEDURE delete_node_post();
CREATE TRIGGER nodes_delete_node_post       AFTER DELETE ON nodes       FOR EACH ROW EXECUTE PROCEDURE delete_node_post();
CREATE TRIGGER snmpdevices_delete_node_post AFTER DELETE ON snmpdevices FOR EACH ROW EXECUTE PROCEDURE delete_node_post();

