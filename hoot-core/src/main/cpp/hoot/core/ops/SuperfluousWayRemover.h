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
 * @copyright Copyright (C) 2015, 2017, 2018, 2019, 2020 DigitalGlobe (http://www.digitalglobe.com/)
 */

#ifndef SUPERFLUOUSWAYREMOVER_H
#define SUPERFLUOUSWAYREMOVER_H

// Hoot
#include <hoot/core/ops/OsmMapOperation.h>
#include <hoot/core/util/Units.h>
#include <hoot/core/util/Configurable.h>

// Qt
#include <QSet>

namespace hoot
{

class OsmMap;

/**
 * Removes all ways that are not being used by relations and contain zero nodes or all the nodes
 * are identical.
 *
 * @todo what about one node ways?
 */
class SuperfluousWayRemover : public OsmMapOperation, public Configurable
{
public:

  static std::string className() { return "hoot::SuperfluousWayRemover"; }

  SuperfluousWayRemover() = default;
  SuperfluousWayRemover(const std::shared_ptr<OsmMap>& map);
  virtual ~SuperfluousWayRemover() = default;

  void apply(std::shared_ptr<OsmMap>& map);

  /**
   * Splits all the ways in the input map and returns the resulting map.
   *
   * @return the number of ways removed
   */
  static long removeWays(const std::shared_ptr<OsmMap>& map);

  /**
   * TODO
   */
  void removeWays();

  /**
   * @see Configurable
   */
  virtual void setConfiguration(const Settings& conf);

  virtual QString getInitStatusMessage() const { return "Removing superfluous ways..."; }

  virtual QString getCompletedStatusMessage() const
  { return "Removed " + QString::number(_numAffected) + " superfluous ways"; }

  virtual QString getDescription() const
  { return "Removes ways not in relations or containing zero or all identical nodes"; }

  /**
   * @see FilteredByGeometryTypeCriteria
   */
  virtual QStringList getCriteria() const;

  virtual std::string getClassName() const { return className(); }

private:

  std::shared_ptr<OsmMap> _inputMap;

  // ways with these IDs will never be removed
  QSet<long> _excludeIds;
  // the number of ways that explicitly weren't removed due to configuration
  int _numExplicitlyExcluded;
};

}

#endif // SUPERFLUOUSWAYREMOVER_H
