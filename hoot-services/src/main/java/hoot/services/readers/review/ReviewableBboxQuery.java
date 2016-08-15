/*
 * This file is part of Hootenanny.
 *
 * Hootenanny is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * --------------------------------------------------------------------
 *
 * The following copyright notices are generated automatically. If you
 * have a new notice to add, please use the format:
 * " * @copyright Copyright ..."
 * This will properly maintain the copyright information. DigitalGlobe
 * copyrights will be updated automatically.
 *
 * @copyright Copyright (C) 2015, 2016 DigitalGlobe (http://www.digitalglobe.com/)
 */
package hoot.services.readers.review;

import static com.querydsl.sql.SQLExpressions.select;
import static hoot.services.models.db.QCurrentNodes.currentNodes;
import static hoot.services.models.db.QCurrentRelationMembers.currentRelationMembers;
import static hoot.services.models.db.QCurrentWayNodes.currentWayNodes;

import java.sql.Connection;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.querydsl.core.Tuple;
import com.querydsl.sql.SQLQuery;

import hoot.services.geo.BoundingBox;
import hoot.services.models.review.ReviewQueryMapper;
import hoot.services.models.review.ReviewableItemBbox;
import hoot.services.utils.DbUtils;


/**
 * The purpose of this class is to retrieve the bounding box of all members of
 * specified relation combined recursively.
 *
 */
class ReviewableBboxQuery extends ReviewableQueryBase implements IReviewableQuery {
    private static final Logger logger = LoggerFactory.getLogger(ReviewableBboxQuery.class);

    private final Long relationId;

    ReviewableBboxQuery(Connection connection, long mapid, long relationid) {
        super(connection, mapid);
        relationId = relationid;
    }

    @Override
    public ReviewQueryMapper execQuery() {
        BoundingBox currBbox = new BoundingBox();
        ReviewableItemBbox reviewableItemBbox = new ReviewableItemBbox(currBbox, getMapId(), relationId);

        // do recursive bbox retrieval since relation may contain other relation
        List<BoundingBox> membersBboxList = new ArrayList<>();
        getRelationMembersBboxRecursive(membersBboxList, Collections.singletonList(relationId));

        for (BoundingBox bbx : membersBboxList) {
            currBbox.add(bbx);
        }

        return reviewableItemBbox;
    }

    /**
     * This function gets bbox of node and way members and add to members list.
     * If the relation contain relation member then it recurse.
     * 
     * @param membersBboxList
     *            - storage containing bbox of all member elements
     * @param relationIds
     *            - relation ids of relation members
     */
    private void getRelationMembersBboxRecursive(List<BoundingBox> membersBboxList, List<Long> relationIds) {
        for (Long relationId : relationIds) {
            BoundingBox nodeMemBbox = getRelationNodeMembersBbox(relationId);
            if (nodeMemBbox != null) {
                membersBboxList.add(nodeMemBbox);
            }

            BoundingBox wayMemBbox = getRelationWayMembersBbox(relationId);
            if (wayMemBbox != null) {
                membersBboxList.add(wayMemBbox);
            }

            List<Long> memRelationIds = getRelationMembers(relationId);
            if (!memRelationIds.isEmpty()) {
                getRelationMembersBboxRecursive(membersBboxList, memRelationIds);
            }
        }
    }

    private static boolean validateTuple(Tuple tuple) {
        boolean isValid = true;

        int columnsCount = tuple.size();
        for (int i = 0; i < columnsCount; i++) {
            Object o = tuple.get(i, Object.class);
            if (o == null) {
                isValid = false;
                break;
            }
        }

        return isValid;
    }

    /**
     * Helper function to translation Tuple to Bonding box
     * 
     * @param tup
     *            - source Tuple
     * @return - BoundingBox
     */
    private static BoundingBox resultSetToBbox(Tuple tup) {
        BoundingBox bbox = null;
        try {
            if (validateTuple(tup)) {
                double minLat = tup.get(0, Double.class);
                double maxLat = tup.get(1, Double.class);
                double minLon = tup.get(2, Double.class);
                double maxLon = tup.get(3, Double.class);

                bbox = new BoundingBox(minLon, minLat, maxLon, maxLat);
            }
        }
        catch (Exception e) {
            // we will not throw error since ret will be null and null ret
            // will be handled gracefully by caller.
            logger.error("Error during tuple to bounding box conversion!", e);
        }

        return bbox;
    }

    /**
     * Retrieves relation way member's bbox.
     * 
     * @param relationId
     *            - container relation id
     * @return - BoundingBox
     */
    private BoundingBox getRelationWayMembersBbox(long relationId) {
        BoundingBox bbox = null;

        SQLQuery<Long> currentRelationMembersSub =
                select(currentRelationMembers.memberId)
                        .from(currentRelationMembers)
                        .where(currentRelationMembers.relationId.eq(relationId)
                                .and(currentRelationMembers.memberType.eq(DbUtils.nwr_enum.way)));

        SQLQuery<Long> wayNodesSub =
                select(currentWayNodes.nodeId)
                        .from(currentWayNodes)
                        .where(currentWayNodes.wayId.in(currentRelationMembersSub));

        List<Tuple> result = new SQLQuery<>(this.getConnection(), DbUtils.getConfiguration(this.getMapId()))
                .select(currentNodes.latitude.min(),
                        currentNodes.latitude.max(),
                        currentNodes.longitude.min(),
                        currentNodes.longitude.min())
                .from(currentNodes)
                .where(currentNodes.id.in(wayNodesSub))
                .fetch();

        for (Tuple tuple : result) {
            bbox = resultSetToBbox(tuple);
        }

        return bbox;
    }

    /**
     * Retrieves relation node member's bbox.
     * 
     * @param relationId
     *            - container relation id
     * @return - BoundingBox
     */
    private BoundingBox getRelationNodeMembersBbox(long relationId) {
        SQLQuery<Long> sub =
                select(currentRelationMembers.memberId)
                .from(currentRelationMembers)
                .where(currentRelationMembers.relationId.eq(relationId)
                        .and(currentRelationMembers.memberType.eq(DbUtils.nwr_enum.node)));

        List<Tuple> result = new SQLQuery<>(this.getConnection(), DbUtils.getConfiguration(this.getMapId()))
                .select(currentNodes.latitude.min(),
                        currentNodes.latitude.max(),
                        currentNodes.longitude.min(),
                        currentNodes.longitude.min())
                .from(currentNodes)
                .where(currentNodes.id.in(sub))
                .fetch();

        BoundingBox bbox = null;
        for (Tuple row : result) {
            bbox = resultSetToBbox(row);
        }

        return bbox;
    }

    /**
     * Retrieves all relation members of a relation.
     * 
     * @param relationId
     *            - container relation id
     * @return - List of relation member ids
     */
    private List<Long> getRelationMembers(long relationId) {
        List<Long> relationMemberIds = new SQLQuery<>(this.getConnection(), DbUtils.getConfiguration(this.getMapId()))
                .select(currentRelationMembers.memberId)
                .from(currentRelationMembers)
                .where(currentRelationMembers.relationId.eq(relationId)
                        .and(currentRelationMembers.memberType.eq(DbUtils.nwr_enum.relation)))
                .fetch();

        return relationMemberIds;
    }
}
