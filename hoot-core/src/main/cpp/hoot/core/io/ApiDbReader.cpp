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
 * @copyright Copyright (C) 2016, 2017 DigitalGlobe (http://www.digitalglobe.com/)
 */
#include "ApiDbReader.h"

// Hoot
#include <hoot/core/util/GeometryUtils.h>
#include <hoot/core/util/MetadataTags.h>
#include <hoot/core/io/TableType.h>
#include <hoot/core/io/ApiDb.h>
#include <hoot/core/util/Log.h>
#include <hoot/core/util/FileUtils.h>

// Qt
#include <QSet>

using namespace geos::geom;

namespace hoot
{

unsigned int ApiDbReader::logWarnCount = 0;

ApiDbReader::ApiDbReader() :
_useDataSourceIds(true),
_status(Status::Invalid),
_open(false),
_sortById(false),
_returnNodesOnly(false)
{
}

bool ApiDbReader::isSupported(QString urlStr)
{
  QUrl url(urlStr);
  return _getDatabase()->isSupported(url);
}

void ApiDbReader::setBoundingBox(const QString bbox)
{
  if (!bbox.trimmed().isEmpty())
  {
    setBounds(GeometryUtils::envelopeFromConfigString(bbox));
  }
}

void ApiDbReader::setOverrideBoundingBox(const QString bbox)
{
  if (!bbox.trimmed().isEmpty())
  {
    _overrideBounds = GeometryUtils::envelopeFromConfigString(bbox);
  }
}

bool ApiDbReader::_hasBounds()
{
  return _isValidBounds(_bounds) || _isValidBounds(_overrideBounds);
}

ElementId ApiDbReader::_mapElementId(const OsmMap& map, ElementId oldId)
{
  LOG_TRACE("Mapping element ID: " << oldId << "...");

  ElementId result;
  LOG_VART(oldId);
  LOG_VART(_useDataSourceIds);
  if (_useDataSourceIds)
  {
    result = oldId;
  }
  else
  {
    long id = oldId.getId();
    switch (oldId.getType().getEnum())
    {
    case ElementType::Node:
      if (_nodeIdMap.count(id) > 0)
      {
        result = ElementId::node(_nodeIdMap.at(id));
        LOG_VART(result);
      }
      else
      {
        long newId = map.createNextNodeId();
        _nodeIdMap[id] = newId;
        result = ElementId::node(newId);
        LOG_VART(result);
      }
      break;
    case ElementType::Way:
      if (_wayIdMap.count(id) > 0)
      {
        result = ElementId::way(_wayIdMap.at(id));
        LOG_VART(result);
      }
      else
      {
        long newId = map.createNextWayId();
        _wayIdMap[id] = newId;
        result = ElementId::way(newId);
        LOG_VART(result);
      }
      break;
    case ElementType::Relation:
      if (_relationIdMap.count(id) > 0)
      {
        result = ElementId::relation(_relationIdMap.at(id));
        LOG_VART(result);
      }
      else
      {
        long newId = map.createNextRelationId();
        _relationIdMap[id] = newId;
        result = ElementId::relation(newId);
        LOG_VART(result);
      }
      break;
    default:
      throw IllegalArgumentException("Expected a valid element type, but got: " +
        QString::number(oldId.getType().getEnum()));
    }
  }

  LOG_TRACE("Mapped element ID: " << oldId << " to ID: " << result << "...");

  return result;
}

void ApiDbReader::_updateMetadataOnElement(ElementPtr element)
{
  LOG_TRACE("Updating metadata on element " << element->getElementId() << "...");

  bool ok;
  Tags& tags = element->getTags();

  if (tags.contains(MetadataTags::HootStatus()))
  {
    QString statusStr = tags.get(MetadataTags::HootStatus());
    bool ok;
    const int statusInt = statusStr.toInt(&ok);
    Status status = static_cast<Status::Type>(statusInt);
    if (ok && status.getEnum() >= Status::Invalid && status.getEnum() <= Status::Conflated)
    {
      element->setStatus(status);
    }
    else
    {
      try
      {
        //  Try parsing the status in text form
        element->setStatus(Status::fromString(statusStr));
      }
      catch (const HootException&)
      {
        if (logWarnCount < ConfigOptions().getLogWarnMessageLimit())
        {
          LOG_WARN("Invalid status: " + statusStr + " for element with ID: " +
                   QString::number(element->getId()));
        }
        else if (logWarnCount == ConfigOptions().getLogWarnMessageLimit())
        {
          LOG_WARN(className() << ": " << Log::LOG_WARN_LIMIT_REACHED_MESSAGE);
        }
        logWarnCount++;
      }
    }
    //We don't need to carry this tag around once the value is set on the element...it will
    //be reinstated by some writers, though.
    if (!ConfigOptions().getReaderKeepFileStatus()) { tags.remove(MetadataTags::HootStatus()); }
  }

  if (tags.contains("type"))
  {
    Relation* r = dynamic_cast<Relation*>(element.get());
    if (r)
    {
      r->setType(tags["type"]);
      //I don't think OSM non-hoot metadata tags should be removed here...
      //tags.remove("type");
    }
  }

  if (tags.contains(MetadataTags::ErrorCircular()))
  {
    element->setCircularError(tags.get(MetadataTags::ErrorCircular()).toDouble(&ok));
    if (!ok)
    {
      try
      {
        double tv = tags.getLength(MetadataTags::ErrorCircular()).value();
        element->setCircularError(tv);
        ok = true;
      }
      catch (const HootException&)
      {
        ok = false;
      }

      if (!ok)
      {
        if (logWarnCount < ConfigOptions().getLogWarnMessageLimit())
        {
          LOG_WARN("Error parsing " + MetadataTags::ErrorCircular() + ".");
        }
        else if (logWarnCount == ConfigOptions().getLogWarnMessageLimit())
        {
          LOG_WARN(className() << ": " << Log::LOG_WARN_LIMIT_REACHED_MESSAGE);
        }
        logWarnCount++;
      }
    }
    //We don't need to carry this tag around once the value is set on the element...it will
    //be reinstated by some writers, though.
    tags.remove(MetadataTags::ErrorCircular());
  }
  else if (tags.contains(MetadataTags::Accuracy()))
  {
    element->setCircularError(tags.get(MetadataTags::Accuracy()).toDouble(&ok));

    if (!ok)
    {
      try
      {
        double tv = tags.getLength(MetadataTags::Accuracy()).value();
        element->setCircularError(tv);
        ok = true;
      }
      catch (const HootException&)
      {
        ok = false;
      }

      if (!ok)
      {
        if (logWarnCount < ConfigOptions().getLogWarnMessageLimit())
        {
          LOG_WARN("Error parsing " + MetadataTags::Accuracy() + ".");
        }
        else if (logWarnCount == ConfigOptions().getLogWarnMessageLimit())
        {
          LOG_WARN(className() << ": " << Log::LOG_WARN_LIMIT_REACHED_MESSAGE);
        }
        logWarnCount++;
      }
    }
    //I don't think OSM non-hoot metadata tags should be removed here...
    //tags.remove(MetadataTags::Accuracy());
  }
}

bool ApiDbReader::_isValidBounds(const Envelope& bounds)
{
  if (bounds.isNull() ||
      (bounds.getMinX() == -180.0 && bounds.getMinY() == -90.0 && bounds.getMaxX() == 180.0 &&
       bounds.getMaxY() == 90.0))
  {
    return false;
  }
  return true;
}

void ApiDbReader::_readByBounds(OsmMapPtr map, const Envelope& bounds)
{
  long boundedNodeCount = 0;
  long boundedWayCount = 0;
  long boundedRelationCount = 0;

  LOG_DEBUG("Retrieving node records within the query bounds...");
  boost::shared_ptr<QSqlQuery> nodeItr = _getDatabase()->selectNodesByBounds(bounds);
  QSet<QString> nodeIds;
  while (nodeItr->next())
  {
    const QSqlQuery resultIterator = *nodeItr;
    NodePtr node = _resultToNode(resultIterator, *map);
    LOG_VART(node);
    map->addElement(node);
    boundedNodeCount++;
    //Don't use the mapped id from the node object here, b/c we want don't want to use mapped ids
    //with any queries.  Mapped ids may not exist yet.
    const long nodeId = resultIterator.value(0).toLongLong();
    LOG_VART(ElementId(ElementType::Node, nodeId));
    nodeIds.insert( QString::number(nodeId));
  }
  LOG_VARD(nodeIds.size());

  if (!_returnNodesOnly)
  {
    if (nodeIds.size() > 0)
    {
      LOG_DEBUG("Retrieving way IDs referenced by the selected nodes...");
      QSet<QString> wayIds;
      boost::shared_ptr<QSqlQuery> wayIdItr = _getDatabase()->selectWayIdsByWayNodeIds(nodeIds);
      while (wayIdItr->next())
      {
        const long wayId = (*wayIdItr).value(0).toLongLong();
        LOG_VART(ElementId(ElementType::Way, wayId));
        wayIds.insert(QString::number(wayId));
      }
      LOG_VARD(wayIds.size());

      if (wayIds.size() > 0)
      {
        LOG_DEBUG("Retrieving ways by way ID...");
        boost::shared_ptr<QSqlQuery> wayItr =
          _getDatabase()->selectElementsByElementIdList(wayIds, TableType::Way);
        while (wayItr->next())
        {
          //I'm a little confused why this wouldn't cause a problem in that you could be writing ways
          //to the map here whose nodes haven't yet been written to the map yet.  Haven't encountered
          //the problem yet with test data, but will continue to keep an eye on it.
          WayPtr way = _resultToWay(*wayItr, *map);
          map->addElement(way);
          LOG_VART(way);
          boundedWayCount++;
        }

        LOG_DEBUG("Retrieving way node IDs referenced by the selected ways...");
        QSet<QString> additionalWayNodeIds;
        boost::shared_ptr<QSqlQuery> additionalWayNodeIdItr =
          _getDatabase()->selectWayNodeIdsByWayIds(wayIds);
        while (additionalWayNodeIdItr->next())
        {
          const long nodeId = (*additionalWayNodeIdItr).value(0).toLongLong();
          LOG_VART(ElementId(ElementType::Node, nodeId));
          additionalWayNodeIds.insert(QString::number(nodeId));
        }

        //subtract nodeIds from additionalWayNodeIds so no dupes get added
        LOG_VARD(nodeIds.size());
        LOG_VARD(additionalWayNodeIds.size());
        additionalWayNodeIds = additionalWayNodeIds.subtract(nodeIds);
        LOG_VARD(additionalWayNodeIds.size());

        if (additionalWayNodeIds.size() > 0)
        {
          nodeIds.unite(additionalWayNodeIds);
          LOG_DEBUG(
            "Retrieving nodes falling outside of the query bounds but belonging to a selected way...");
          boost::shared_ptr<QSqlQuery> additionalWayNodeItr =
            _getDatabase()->selectElementsByElementIdList(additionalWayNodeIds, TableType::Node);
          while (additionalWayNodeItr->next())
          {
            NodePtr node = _resultToNode(*additionalWayNodeItr, *map);
            map->addElement(node);
            LOG_VART(node);
            boundedNodeCount++;
          }
        }
      }

      LOG_DEBUG("Retrieving relation IDs referenced by the selected ways and nodes...");
      QSet<QString> relationIds;
      assert(nodeIds.size() > 0);
      boost::shared_ptr<QSqlQuery> relationIdItr =
        _getDatabase()->selectRelationIdsByMemberIds(nodeIds, ElementType::Node);
      while (relationIdItr->next())
      {
        const long relationId = (*relationIdItr).value(0).toLongLong();
        LOG_VART(ElementId(ElementType::Relation, relationId));
        relationIds.insert(QString::number(relationId));
      }
      if (wayIds.size() > 0)
      {
        relationIdItr = _getDatabase()->selectRelationIdsByMemberIds(wayIds, ElementType::Way);
        while (relationIdItr->next())
        {
          const long relationId = (*relationIdItr).value(0).toLongLong();
          LOG_VART(ElementId(ElementType::Relation, relationId));
          relationIds.insert(QString::number(relationId));
        }
      }
      LOG_VARD(relationIds.size());

      if (relationIds.size() > 0)
      {
        LOG_DEBUG("Retrieving relations by relation ID...");
        boost::shared_ptr<QSqlQuery> relationItr =
          _getDatabase()->selectElementsByElementIdList(relationIds, TableType::Relation);
        while (relationItr->next())
        {
          RelationPtr relation = _resultToRelation(*relationItr, *map);
          map->addElement(relation);
          LOG_VART(relation);
          boundedRelationCount++;
        }
      }
    }
  }

  LOG_DEBUG(
    "Bounded query read " << (boundedNodeCount + boundedWayCount + boundedRelationCount) <<
    " total elements.");
  LOG_VARD(boundedNodeCount);
  LOG_VARD(boundedWayCount);
  LOG_VARD(boundedRelationCount);
  LOG_DEBUG("Current map:");
  LOG_VARD(map->getNodes().size());
  LOG_VARD(map->getWays().size());
  LOG_VARD(map->getRelations().size());
}

void ApiDbReader::initializePartial()
{
  _partialMap.reset(new OsmMap());

  _elementResultIterator.reset();
  _firstPartialReadCompleted = false;
  _elementsRead = 0;

  _nodeIndex = 0;
  _totalNumMapNodes = 0;
  _wayIndex = 0;
  _totalNumMapWays = 0;
  _relationIndex = 0;
  _totalNumMapRelations = 0;
}

void ApiDbReader::read(OsmMapPtr map)
{
  if (!_hasBounds())
  {
    LOG_DEBUG("Executing API read query...");
    for (int ctr = ElementType::Node; ctr != ElementType::Unknown; ctr++)
    {
      if (_returnNodesOnly && ctr != ElementType::Node)
      {
        break;
      }
      _read(map, static_cast<ElementType::Type>(ctr));
    }
  }
  else
  {
    Envelope bounds;
    if (!_overrideBounds.isNull())
    {
      bounds = _overrideBounds;
    }
    else
    {
      bounds = _bounds;
    }
    LOG_DEBUG("Executing API bounded read query with bounds " << bounds.toString() << "...");
    _readByBounds(map, bounds);
  }
}

void ApiDbReader::_read(OsmMapPtr map, const ElementType& elementType)
{
  //This method could possibly be replaced by the _readBounds method set to a global extent.

  long elementCount = 0;

  // contact the DB and select all
  boost::shared_ptr<QSqlQuery> elementResultsIterator =
    _getDatabase()->selectAllElements(elementType, _sortById);

  //need to check isActive, rather than next() here b/c resultToElement actually calls next() and
  //it will always return an extra null node at the end (see comments in _resultToElement)
  while (elementResultsIterator->isActive())
  {
    boost::shared_ptr<Element> element =
      _resultToElement(*elementResultsIterator, elementType, *map );
    //this check is necessary due to an inefficiency in _resultToElement
    if (element.get())
    {
      map->addElement(element);
      LOG_VART(element);
      elementCount++;
    }
  }

  LOG_DEBUG(
    "Select all query read " << elementCount << " " << elementType.toString() << " elements.");
  LOG_VARD(map->getNodes().size());
  LOG_VARD(map->getWays().size());
  LOG_VARD(map->getRelations().size());
}

long ApiDbReader::_numElementsRead() const
{
  return _nodeIndex + _wayIndex + _relationIndex;
}

long ApiDbReader::_numElementsTotal() const
{
  return _totalNumMapNodes + _totalNumMapWays + _totalNumMapRelations;
}

bool ApiDbReader::hasMoreElements()
{
  if (!_firstPartialReadCompleted)
  {
    //get the total element counts before beginning results parsing
    _totalNumMapNodes = _getDatabase()->numElements(ElementType::Node);
    _totalNumMapWays = _getDatabase()->numElements(ElementType::Way);
    _totalNumMapRelations = _getDatabase()->numElements(ElementType::Relation);

    LOG_INFO(
      "Reading dataset with " << FileUtils::formatPotentiallyLargeNumber(_totalNumMapNodes) <<
      " nodes, " << FileUtils::formatPotentiallyLargeNumber(_totalNumMapWays) << " ways, and " <<
      FileUtils::formatPotentiallyLargeNumber(_totalNumMapRelations) << " relations...");

    _firstPartialReadCompleted = true;
  }

  //LOG_VART(_numElementsRead());
  //LOG_VART(_numElementsTotal());
  assert(_numElementsRead() <= _numElementsTotal());
  //each results index is 0 based, so as soon as the sum of indexes is equal to the total number of
  //elements, we're done iterating through them
  return (_numElementsRead() < _numElementsTotal());
}

boost::shared_ptr<Element> ApiDbReader::readNextElement()
{
  if (!hasMoreElements())
  {
    throw HootException("No more elements available to read from map");
  }

  ElementType selectElementType = _getCurrentSelectElementType();
  LOG_VART(selectElementType);
  //see if another result is available
  if (!_elementResultIterator.get() || !_elementResultIterator->isActive())
  {
    //no results available, so request some more results
    LOG_DEBUG("Requesting " << _maxElementsPerMap << " more query results...");
    _elementResultIterator =
      _getDatabase()->selectElements(
        selectElementType, _sortById, _maxElementsPerMap,
        _getCurrentElementOffset(selectElementType));
  }

  //results still available, so keep parsing through them
  boost::shared_ptr<Element> element =
    _resultToElement(*_elementResultIterator, selectElementType, *_partialMap);
  LOG_VART(element.get());
  if (!element.get())
  {
    //exceptional case to deal with having to call QSqlQuery::next() inside of
    //ServicesDb::resultToElement rather than from inside this method
    _elementResultIterator.reset();
    return boost::shared_ptr<Element>();
  }

  _incrementElementIndex(selectElementType);
  LOG_VART(selectElementType);
  LOG_VART(element->getElementType());
  assert(selectElementType == element->getElementType());
  _elementsRead++;
  LOG_VART(_elementsRead);
  LOG_VART(_maxElementsPerMap);
  if (_elementsRead == _maxElementsPerMap)
  {
    _elementResultIterator->finish();
    LOG_DEBUG("Closed result iterator.");
  }
  LOG_VART(element->getElementId());

//  if (Log::getInstance().getLevel() <= Log::Debug &&
//      (_numElementsTotal() >= 10 || (_numElementsRead() % (_maxElementsPerMap / 10) == 0)))
//  {
//    PROGRESS_DEBUG("Read: " << _numElementsRead() << " \ " << _numElementsTotal());
//  }

  return element;
}

const ElementType ApiDbReader::_getCurrentSelectElementType() const
{
  ElementType selectElementType;
  if (_nodeIndex < _totalNumMapNodes)
  {
    selectElementType = ElementType::Node;
  }
  else if (_wayIndex < _totalNumMapWays)
  {
    selectElementType = ElementType::Way;
  }
  else if (_relationIndex < _totalNumMapRelations)
  {
    selectElementType = ElementType::Relation;
  }
  else
  {
    //hasMoreElements should have kept us from being here
    throw HootException("_getCurrentSelectElementType");
  }
  return selectElementType;
}

long ApiDbReader::_getCurrentElementOffset(const ElementType& selectElementType) const
{
  long selectOffset = -1;
  if (selectElementType == ElementType::Node)
  {
    selectOffset = _nodeIndex;
  }
  else if (selectElementType == ElementType::Way)
  {
    selectOffset = _wayIndex;
  }
  else if (selectElementType == ElementType::Relation)
  {
    selectOffset = _relationIndex;
  }
  else
  {
    //hasMoreElements should have kept us from being here
    throw HootException("_getCurrentElementOffset");
  }
  LOG_VART(selectOffset);
  return selectOffset;
}

void ApiDbReader::_incrementElementIndex(const ElementType& selectElementType)
{
  if (selectElementType == ElementType::Node)
  {
    _nodeIndex++;
    LOG_VART(_nodeIndex);
  }
  else if (selectElementType == ElementType::Way)
  {
    _wayIndex++;
    LOG_VART(_wayIndex);
  }
  else if (selectElementType == ElementType::Relation)
  {
    _relationIndex++;
    LOG_VART(_relationIndex);
  }
  else
  {
    //hasMoreElements should have kept us from being here
    throw HootException("_incrementElementIndex");
  }
}

void ApiDbReader::finalizePartial()
{
  LOG_DEBUG("Finalizing read operation...");
  _elementResultIterator.reset();
  _partialMap.reset();
  if (_open)
  {
    //The exception thrown by this commit will mask exception text coming from failed queries.  Not
    //sure yet how to prevent that from happening, so you may have to temporarily comment out the
    //commit statement to see query error detail when debugging.
    _getDatabase()->commit();
    _getDatabase()->close();
    _open = false;
  }
}

void ApiDbReader::close()
{
  LOG_DEBUG("Closing database reader...");
  finalizePartial();
}

boost::shared_ptr<Element> ApiDbReader::_resultToElement(QSqlQuery& resultIterator,
                                                         const ElementType& elementType,
                                                         OsmMap& map)
{
  assert(resultIterator.isActive());
  //It makes much more sense to have callers call next on the iterator before passing it into this
  //method.  However, I was getting some initialization errors with QSqlQuery when the
  //reader called it in that way during a partial map read.  So, calling it inside here
  //instead.  A side effect is that this method will return a NULL element during the last
  //iteration.  Therefore, callers should check resultIterator->isActive in a loop in place of
  //calling resultIterator->next() and also should check for the null element.
  if (resultIterator.next())
  {
    boost::shared_ptr<Element> element;
    switch (elementType.getEnum())
    {
      case ElementType::Node:
        {
          element = _resultToNode(resultIterator, map);
        }
        break;

      case ElementType::Way:
        {
          element = _resultToWay(resultIterator, map);
        }
        break;

      case ElementType::Relation:
        {
          element = _resultToRelation(resultIterator, map);
        }
        break;

      default:
        throw HootException(QString("Unexpected element type: %1").arg(elementType.toString()));
    }

    return element;
  }
  else
  {
    resultIterator.finish();
    return boost::shared_ptr<Element>();
  }
}

boost::shared_ptr<OGRSpatialReference> ApiDbReader::getProjection() const
{
  boost::shared_ptr<OGRSpatialReference> wgs84(new OGRSpatialReference());
  if (wgs84->SetWellKnownGeogCS("WGS84") != OGRERR_NONE)
  {
    throw HootException("Error creating EPSG:4326 projection.");
  }
  return wgs84;
}

}
