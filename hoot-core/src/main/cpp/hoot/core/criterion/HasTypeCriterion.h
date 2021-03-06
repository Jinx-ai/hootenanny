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
 * @copyright Copyright (C) 2020 DigitalGlobe (http://www.digitalglobe.com/)
 */
#ifndef HAS_TYPE_CRITERION_H
#define HAS_TYPE_CRITERION_H

// Hoot
#include <hoot/core/criterion/ElementCriterion.h>

namespace hoot
{

/**
 * Determines if an element has a type recognized by the Hootenanny schema
 */
class HasTypeCriterion : public ElementCriterion
{
public:

  static std::string className() { return "hoot::HasTypeCriterion"; }

  HasTypeCriterion() = default;
  virtual ~HasTypeCriterion() = default;

  virtual bool isSatisfied(const ConstElementPtr& e) const;

  virtual ElementCriterionPtr clone() { return ElementCriterionPtr(new HasTypeCriterion()); }

  virtual QString getDescription() const
  { return "Determines if an element has a recognizable type."; }

  virtual QString toString() const override
  { return QString::fromStdString(className()).remove("hoot::"); }
};

}

#endif // HAS_TYPE_CRITERION_H
