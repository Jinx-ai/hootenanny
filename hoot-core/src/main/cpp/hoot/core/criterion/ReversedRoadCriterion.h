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
 * @copyright Copyright (C) 2016, 2017, 2018, 2019, 2020 DigitalGlobe (http://www.digitalglobe.com/)
 */
#ifndef REVERSED_ROAD_CRITERION_H
#define REVERSED_ROAD_CRITERION_H

// Hoot
#include <hoot/core/criterion/GeometryTypeCriterion.h>
#include <hoot/core/elements/ConstOsmMapConsumer.h>
#include <hoot/core/elements/OsmMap.h>

namespace hoot
{

class Element;

/**
 * Identifies reversed roads
 */
class ReversedRoadCriterion : public GeometryTypeCriterion, public ConstOsmMapConsumer
{
public:

  static std::string className() { return "hoot::ReversedRoadCriterion"; }

  ReversedRoadCriterion() = default;
  ReversedRoadCriterion(ConstOsmMapPtr map) : _map(map) { }
  virtual ~ReversedRoadCriterion() = default;

  virtual bool isSatisfied(const ConstElementPtr& e) const override;

  virtual ElementCriterionPtr clone() { return ElementCriterionPtr(new ReversedRoadCriterion()); }

  virtual QString getDescription() const { return "Identifies reversed roads"; }

  virtual GeometryType getGeometryType() const
  { return GeometryType::Line; }

  virtual QString toString() const override
  { return QString::fromStdString(className()).remove("hoot::"); }

  virtual void setOsmMap(const OsmMap* map) { _map = map->shared_from_this(); }

private:

  ConstOsmMapPtr _map;
};

}

#endif // REVERSED_ROAD_CRITERION_H
