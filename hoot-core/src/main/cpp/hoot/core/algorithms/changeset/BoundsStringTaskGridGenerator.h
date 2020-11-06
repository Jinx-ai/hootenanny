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
#ifndef BOUNDS_STRING_TASK_GRID_GENERATOR_H
#define BOUNDS_STRING_TASK_GRID_GENERATOR_H

// Hoot
#include <hoot/core/algorithms/changeset/TaskGridGenerator.h>

// GEOS
#include <geos/geom/Envelope.h>

namespace hoot
{

/**
 * Generates a task grid with a single cell from an envelope string
 *
 * TODO: Maybe we could support a non-rectangular bounds in this class (?), but not sure we need to
 * right now.
 */
class BoundsStringTaskGridGenerator : public TaskGridGenerator
{

public:

  /**
   * Constructor
   *
   * @param bounds a string of the form minx,miny,maxx,maxy
   * @param output optional output path for writing the bounds file
   */
  BoundsStringTaskGridGenerator(const QString& bounds, const QString& outputPath = "");
  virtual ~BoundsStringTaskGridGenerator() = default;

  /**
   * @see TaskGridGenerator
   */
  virtual TaskGrid generateTaskGrid();

private:

  // the bounds for the grid
  geos::geom::Envelope _bounds;
  QString _outputPath;
};

}

#endif // BOUNDS_STRING_TASK_GRID_GENERATOR_H
