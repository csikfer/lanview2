ALTER TYPE fieldflag ADD VALUE 'HTML';


INSERT INTO port_params (param_type_id, port_id, param_value)
    (SELECT public.param_type_name2id('query_mac_tab'), si.port_id, 'true' 
        FROM lldp_links AS ll
        JOIN interfaces AS si ON ll.port_id1 = si.port_id
        JOIN interfaces AS wi ON ll.port_id2 = wi.port_id
        JOIN nodes      AS s  ON si.node_id = s.node_id
        JOIN nodes      AS w  ON wi.node_id = w.node_id
        WHERE 'switch'      = ANY (s.node_type)
          AND 'workstation' = ANY (w.node_type) )
    ON CONFLICT DO NOTHING;