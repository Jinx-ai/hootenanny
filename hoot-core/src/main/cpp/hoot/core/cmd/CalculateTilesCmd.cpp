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
 * @copyright Copyright (C) 2015, 2016, 2017 DigitalGlobe (http://www.digitalglobe.com/)
 */

// Hoot
#include <hoot/core/util/Factory.h>
#include <hoot/core/cmd/BaseCommand.h>
#include <hoot/core/io/OsmMapReader.h>
#include <hoot/core/io/OsmMapReaderFactory.h>
#include <hoot/core/io/OsmMapWriterFactory.h>
#include <hoot/core/util/GeometryUtils.h>
#include <hoot/core/util/OpenCv.h>
#include <hoot/core/conflate/TileBoundsCalculator.h>
#include <hoot/core/visitors/CalculateMapBoundsVisitor.h>
#include <hoot/core/io/ApiDbReader.h>

// Qt
#include <QTemporaryFile>

namespace hoot
{

class CalculateTilesCmd : public BaseCommand
{

  public:

    static std::string className() { return "hoot::CalculateTilesCmd"; }

    CalculateTilesCmd() { }

    virtual QString getName() const { return "calculate-tiles"; }

    virtual int runSimple(QStringList args)
    {
      if (args.size() != 2 && args.size() != 4)
      {
        std::cout << getHelp() << std::endl << std::endl;
        throw HootException(QString("%1 takes either two or four parameters.").arg(getName()));
      }

      QStringList inputs;
      const QString input = args[0];
      LOG_VARD(input);
      if (!input.contains(";"))
      {
        inputs.append(input);
      }
      else
      {
        //multiple inputs
        inputs = input.split(";");
      }
      LOG_VARD(inputs);

      const QString output = args[1];
      if (!output.toLower().endsWith(".geojson"))
      {
        throw HootException(
          "Invalid output file format: " + output + ".  Only the GeoJSON output format is " +
          "supported.");
      }
      LOG_VARD(output);

      //if either of max nodes per tile or pixel size is specified, then both must be specified

      long maxNodesPerTile = 1000;
      if (args.size() > 2)
      {
        bool parseSuccess = false;
        maxNodesPerTile = args[2].toLong(&parseSuccess);
        if (!parseSuccess || maxNodesPerTile < 1)
        {
          throw HootException("Invalid maximum nodes per tile value: " + args[2]);
        }
      }
      LOG_VARD(maxNodesPerTile);

      double pixelSize = 0.001; //.1km?
      if (args.size() > 2)
      {
        bool parseSuccess = false;
        pixelSize = args[3].toDouble(&parseSuccess);
        if (!parseSuccess || pixelSize <= 0.0)
        {
          throw HootException("Invalid pixel size value: " + args[3]);
        }
      }
      LOG_VARD(pixelSize);

      conf().set(ConfigOptions().getIdGeneratorKey(), "hoot::PositiveIdGenerator");

      OsmMapPtr inputMap = _readInputs(inputs);
      const std::vector< std::vector<geos::geom::Envelope> > tiles =
        _calculateTiles(maxNodesPerTile, pixelSize, inputMap);
      _writeOutputAsGeoJson(tiles, output);

      return 0;
    }

  private:

    OsmMapPtr _readInputs(const QStringList inputs)
    { 
      const bool bboxSpecified =
        !ConfigOptions().getConvertBoundingBox().trimmed().isEmpty() ||
        !ConfigOptions().getConvertBoundingBoxHootApiDatabase().trimmed().isEmpty() ||
        !ConfigOptions().getConvertBoundingBoxOsmApiDatabase().trimmed().isEmpty();

      OsmMapPtr map(new OsmMap());
      for (int i = 0; i < inputs.size(); i++)
      {
        boost::shared_ptr<OsmMapReader> reader =
          OsmMapReaderFactory::getInstance().createReader(inputs.at(i), true, Status::Unknown1);

        boost::shared_ptr<ApiDbReader> apiDbReader =
          boost::dynamic_pointer_cast<ApiDbReader>(reader);
        if (apiDbReader)
        {
          //the tiles calculation is only concerned with nodes, and the only readers capable of
          //filtering down to nodes up front right now are the api db readers; more importantly,
          //setting this to true also prevents any features from being returned outside of
          //convert.bounding.box, if it was specified (ways partially inside the bounds, etc.)
          apiDbReader->setReturnNodesOnly(true);
        }
        if (bboxSpecified && !apiDbReader)
        {
          //non api db readers don't support convert.bounding.box right now and are just going to
          //generate confusing output with its specified, so let's throw
          throw HootException(
            "convert.bounding.box configuration option specified for a non API DB reader");
        }

        reader->open(inputs.at(i));
        reader->read(map);
      }
      LOG_VARD(map->getNodeCount());

      if (Log::getInstance().getLevel() <= Log::Debug)
      {
        OGREnvelope envelope = CalculateMapBoundsVisitor::getBounds(map);
        boost::shared_ptr<geos::geom::Envelope> tempEnv(GeometryUtils::toEnvelope(envelope));
        LOG_VARD(tempEnv->toString());
        const QString debugMapPath = "tmp/calc-tiles-combined-map-debug.osm";
        LOG_DEBUG("writing debug output to " << debugMapPath)
        OsmMapWriterFactory::getInstance().write(map, debugMapPath);
      }

      return map;
    }

    std::vector< std::vector<geos::geom::Envelope> > _calculateTiles(const long maxNodesPerTile,
                                                                     const double pixelSize,
                                                                     OsmMapPtr map)
    {
      TileBoundsCalculator tileBoundsCalculator(pixelSize);
      tileBoundsCalculator.setMaxNodesPerBox(maxNodesPerTile);
      //tbc.setSlop(0.1);
      cv::Mat r1, r2;
      tileBoundsCalculator.renderImage(map, r1, r2);
      //we're calculating for unknown1 only, so fill the second matrix with all zeroes
      cv::Mat zeros = cv::Mat::zeros(r1.size(), r1.type());
      tileBoundsCalculator.setImages(r1, zeros);
      return tileBoundsCalculator.calculateTiles();
    }

    /*write out to temp osm and then use ogr2ogr to convert to geojson

    This is kind of a shortcut way to get geojson output.  Its probably worth either figuring
    out a way to use 'hoot osm2ogr' without a translation (or just with a simple one) to do this
    OR adding an GeoJsonWriter class that can do this same thing using the GDAL API, so the
    'hoot convert' command can call it.*/
    void _writeOutputAsGeoJson(const std::vector< std::vector<geos::geom::Envelope> >& tiles,
                               const QString outputPath)
    {
      QTemporaryFile osmTempFile("calculate-tiles-temp-XXXXXX.osm");
      if (!osmTempFile.open())
      {
        throw HootException(
          "Unable to open OSM temp file: " + osmTempFile.fileName() + " for GeoJSON output.");
      }
      LOG_VARD(osmTempFile.fileName());
      _writeOutputAsOsm(tiles, osmTempFile.fileName());

      QFile outFile(outputPath);
      if (outFile.exists() && !outFile.remove())
      {
        throw HootException("Unable to open GeoJSON file: " + outputPath + " for writing.");
      }
      LOG_VARD(outputPath);

      //The "lines" part at the end is necessary to prevent a conversion error from osm to geojson.
      //Not fully clear on what that's doing yet.
      const QString cmd =
        "ogr2ogr -f GeoJSON " + outputPath + " " + osmTempFile.fileName() + " lines";
      //const QString cmd =
        //"ogr2ogr -f GeoJSON " + outputPath + " " + osmTempFile.fileName() + " multipolygons";
      //const QString cmd =
        //"ogr2ogr -f GeoJSON " + outputPath + " " + osmTempFile.fileName();
      //-skipfailures
      //const QString cmd =
        //"ogr2ogr -nlt POLYGON " + osmTempFile.fileName() + " " + outputPath + " OGRGeoJSON";
      //const QString cmd =
        //"ogr2ogr -nlt POLYGON " + outputPath + " " + osmTempFile.fileName() + " OGRGeoJSON";
      LOG_VARD(cmd);
      LOG_INFO("Writing output to " << outputPath);
      const int retval = std::system(cmd.toStdString().c_str());
      if (retval != 0)
      {
        throw HootException(
          "Failed converting " + osmTempFile.fileName() + " to GeoJSON: " + outputPath +
          ".  Status: " + QString::number(retval));
      }
    }

    void _writeOutputAsOsm(const std::vector< std::vector<geos::geom::Envelope> >& tiles,
                           const QString outputPath)
    {
      LOG_VARD(outputPath);

      //write out a viewable version of the boundaries for debugging purposes
      OsmMapPtr boundaryMap(new OsmMap());
      for (size_t tx = 0; tx < tiles.size(); tx++)
      {
        for (size_t ty = 0; ty < tiles[tx].size(); ty++)
        {
          const geos::geom::Envelope env = tiles[tx][ty];
          const double circularError = ConfigOptions().getCircularErrorDefaultValue();

          NodePtr lowerLeft(
            new Node(
              Status::Unknown1,
              boundaryMap->createNextNodeId(),
              geos::geom::Coordinate(env.getMinX(), env.getMinY()),
              circularError));
          boundaryMap->addNode(lowerLeft);
          NodePtr upperRight(
            new Node(
              Status::Unknown1,
              boundaryMap->createNextNodeId(),
              geos::geom::Coordinate(env.getMaxX(), env.getMaxY()),
              circularError));
          boundaryMap->addNode(upperRight);
          NodePtr upperLeft(
            new Node(
              Status::Unknown1,
              boundaryMap->createNextNodeId(),
              geos::geom::Coordinate(env.getMinX(), env.getMaxY()),
              circularError));
          boundaryMap->addNode(upperLeft);
          NodePtr lowerRight(
            new Node(
              Status::Unknown1,
              boundaryMap->createNextNodeId(),
              geos::geom::Coordinate(env.getMaxX(), env.getMinY()),
              circularError));
          boundaryMap->addNode(lowerRight);

          WayPtr bbox(new Way(Status::Unknown1, boundaryMap->createNextWayId(), circularError));
          bbox->addNode(lowerLeft->getId());
          bbox->addNode(upperLeft->getId());
          bbox->addNode(upperLeft->getId());
          bbox->addNode(upperRight->getId());
          bbox->addNode(upperRight->getId());
          bbox->addNode(lowerRight->getId());
          bbox->addNode(lowerRight->getId());
          bbox->addNode(lowerLeft->getId());
          boundaryMap->addWay(bbox);
        }
      }

      OsmMapWriterFactory::getInstance().write(boundaryMap, outputPath);

      if (Log::getInstance().getLevel() <= Log::Debug)
      {
        const QString debugOutputPath = "tmp/calc-tiles-osm-debug.osm";
        QFile outFile(outputPath);
        LOG_DEBUG("writing debug output to " << debugOutputPath);
        outFile.copy(debugOutputPath);
      }
    }
};

HOOT_FACTORY_REGISTER(Command, CalculateTilesCmd)

}
