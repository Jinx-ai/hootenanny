#!/bin/bash
set -e

# This test is only meant to be run --with-rnd and hoot-rnd doesn't run its own test suite.  
# So, I'm checking to see if the command is in the list of hoot commands and exiting if it 
# doesn't...there may be a better way to handle this.  This will throw off the test count, unfortunately.
OPT_COMMAND=`hoot | grep multiary-ingest`
#echo $OPT_COMMAND
if [ -z "$OPT_COMMAND" ]; then
  #echo "multiary-ingest command not present.  Exiting test without running it."
 exit 0
fi

REF_DIR=test-files/cmd/slow/ServiceMultiaryIngestCmdTest
OUTPUT_DIR=test-output/cmd/slow/ServiceMultiaryIngestCmdTest

rm -rf $OUTPUT_DIR
mkdir -p $OUTPUT_DIR

source conf/database/DatabaseConfig.sh
HOOT_DB_URL="hootapidb://$DB_USER:$DB_PASSWORD@$DB_HOST:$DB_PORT/$DB_NAME"
HOOT_OPTS="--warn -D uuid.helper.repeatable=true -D reader.add.source.datetime=false -D writer.include.circular.error.tags=false -D api.db.email=OsmApiDbHootApiDbConflate@hoottestcpp.org"

# geonames

REFERENCE_INPUT=$REF_DIR/allCountries-11-18-13-10.geonames
NEW_INPUT=$REF_DIR/allCountries-8-15-17-10.geonames
FINAL_OUTPUT=$OUTPUT_DIR/allCountries-geonames-output.osm
CHANGESET_OUTPUT=$OUTPUT_DIR/allCountries-geonames-changeset.spark.1
GOLD_OUTPUT=$REF_DIR/allCountries-geonames-output.osm
GOLD_CHANGESET=$REF_DIR/allCountries-geonames-changeset.spark.1

echo ""
echo "MULTIARY INGEST - DELETING GEONAMES REFERENCE LAYER..."
echo ""
hoot delete-map $HOOT_OPTS "$HOOT_DB_URL/MultiaryIngest-ReferenceLayer"

# first time ref layer write
echo ""
echo "MULTIARY INGEST - INGESTING GEONAMES DATASET 1..."
echo ""
hoot multiary-ingest $HOOT_OPTS $REFERENCE_INPUT "$HOOT_DB_URL/MultiaryIngest-ReferenceLayer" $CHANGESET_OUTPUT true

# ingest of new data, compare, and write to existing data
echo ""
echo "MULTIARY INGEST - INGESTING GEONAMES DATASET 2..."
echo ""
hoot multiary-ingest $HOOT_OPTS $NEW_INPUT "$HOOT_DB_URL/MultiaryIngest-ReferenceLayer" $CHANGESET_OUTPUT true

echo ""
echo "MULTIARY INGEST - EXPORTING GEONAMES REFERENCE LAYER..."
echo ""
hoot convert $HOOT_OPTS "$HOOT_DB_URL/MultiaryIngest-ReferenceLayer" $FINAL_OUTPUT

echo ""
echo "MULTIARY INGEST - COMPARING GEONAMES REFERENCE LAYER OUTPUT..."
echo ""
hoot is-match $HOOT_OPTS $GOLD_OUTPUT $FINAL_OUTPUT 

echo ""
echo "MULTIARY INGEST - COMPARING GEONAMES CHANGESET OUTPUT..."
echo ""
diff $GOLD_CHANGESET $CHANGESET_OUTPUT

# osm pbf

REFERENCE_INPUT=$REF_DIR/allCountries-11-18-13-10.osm.pbf
NEW_INPUT=$REF_DIR/allCountries-8-15-17-10.osm.pbf
FINAL_OUTPUT=$OUTPUT_DIR/allCountries-osmpbf-output.osm
CHANGESET_OUTPUT=$OUTPUT_DIR/allCountries-osmpbf-changeset.spark.1
GOLD_OUTPUT=$REF_DIR/allCountries-osmpbf-output.osm
GOLD_CHANGESET=$REF_DIR/allCountries-osmpbf-changeset.spark.1

echo ""
echo "MULTIARY INGEST - DELETING OSM PBF REFERENCE LAYER..."
echo ""
hoot delete-map $HOOT_OPTS "$HOOT_DB_URL/MultiaryIngest-ReferenceLayer"

# first time ref layer write
echo ""
echo "MULTIARY INGEST - INGESTING OSM PBF DATASET 1..."
echo ""
hoot multiary-ingest $HOOT_OPTS $REFERENCE_INPUT "$HOOT_DB_URL/MultiaryIngest-ReferenceLayer" $CHANGESET_OUTPUT true

# ingest of new data, compare, and write to existing data
echo ""
echo "MULTIARY INGEST - INGESTING OSM PBF DATASET 2..."
echo ""
hoot multiary-ingest $HOOT_OPTS $NEW_INPUT "$HOOT_DB_URL/MultiaryIngest-ReferenceLayer" $CHANGESET_OUTPUT true

echo ""
echo "MULTIARY INGEST - EXPORTING OSM PBF REFERENCE LAYER..."
echo ""
hoot convert $HOOT_OPTS "$HOOT_DB_URL/MultiaryIngest-ReferenceLayer" $FINAL_OUTPUT

echo ""
echo "MULTIARY INGEST - COMPARING OSM PBF REFERENCE LAYER OUTPUT..."
echo ""
hoot is-match $HOOT_OPTS $GOLD_OUTPUT $FINAL_OUTPUT 

echo ""
echo "MULTIARY INGEST - COMPARING OSM PBF CHANGESET OUTPUT..."
echo ""
diff $GOLD_CHANGESET $CHANGESET_OUTPUT

