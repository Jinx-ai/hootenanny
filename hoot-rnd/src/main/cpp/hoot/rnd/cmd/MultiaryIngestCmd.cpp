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
 * @copyright Copyright (C) 2017 DigitalGlobe (http://www.digitalglobe.com/)
 */

// Hoot
#include <hoot/core/util/Factory.h>
#include <hoot/core/cmd/BaseCommand.h>
#include <hoot/core/io/ChangesetDeriver.h>
#include <hoot/core/io/ElementSorter.h>
#include <hoot/core/io/OsmMapReaderFactory.h>
#include <hoot/core/io/OsmMapWriterFactory.h>
#include <hoot/core/io/HootApiDbWriter.h>
#include <hoot/rnd/io/SparkChangesetWriter.h>
#include <hoot/core/io/HootApiDbReader.h>
#include <hoot/core/io/OsmChangeWriterFactory.h>
#include <hoot/rnd/io/ElementCriterionVisitorInputStream.h>
#include <hoot/core/filters/PoiCriterion.h>
#include <hoot/core/visitors/TranslationVisitor.h>
#include <hoot/core/io/ElementOutputStream.h>
#include <hoot/core/io/GeoNamesReader.h>
#include <hoot/core/visitors/CalculateHashVisitor2.h>
#include <hoot/core/util/FileUtils.h>
#include <hoot/core/io/OsmPbfReader.h>
#include <hoot/core/io/OsmXmlReader.h>
#include <hoot/core/io/OgrReader.h>
#include <hoot/core/io/OsmPbfWriter.h>

// Qt
#include <QUuid>
#include <QElapsedTimer>
#include <QTemporaryFile>
#include <QFileInfo>

namespace hoot
{

/**
 * This command ingests data into the Multiary POI conflation workflow.  It takes a supported
 * data input, a reference database layer, and a changeset output location.  The input is
 * filtered down to POIs only and translated to OSM, then sorted by element ID (if not already).
 * Finally, the input data is then compared to the database ref layer in order to derive the
 * difference between the two in the form of a changeset.  The changeset changes are written both
 * to the reference layer as features and to a changeset output file as change statements for later
 * use by Spark.  Alternatively, if the specified database reference layer is empty, no changeset
 * is derived and the entire contents of the data input are simply written directly to the
 * reference layer.
 *
 * This command requires that the input be a streamable format, the output layer be a Hootenanny
 * API database layer, and the changeset output format be a Spark changeset.
 */
class MultiaryIngestCmd : public BaseCommand
{
public:

  static std::string className() { return "hoot::MultiaryIngestCmd"; }

  MultiaryIngestCmd() :
  _sortInput(true),
  _changesParsed(0)
  {
  }

  virtual QString getName() const { return "multiary-ingest"; }

  virtual int runSimple(QStringList args)
  {
    if (args.size() != 3 && args.size() != 4)
    {
      std::cout << getHelp() << std::endl << std::endl;
      throw HootException(QString("%1 takes three or four parameters.").arg(getName()));
    }

    const QString newInput = args[0];
    const QString referenceOutput = args[1];
    const QString changesetOutput = args[2];
    _sortInput = true;
    if (args.size() == 4 && args[3].toLower().trimmed() == "false")
    {
      _sortInput = false;
    }

    LOG_VARD(newInput);
    LOG_VARD(referenceOutput);
    LOG_VARD(changesetOutput);
    LOG_VARD(_sortInput);

    if (!OsmMapReaderFactory::getInstance().hasElementInputStream(newInput))
    {
      throw IllegalArgumentException(
        QString("This command currently only supports streamable input formats.") +
        QString("See the Supported Data Formats section in README.md for more detail.") +
        QString("Specified input: ") + newInput);
    }

    if (!HootApiDbReader().isSupported(referenceOutput))
    {
      throw HootException(
        getName() + " only supports a hootapidb:// data source as the reference output.  " +
        "Specified reference layer: " + referenceOutput);
    }

    if (!SparkChangesetWriter().isSupported(changesetOutput))
    {
      throw HootException(
        getName() + " only supports a .spark.x file for changeset output.  Specified output: " +
        changesetOutput);
    }

    //inputs must be sorted by element id for changeset derivation to work
    conf().set(ConfigOptions::getApiDbReaderSortByIdKey(), true);
    //in order for the sorting to work, all original element ids must be retained...no new ones
    //assigned; we're assuming no duplicate ids in the input
    conf().set(ConfigOptions::getHootapiDbWriterRemapIdsKey(), false);
    //translate inputs to OSM
    conf().set(ConfigOptions::getTranslationScriptKey(), "translations/OSM_Ingest.js");
    //for the changeset derivation to work, all input IDs must not be modified
    conf().set(ConfigOptions::getReaderUseDataSourceIdsKey(), true);

    LOG_INFO(
      "Ingesting Multiary data from " << newInput << " into reference output layer: " <<
      referenceOutput << " and writing changes to changeset file: " << changesetOutput << "...");

    HootApiDb referenceDb;
    referenceDb.open(referenceOutput);
    const QStringList dbUrlParts = referenceOutput.split("/");
    if (!referenceDb.mapExists(dbUrlParts[dbUrlParts.size() - 1]))
    {
      LOG_INFO(
        "Reference dataset does not exist.  Generating a changeset from the input data only.");

      //If there's no existing reference data, then there's no point in deriving a changeset diff.
      //So in that case, write all of the input data directly to the ref layer and generate a Spark
      //changeset consisting entirely of create statements.
      _writeChanges(_getFilteredNewInputStream(newInput), referenceOutput, changesetOutput);
    }
    else
    {
      LOG_INFO(
        "Reference dataset exists.  Deriving a changeset between the input and reference data.");

      //sort incoming data by element id, if necessary, for changeset derivation (only passing nodes
      //through, so don't need to also sort by element type)
      _sortedNewInput = _getSortedNewInput(newInput);

      //create the changes and write them to the ref db layer and also to a changeset file for
      //external use by Spark
      _deriveAndWriteChanges(
        _getFilteredNewInputStream(_sortedNewInput), referenceOutput, changesetOutput);
    }

    LOG_INFO(
      "Multiary data ingested from " << newInput << " into reference output layer: " <<
      referenceOutput << " and changes written to changeset file: " << changesetOutput <<
      ".  Time: " << FileUtils::secondsToDhms(_timer.elapsed()));
    LOG_INFO("Changes Parsed: " << FileUtils::formatPotentiallyLargeNumber(_changesParsed));

    return 0;
  }

private:

  bool _sortInput;
  QString _sortedNewInput;
  boost::shared_ptr<QTemporaryFile> _sortTempFile;

  long _changesParsed;

  QElapsedTimer _timer;

  void _checkForOsmosis()
  {
    if (std::system(QString("osmosis -q > /dev/null").toStdString().c_str()) != 0)
    {
      throw HootException(
        QString("Unable to access the Osmosis application.  Osmosis is required to") +
        QString("sort OSM PBF and OSM XML files.  Is Osmosis installed?"));
    }
  }

  void _sortPbf(const QString input, const QString output)
  {
    _checkForOsmosis();

    const QString cmd =
      "osmosis -q --read-pbf file=\"" + input + "\" --sort --write-pbf " +
      "omitmetadata=true file=\"" + output + "\" > /dev/null";
    if (std::system(cmd.toStdString().c_str()) != 0)
    {
      throw HootException("Unable to sort OSM PBF file.");
    }
  }

  boost::shared_ptr<QTemporaryFile> _ogrToPbf(const QString input)
  {
    boost::shared_ptr<QTemporaryFile> pbfTemp(
      new QTemporaryFile("multiary-ingest-sort-temp-XXXXXX.osm.pbf"));
    if (!pbfTemp->open())
    {
      throw HootException("Unable to open sort temp file: " + pbfTemp->fileName() + ".");
    }
    LOG_INFO(
      "Converting OGR input: " << input << " to sortable PBF output: " << pbfTemp->fileName() <<
      "...");

    OgrReader reader;
    reader.open(input);
    reader.initializePartial();

    boost::shared_ptr<OsmPbfWriter> writer(new OsmPbfWriter());
    writer->open(pbfTemp->fileName());
    writer->initializePartial();
    boost::shared_ptr<PartialOsmMapWriter> partialWriter =
      boost::dynamic_pointer_cast<PartialOsmMapWriter>(writer);

    while (reader.hasMoreElements())
    {
      ElementPtr element = reader.readNextElement();
      partialWriter->writeElement(element);
    }

    reader.finalizePartial();
    partialWriter->finalizePartial();

    return pbfTemp;
  }

  QString _getSortedNewInput(const QString newInput)
  {
    if (!_sortInput)
    {
      return newInput;
    }

    LOG_INFO("Sorting " << newInput << " by node ID...");
    _timer.restart();

    const QString sortTempFileBaseName = "multiary-ingest-sort-temp-XXXXXX";
    if (!OgrReader().isSupported(newInput))
    {
      QFileInfo newInputFileInfo(newInput);
      _sortTempFile.reset(
        new QTemporaryFile(sortTempFileBaseName + "." + newInputFileInfo.completeSuffix()));
    }
    else
    {
      _sortTempFile.reset(new QTemporaryFile(sortTempFileBaseName + ".osm.pbf"));
    }
    if (!_sortTempFile->open())
    {
      throw HootException("Unable to open sort temp file: " + _sortTempFile->fileName() + ".");
    }

    if (GeoNamesReader().isSupported(newInput))
    {
      //sort the input by node id (first field) using the unix sort command
      if (std::system(
           QString("sort " + newInput + " --output=" +
             _sortTempFile->fileName()).toStdString().c_str()) != 0)
      {
        throw HootException("Unable to sort input file.");
      }
    }
    //use osmosis to sort the OSM files by node id
    else if (OsmPbfReader().isSupported(newInput))
    {
      //check for sorted flag
      if (OsmPbfReader().isSorted(newInput))
      {
        LOG_WARN(
          "OSM PBF input file: " << newInput << " is marked as sorted by node ID, as " <<
          "indicated by its header, yet Hootenanny was instructed to sort the file.");
      }
      _sortPbf(newInput, _sortTempFile->fileName());
    }
    else if (OsmXmlReader().isSupported(newInput))
    {
      _checkForOsmosis();

      const QString cmd =
        "osmosis -q --read-xml file=\"" + newInput + "\" --sort --write-xml file=\"" +
        _sortTempFile->fileName() + "\"  > /dev/null";
      if (std::system(cmd.toStdString().c_str()) != 0)
      {
        throw HootException("Unable to sort OSM XML file.");
      }
    }
    else if (OgrReader().isSupported(newInput))
    {
      //Unfortunately for now, sorting an OGR input is going to require an extra pass over the data
      //to first write it to a sortable format.
      _sortPbf(_ogrToPbf(newInput)->fileName(), _sortTempFile->fileName());
    }
    else
    {
      throw HootException("Unsupported input format for node sorting.");
    }

    LOG_INFO(
      newInput << " sorted by node ID.  Time: " << FileUtils::secondsToDhms(_timer.elapsed()));

    return _sortTempFile->fileName();
  }

  boost::shared_ptr<ElementInputStream> _getFilteredNewInputStream(const QString sortedNewInput)
  {
    boost::shared_ptr<PartialOsmMapReader> newInputReader =
      boost::dynamic_pointer_cast<PartialOsmMapReader>(
        OsmMapReaderFactory::getInstance().createReader(sortedNewInput));
    newInputReader->setUseDataSourceIds(true);
    newInputReader->open(sortedNewInput);
    boost::shared_ptr<ElementInputStream> inputStream =
      boost::dynamic_pointer_cast<ElementInputStream>(newInputReader);

    //as the incoming data is read, filter it down to POIs only and translate each element;

    boost::shared_ptr<PoiCriterion> elementCriterion;
    //all geonames are pois, so skip the element filtering expense for that format
    if (!GeoNamesReader().isSupported(sortedNewInput))
    {
      elementCriterion.reset(new PoiCriterion());
    }
    QList<ElementVisitorPtr> visitors;
    visitors.append(boost::shared_ptr<TranslationVisitor>(new TranslationVisitor()));
    visitors.append(boost::shared_ptr<CalculateHashVisitor2>(new CalculateHashVisitor2()));
    boost::shared_ptr<ElementInputStream> filteredNewInputStream(
      new ElementCriterionVisitorInputStream(inputStream, elementCriterion, visitors));
    return filteredNewInputStream;
  }

  void _writeChanges(boost::shared_ptr<ElementInputStream> filteredNewInputStream,
                     const QString referenceOutput, const QString changesetOutput)
  {
    _timer.restart();

    HootApiDbWriter referenceWriter;
    referenceWriter.setCreateUser(true);
    referenceWriter.setOverwriteMap(true);
    referenceWriter.open(referenceOutput);

    SparkChangesetWriter changesetFileWriter;
    changesetFileWriter.open(changesetOutput);

    while (filteredNewInputStream->hasMoreElements())
    {
      ElementPtr element = filteredNewInputStream->readNextElement();

      referenceWriter.writeElement(element);
      changesetFileWriter.writeChange(Change(Change::Create, element));
      _changesParsed++;
    }
  }

  void _deriveAndWriteChanges(boost::shared_ptr<ElementInputStream> filteredNewInputStream,
                              const QString referenceOutput, const QString changesetOutput)
  {
    _timer.restart();

    LOG_VARD(conf().get(ConfigOptions::getApiDbReaderSortByIdKey()));
    boost::shared_ptr<HootApiDbReader> referenceReader(new HootApiDbReader());
    referenceReader->setUseDataSourceIds(true);
    referenceReader->open(referenceOutput);

    ChangesetDeriverPtr changesetDeriver(
      new ChangesetDeriver(
        boost::dynamic_pointer_cast<ElementInputStream>(referenceReader), filteredNewInputStream));

    HootApiDbWriter referenceChangeWriter;
    referenceChangeWriter.setCreateUser(false);
    referenceChangeWriter.setOverwriteMap(false);
    referenceChangeWriter.open(referenceOutput);

    SparkChangesetWriter changesetFileWriter;
    changesetFileWriter.open(changesetOutput);

    while (changesetDeriver->hasMoreChanges())
    {
      const Change change = changesetDeriver->readNextChange();
      if (change.getType() != Change::Unknown)
      {
        changesetFileWriter.writeChange(change);
        referenceChangeWriter.writeChange(change);
        _changesParsed++;
      }
    }
  }
};

HOOT_FACTORY_REGISTER(Command, MultiaryIngestCmd)

}
