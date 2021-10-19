
DROP VIEW portvars;	-- A megjelenítés nem a view-en keresztül lesz megoldva.

CREATE OR REPLACE FUNCTION port_id2node_id(bigint) RETURNS bigint LANGUAGE SQL STABLE PARALLEL SAFE AS $$
    SELECT node_id FROM nports WHERE port_id = $1;
$$;

CREATE OR REPLACE FUNCTION is_port_of_node(bigint, bigint) RETURNS boolean LANGUAGE SQL STABLE PARALLEL SAFE AS $$
    SELECT 0 < COUNT(*) FROM nports WHERE port_id = $1 AND node_id = $2;
$$;

CREATE OR REPLACE FUNCTION is_port_of_host_service(bigint, bigint) RETURNS boolean LANGUAGE SQL STABLE PARALLEL SAFE AS $$
    SELECT 0 < COUNT(*) FROM host_services WHERE port_id = $1 AND host_service_id = $2;
$$;

CREATE OR REPLACE FUNCTION is_port_of_service_var(bigint, bigint) RETURNS boolean LANGUAGE SQL STABLE PARALLEL SAFE AS $$
    SELECT 0 < COUNT(*) FROM service_vars JOIN host_services USING(host_service_id) WHERE port_id = $1 AND service_var_id = $2;
$$;

ALTER TYPE tooltype ADD VALUE 'terminal';

SELECT set_db_version(1, 30);
