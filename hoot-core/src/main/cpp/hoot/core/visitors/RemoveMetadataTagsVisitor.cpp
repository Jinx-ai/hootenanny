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
#include "RemoveMetadataTagsVisitor.h"

// hoot
#include <hoot/core/util/Factory.h>
#include <hoot/core/schema/OsmSchema.h>
#include <hoot/core/util/ConfigOptions.h>
#include <hoot/core/util/Log.h>
#include <hoot/core/criterion/NotCriterion.h>

namespace hoot
{

HOOT_FACTORY_REGISTER(ElementVisitor, RemoveMetadataTagsVisitor)

void RemoveMetadataTagsVisitor::visit(const std::shared_ptr<Element>& e)
{
  // see if the element passes the filter (if there is one)
  if (_criterion.get() && !_criterion->isSatisfied(e))
  {
    return;
  }
  _numAffected++;

  Tags tags = e->getTags();
  _numTagsRemoved += tags.removeMetadata();
  e->setTags(tags);
}

}
