-- Javítások... 2021.12.23.


CREATE OR REPLACE FUNCTION place_id2category(bigint) RETURNS text
    LANGUAGE SQL STABLE PARALLEL SAFE AS
$$
    SELECT array_to_string(ARRAY(SELECT place_group_name FROM place_groups JOIN place_group_places USING(place_group_id) WHERE place_id = $1 AND  place_group_type = 'category'),',');
$$;


DROP VIEW public.phs_links_shape;
DROP FUNCTION public.shared_cable(pid bigint, pref text);

CREATE OR REPLACE FUNCTION public.shared_cable(pid bigint, pref text DEFAULT ''::text) RETURNS text
    LANGUAGE SQL STABLE PARALLEL SAFE AS
$$
    SELECT 
        CASE 
            WHEN shared_port_id IS NULL AND shared_cable = ''::portshare THEN ''
            WHEN shared_port_id IS NULL THEN pref || shared_cable::text
            ELSE pref || shared_cable::text || '(' || port_id2name(shared_port_id) || ')'
        END
    FROM pports WHERE port_id = pid;
$$;

CREATE OR REPLACE FUNCTION public.shared_cable_back(pid bigint) RETURNS text
    LANGUAGE SQL STABLE PARALLEL SAFE AS
$$
    WITH pps AS (
        SELECT
            pp.shared_port_id AS pp_shp_id,
            pp.shared_cable   AS pp_sc,
            sp.shared_cable || ':' || sp.port_name AS sp_sp
        FROM pports AS pp
        LEFT JOIN pports AS sp ON sp.shared_port_id = pp.port_id
        WHERE pp.port_id = pid
    )
    SELECT
        CASE
            WHEN pp_shp_id IS NULL AND pp_sc = ''::portshare THEN
                ''
            WHEN pp_shp_id IS NULL AND array_length(array_agg(sp_sp), 1) > 0 THEN
                pp_sc::text || ' / ' || array_to_string(array_agg(sp_sp), '; ')
            WHEN pp_shp_id IS NULL AND array_length(array_agg(sp_sp), 1) = 0 THEN
                pp_sc::text
            ELSE
                pp_sc::text || '!!(' || port_id2name(pp_shp_id) || ')'
        END
    FROM pps
    GROUP BY pp_shp_id, pp_sc
$$;


CREATE OR REPLACE VIEW public.phs_links_shape
 AS
 SELECT
    l.phs_link_id,
    l.port_id1,
    n1.node_id AS node_id1,
    n1.node_name AS node_name1,
    CASE
        WHEN l.phs_link_type1 = 'Front'::phslinktype THEN p1.port_name || shared_cable(l.port_id1, ' / '::text)
        ELSE p1.port_name
    END AS port_name1,
    p1.port_index AS port_index1,
    p1.port_tag AS port_tag1,
    (n1.node_name || ':'::text) || p1.port_name AS port_full_name1,
    l.phs_link_type1,
        CASE
            WHEN l.phs_link_type1 = 'Front'::phslinktype THEN l.port_shared::text
            WHEN l.phs_link_type1 = 'Back'::phslinktype THEN shared_cable_back(l.port_id1)
            ELSE ''::text
        END AS port_shared1,
    l.port_id2,
    n2.node_id AS node_id2,
    n2.node_name AS node_name2,
        CASE
            WHEN l.phs_link_type2 = 'Front'::phslinktype THEN p2.port_name || shared_cable(l.port_id2, ' / '::text)
            ELSE p2.port_name
        END AS port_name2,
    p2.port_index AS port_index2,
    p2.port_tag AS port_tag2,
    (n2.node_name || ':'::text) || p2.port_name AS port_full_name2,
    l.phs_link_type2,
        CASE
            WHEN l.phs_link_type2 = 'Front'::phslinktype THEN l.port_shared::text
            WHEN l.phs_link_type2 = 'Back'::phslinktype THEN shared_cable_back(l.port_id2)
            ELSE ''::text
        END AS port_shared2,
    l.phs_link_note,
    l.link_type,
    l.create_time,
    l.create_user_id,
    l.forward
   FROM phs_links l
     JOIN (nports p1
     JOIN patchs n1 USING (node_id)) ON p1.port_id = l.port_id1
     JOIN (nports p2
     JOIN patchs n2 USING (node_id)) ON p2.port_id = l.port_id2;

ALTER TABLE public.phs_links_shape
    OWNER TO lanview2;


