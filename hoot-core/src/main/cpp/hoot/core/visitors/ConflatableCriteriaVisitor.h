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
 * @copyright Copyright (C) 2019, 2020 DigitalGlobe (http://www.digitalglobe.com/)
 */
#ifndef CONFLATABLE_CRITERIA_VISITOR_H
#define CONFLATABLE_CRITERIA_VISITOR_H

// hoot
#include <hoot/core/elements/ElementVisitor.h>
#include <hoot/core/elements/ConstOsmMapConsumer.h>
#include <hoot/core/elements/OsmMap.h>

namespace hoot
{

/**
 * This is primarily useful for debugging when trying to find out whether hoot can conflate or
 * which conflator it will use for a particular feature.
 */
class ConflatableCriteriaVisitor : public ElementVisitor, public ConstOsmMapConsumer
{

public:

  static std::string className() { return "hoot::ConflatableCriteriaVisitor"; }

  ConflatableCriteriaVisitor() = default;
  virtual ~ConflatableCriteriaVisitor() = default;

  virtual void visit(const std::shared_ptr<Element>& e);

  virtual QString getDescription() const
  { return "Marks elements with all criterion classes that consider them conflatable"; }

  virtual std::string getClassName() const { return className(); }

  virtual void setOsmMap(const OsmMap* map) { _map = map->shared_from_this(); }

private:

  ConstOsmMapPtr _map;
};

}

#endif // CONFLATABLE_CRITERIA_VISITOR_H
