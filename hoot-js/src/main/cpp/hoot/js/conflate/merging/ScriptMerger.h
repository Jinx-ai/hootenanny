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
#ifndef SCRIPTMERGER_H
#define SCRIPTMERGER_H

// hoot
#include <hoot/core/conflate/merging/MergerBase.h>
#include <hoot/js/PluginContext.h>

namespace hoot
{

/**
 * @sa ScriptMatch
 */
class ScriptMerger : public MergerBase
{
public:

  static std::string className() { return "hoot::ScriptMerger"; }

  static int logWarnCount;

  ScriptMerger() = default;
  virtual ~ScriptMerger() = default;

  ScriptMerger(const std::shared_ptr<PluginContext>& script, v8::Persistent<v8::Object>& plugin,
    const std::set<std::pair<ElementId, ElementId>>& pairs);

  virtual void apply(const OsmMapPtr& map,
                     std::vector<std::pair<ElementId, ElementId>>& replaced) override;

  /**
   * Returns true if the plugin has a function with the specified name.
   */
  bool hasFunction(QString name) const;

  virtual QString toString() const override { return QString("ScriptMerger"); }

  virtual QString getDescription() const
  { return "Merges elements matched with Generic Conflation"; }

  virtual QString getName() const { return QString::fromStdString(className()) + "-" + _matchType; }

  void setMatchType(const QString& matchType) { _matchType = matchType; }

protected:

  PairsSet _pairs;
  v8::Persistent<v8::Object> _plugin;
  std::shared_ptr<PluginContext> _script;
  ElementId _eid1, _eid2;
  QString _matchType;

  /**
   * Calls mergePair in the JS.
   */
  virtual void _applyMergePair(
    const OsmMapPtr& map, std::vector<std::pair<ElementId, ElementId>>& replaced) const;
  /**
   * Calls mergeSet in the JS.
   */
  virtual void _applyMergeSets(
    const OsmMapPtr& map, std::vector<std::pair<ElementId, ElementId>>& replaced) const;

  v8::Handle<v8::Value> _callMergePair(const OsmMapPtr& map) const;
  void _callMergeSets(const OsmMapPtr& map,
                      std::vector<std::pair<ElementId, ElementId>>& replaced) const;
  virtual PairsSet& _getPairs() override { return _pairs; }
  virtual const PairsSet& _getPairs() const override { return _pairs; }
};

}

#endif // SCRIPTMERGER_H
