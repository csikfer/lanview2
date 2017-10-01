--
-- PostgreSQL database dump
--

SET statement_timeout = 0;
SET lock_timeout = 0;
SET client_encoding = 'UTF8';
SET standard_conforming_strings = on;
SET check_function_bodies = false;
SET client_min_messages = warning;

SET search_path = "public", pg_catalog;

--
-- Data for Name: selects; Type: TABLE DATA; Schema: public; Owner: -
--

INSERT INTO "selects" VALUES (1, 'lldp.descr', 'HP ProCurve 1700', 10, 'PROCURVE J%V(A|B).0%', 'similar', 'ProCurveWeb', NULL);
INSERT INTO "selects" VALUES (2, 'lldp.descr', 'HP ProCurve 1800', 20, 'PROCURVE J%P(A|B).0%', 'similar', 'ProCurveWeb', NULL);
INSERT INTO "selects" VALUES (4, 'lldp.descr', 'HP ProCurve', 40, 'HP J%, revision [KW]%', 'similar', 'ProCurve', NULL);
INSERT INTO "selects" VALUES (6, 'lldp.descr', 'Cisco', 60, 'Cisco IOS Software%', 'similar', 'Cisco', NULL);
INSERT INTO "selects" VALUES (7, 'lldp.descr', 'HP AP Controlled', 70, '%HP AP Controlled%', 'similar', 'HPAPC', NULL);
INSERT INTO "selects" VALUES (8, 'lldp.descr', 'Linux', 80, '%Linux%', 'similar', 'Linux', NULL);
INSERT INTO "selects" VALUES (3, 'lldp.descr', 'HP ProCurve old', 30, 'ProCurve J%, revision [HRTNKE]%', 'similar', 'ProCurve', NULL);
INSERT INTO "selects" VALUES (5, 'lldp.descr', 'HP HPE 1910', 50, 'HPE? V1910%', 'similar', '3COM', NULL);


--
-- Name: selects_select_id_seq; Type: SEQUENCE SET; Schema: public; Owner: -
--

SELECT pg_catalog.setval('"selects_select_id_seq"', 8, true);


--
-- PostgreSQL database dump complete
--

