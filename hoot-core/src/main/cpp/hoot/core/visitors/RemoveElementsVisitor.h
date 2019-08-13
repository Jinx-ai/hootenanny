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
 * @copyright Copyright (C) 2015, 2016, 2017, 2018, 2019 DigitalGlobe (http://www.digitalglobe.com/)
 */
#ifndef REMOVEELEMENTSVISITOR_H
#define REMOVEELEMENTSVISITOR_H

// hoot
#include <hoot/core/elements/OsmMapConsumer.h>
#include <hoot/core/elements/ElementVisitor.h>
#include <hoot/core/criterion/ElementCriterionConsumer.h>
#include <hoot/core/util/Configurable.h>
#include <hoot/core/info/OperationStatusInfo.h>
#include <hoot/core/util/StringUtils.h>

namespace hoot
{

/**
 * Removes any elements where that satisfy a criterion
 */
class RemoveElementsVisitor : public ElementVisitor, public OsmMapConsumer,
    public ElementCriterionConsumer, public Configurable, public OperationStatusInfo
{
public:

  static std::string className() { return "hoot::RemoveElementsVisitor"; }

  RemoveElementsVisitor(bool negateCriteria = false);

  virtual void addCriterion(const ElementCriterionPtr& crit);

  virtual void visit(const ElementPtr& e);

  virtual void setConfiguration(const Settings& conf);

  virtual void setOsmMap(OsmMap* map);
  virtual void setOsmMap(const OsmMap* /*map*/) { assert(false); }

  void setRecursive(bool recursive) { _recursive = recursive; }
  void setChainCriteria(bool chain) { _chainCriteria = chain; }

  static void removeWays(const std::shared_ptr<OsmMap>& pMap, const ElementCriterionPtr& pCrit);

  int getCount() { return _count; }

  virtual QString getDescription() const { return "Removes elements that satisfy a criterion"; }

  virtual QString getInitStatusMessage() const { return "Removing elements..."; }

  virtual QString getCompletedStatusMessage() const
  {
    return
      "Removed " + StringUtils::formatLargeNumber(_count) + " / " +
      StringUtils::formatLargeNumber(_startElementCount) + " elements."; }

private:

  OsmMap* _map;
  std::vector<ElementCriterionPtr> _criteria;
  bool _recursive;
  int _count;
  //This allows for negating the criterion as an option sent in from the command line.
  bool _negateCriteria;
  bool _chainCriteria;
  long _startElementCount;

  bool _criteriaSatisfied(const ConstElementPtr& e) const;
};


}

#endif // REMOVEELEMENTSVISITOR_H
