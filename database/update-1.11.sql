-- ADD port_tag# fields
DROP   VIEW phs_links_shape;
CREATE VIEW phs_links_shape AS
    SELECT phs_link_id, 
           port_id1,
            n1.node_id AS node_id1,
            n1.node_name AS node_name1,
            p1.port_name AS port_name1,
            p1.port_index AS port_index1,
            p1.port_tag AS port_tag1,
            n1.node_name || ':' || p1.port_name AS port_full_name1,
            phs_link_type1,
            CASE WHEN phs_link_type1 = 'Front'::phslinktype THEN port_shared
                                                            ELSE ''::portshare
            END AS port_shared1,
           port_id2,
            n2.node_id AS node_id2,
            n2.node_name AS node_name2,
            p2.port_name AS port_name2,
            p2.port_index AS port_index2,
            p2.port_tag AS port_tag2,
            n2.node_name || ':' || p2.port_name AS port_full_name2,
            phs_link_type2,
            CASE WHEN phs_link_type2 = 'Front'::phslinktype THEN port_shared
                                                            ELSE ''::portshare
            END AS port_shared2,
           phs_link_note,
           link_type,
           create_time,
           create_user_id,
           modify_time,
           modify_user_id,
           forward
    FROM phs_links JOIN ( nports AS p1 JOIN patchs AS n1 USING(node_id)) ON p1.port_id = port_id1
                   JOIN ( nports AS p2 JOIN patchs AS n2 USING(node_id)) ON p2.port_id = port_id2;
COMMENT ON VIEW phs_links_shape IS 'Symmetric View Table for physical links with shape';

ALTER TABLE app_errs  ADD COLUMN back_stack text DEFAULT NULL;
ALTER TABLE enum_vals ADD COLUMN icon       text DEFAULT NULL;
ALTER TABLE imports   ADD COLUMN exp_msg    text DEFAULT NULL; 

INSERT INTO table_shape_fields (table_shape_id, table_shape_field_name, field_sequence_number) VALUES
    (table_shape_name2id('phs_links'), 'port_tag1',   25),
    (table_shape_name2id('phs_links'), 'port_tag2',   85),
    (table_shape_name2id('app_errs'),  'back_stack', 300),
    (table_shape_name2id('enum_vals'), 'icon',       100);

DROP   VIEW mactab_shape;
CREATE VIEW mactab_shape AS 
 SELECT n.node_id,
    n.node_name,
    t.port_id,
    p.port_name,
    p.port_index,
    t.hwaddress,
    t.mactab_state,
    t.first_time,
    t.last_time,
    t.state_updated_time,
    t.set_type,
    r.node_name AS r_node_name,
    port_id2name(i.port_id) AS r_port_name,
    ARRAY( SELECT arps.ipaddress::text AS ipaddress
           FROM arps
          WHERE arps.hwaddress = t.hwaddress) AS ipaddrs_by_arp,
    ARRAY( SELECT ip_addresses.address::text AS address
           FROM ip_addresses
          WHERE ip_addresses.port_id = i.port_id) AS ipaddrs_by_rif
   FROM mactab t
     JOIN nports p USING (port_id)
     JOIN nodes  n USING (node_id)
     LEFT JOIN interfaces i USING (hwaddress)
     LEFT JOIN nodes      r ON r.node_id = i.node_id;

INSERT INTO table_shape_fields (table_shape_id, table_shape_field_name, field_sequence_number) VALUES
    (table_shape_name2id('mactab_node'), 'port_index',   25);
    
    
SELECT set_db_version(1, 11);