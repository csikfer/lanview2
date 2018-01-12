-- bugfixes
-- bugfixes end

CREATE OR REPLACE FUNCTION cast_to_bigint(text, bigint DEFAULT NULL) RETURNS bigint AS $$
BEGIN
    RETURN cast($1 as bigint);
EXCEPTION
    WHEN invalid_text_representation THEN
        RETURN $2;
END;
$$ LANGUAGE plpgsql IMMUTABLE;

CREATE OR REPLACE FUNCTION cast_to_double(text, double precision DEFAULT NULL) RETURNS double precision AS $$
BEGIN
    RETURN cast($1 as double precision);
EXCEPTION
    WHEN invalid_text_representation THEN
        RETURN $2;
END;
$$ LANGUAGE plpgsql IMMUTABLE;

CREATE OR REPLACE FUNCTION cast_to_date(text, date DEFAULT NULL) RETURNS date AS $$
BEGIN
    RETURN cast($1 as date);
EXCEPTION
    WHEN invalid_text_representation THEN
        RETURN $2;
END;
$$ LANGUAGE plpgsql IMMUTABLE;

CREATE OR REPLACE FUNCTION cast_to_time(text, time DEFAULT NULL) RETURNS time AS $$
BEGIN
    RETURN cast($1 as time);
EXCEPTION
    WHEN invalid_text_representation THEN
        RETURN $2;
END;
$$ LANGUAGE plpgsql IMMUTABLE;

CREATE OR REPLACE FUNCTION cast_to_datetime(text, timestamp DEFAULT NULL) RETURNS timestamp AS $$
BEGIN
    RETURN cast($1 as timestamp);
EXCEPTION
    WHEN invalid_text_representation THEN
        RETURN $2;
END;
$$ LANGUAGE plpgsql IMMUTABLE;

CREATE OR REPLACE FUNCTION cast_to_interval(text, interval DEFAULT NULL) RETURNS interval AS $$
BEGIN
    RETURN cast($1 as interval);
EXCEPTION
    WHEN invalid_text_representation THEN
        RETURN $2;
END;
$$ LANGUAGE plpgsql IMMUTABLE;

-- SELECT set_db_version(1, 9);
