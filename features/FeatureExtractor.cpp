#include <fstream>
#include <sstream>
#include <io/vectors.h>
#include <util/Logger.h>
#include <util/ProgramOptions.h>
#include <util/helpers.hpp>
#include <util/timing.h>
#include <vigra/multi_impex.hxx>

#include "FeatureExtractor.h"

logger::LogChannel featureextractorlog("featureextractorlog", "[FeatureExtractor] ");

util::ProgramOption optionDumpFeatureNames(
	util::_long_name        = "dumpFeatureNames",
	util::_description_text = "Write the feature names to files. The filenames will be the value of this argument plus 'node_?' or 'edge_?' "
	                          "for the respective node and edge types."
);

void
FeatureExtractor::extract(
		FeatureProviderBase& featureProvider,
		NodeFeatures& nodeFeatures,
		EdgeFeatures& edgeFeatures) {

	extractNodeFeatures(featureProvider, nodeFeatures);
	extractEdgeFeatures(featureProvider, nodeFeatures, edgeFeatures);
}

void
FeatureExtractor::extractNodeFeatures(
		FeatureProviderBase& featureProvider,
		NodeFeatures& nodeFeatures) {

	int numNodes = _crag.nodes().size();

	LOG_USER(featureextractorlog) << "extracting features for " << numNodes << " nodes" << std::endl;

	featureProvider.appendFeatures(_crag, nodeFeatures);

	LOG_USER(featureextractorlog)
			<< "extracted " << nodeFeatures.dims(Crag::VolumeNode)
			<< " features per volume node" << std::endl;
	LOG_USER(featureextractorlog)
			<< "extracted " << nodeFeatures.dims(Crag::SliceNode)
			<< " features per slice node" << std::endl;
	LOG_USER(featureextractorlog)
			<< "extracted " << nodeFeatures.dims(Crag::AssignmentNode)
			<< " features per assignment node" << std::endl;

	LOG_DEBUG(featureextractorlog)
			<< "base volume node features: " << nodeFeatures.getFeatureNames(Crag::VolumeNode)
			<< std::endl;
	LOG_DEBUG(featureextractorlog)
			<< "base slice node features: " << nodeFeatures.getFeatureNames(Crag::SliceNode)
			<< std::endl;
	LOG_DEBUG(featureextractorlog)
			<< "base assignment node features: " << nodeFeatures.getFeatureNames(Crag::AssignmentNode)
			<< std::endl;

	_numOriginalVolumeNodeFeatures     = nodeFeatures.dims(Crag::VolumeNode);
	_numOriginalSliceNodeFeatures      = nodeFeatures.dims(Crag::SliceNode);
	_numOriginalAssignmentNodeFeatures = nodeFeatures.dims(Crag::AssignmentNode);

	if (optionDumpFeatureNames) {

		for (auto type : Crag::NodeTypes) {

			std::string filename = optionDumpFeatureNames.as<std::string>() + "node_" + boost::lexical_cast<std::string>(type);
			std::ofstream file(filename);

			file << "number of features: " << nodeFeatures.dims(type) << "\n";
			file << "number of names: " << nodeFeatures.getFeatureNames(type).size() << "\n";

			for (auto name : nodeFeatures.getFeatureNames(type))
				file << name << "\n";
			file.close();
		}
	}

	LOG_USER(featureextractorlog) << "done" << std::endl;
}

void
FeatureExtractor::extractEdgeFeatures(
		FeatureProviderBase& featureProvider,
		const NodeFeatures& nodeFeatures,
		EdgeFeatures&       edgeFeatures) {

	LOG_USER(featureextractorlog) << "extracting edge features..." << std::endl;

	featureProvider.appendFeatures(_crag, edgeFeatures);

	LOG_USER(featureextractorlog)
			<< "extracted " << edgeFeatures.dims(Crag::AdjacencyEdge)
			<< " features per adjacency edge" << std::endl;
	LOG_USER(featureextractorlog)
			<< "extracted " << edgeFeatures.dims(Crag::NoAssignmentEdge)
			<< " features per no-assignment edge" << std::endl;

    if (optionDumpFeatureNames) {

        for (auto type : Crag::EdgeTypes) {

            std::string filename = optionDumpFeatureNames.as<std::string>() + "edge_" + boost::lexical_cast<std::string>(type);
            std::ofstream file(filename);

			file << "number of features: " << edgeFeatures.dims(type) << "\n";
			file << "number of names: " << edgeFeatures.getFeatureNames(type).size() << "\n";

            for (auto name : edgeFeatures.getFeatureNames(type))
                file << name << "\n";
            file.close();
        }
    }

    LOG_USER(featureextractorlog) << "done" << std::endl;
}

void
FeatureExtractor::normalize(
		NodeFeatures& nodeFeatures,
		EdgeFeatures& edgeFeatures,
		FeatureWeights& min,
		FeatureWeights& max){

	if (!min.empty() && !max.empty()) {

		LOG_USER(featureextractorlog) << "normalizing node features with provided min and max" << std::endl;
		LOG_ALL(featureextractorlog)
				<< "min:" << min << "max:" << max << std::endl;

		nodeFeatures.normalize(min, max);

		if (edgeFeatures.dims(Crag::AdjacencyEdge) > 0)
		{
			LOG_USER(featureextractorlog) << "normalizing edge features with provided min and max" << std::endl;
			edgeFeatures.normalize(min, max);
		}
	} else {

		LOG_USER(featureextractorlog) << "normalizing node features" << std::endl;

		nodeFeatures.normalize();
		nodeFeatures.getMin(min);
		nodeFeatures.getMax(max);

		LOG_USER(featureextractorlog) << "normalizing edge features" << std::endl;

		edgeFeatures.normalize();
		edgeFeatures.getMin(min);
		edgeFeatures.getMax(max);
	}
}
