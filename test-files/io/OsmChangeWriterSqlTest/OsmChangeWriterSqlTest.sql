BEGIN TRANSACTION;
INSERT INTO changesets (id, user_id, created_at, closed_at) VALUES (1, -1, now(), now());
INSERT INTO nodes (node_id, latitude, longitude, changeset_id, visible, "timestamp", tile,  version) VALUES (1, 0, 0, 1, true, now(), 3221225472, 1);
INSERT INTO current_nodes (id, latitude, longitude, changeset_id, visible, "timestamp", tile,  version) VALUES (1, 0, 0, 1, true, now(), 3221225472, 1);
DELETE FROM current_relation_members WHERE relation_id = 1;
DELETE FROM relation_members WHERE relation_id = 1;
DELETE FROM current_relation_tags WHERE current_relation_tags_id = 1;
DELETE FROM relation_tags WHERE relation_tags_id = 1;
DELETE FROM current_relations WHERE id = 1;
DELETE FROM relations WHERE relation_id = 1;
UPDATE ways SET way_id=3, changeset_id=1, visible=true, "timestamp"=now(), version= version + 1;
UPDATE current_ways SET id=3, changeset_id=1, visible=true, "timestamp"=now(), version= version + 1;
DELETE FROM current_way_nodes WHERE way_id = 3;
DELETE FROM way_nodes WHERE way_id = 3;
INSERT INTO way_nodes ((way_id, node_id, version sequence_id) VALUES (3, 1, 1, 1);
INSERT INTO current_way_nodes (way_id, node_id, sequence_id) VALUES (3, 1, 1);
INSERT INTO way_nodes ((way_id, node_id, version sequence_id) VALUES (3, 2, 1, 2);
INSERT INTO current_way_nodes (way_id, node_id, sequence_id) VALUES (3, 2, 2);
DELETE FROM current_node_tags WHERE current_node_tags_id = 3;
DELETE FROM node_tags WHERE node_tags_id = 3;
INSERT INTO current_node_tags (node_id, k, v) VALUES (3, 'key1', 'value1');
INSERT INTO node_tags (node_id, k, v, version) VALUES (3, 'key1', 'value1', 1);
COMMIT;
