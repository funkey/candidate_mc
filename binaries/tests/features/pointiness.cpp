#include <map>
#include <tests.h>
#include <region_features/ShapeFeatures.h>

namespace pointiness_test {

struct Features {

	void append(unsigned char label, double v) { BOOST_CHECK_EQUAL(label, 1); values.push_back(v); }

	std::vector<double> values;
};

} using namespace pointiness_test;

void pointiness() {

	// create a square
	vigra::MultiArray<2, unsigned char> square(vigra::Shape2(100, 100));
	square = 1;

	{
		// get pointiness features for it
		region_features::ShapeFeatures<2, unsigned char>::Parameters p;
		p.numAnglePoints = 4;
		p.contourVecAsArcSegmentRatio = 0.25;
		p.numAngleHistBins = 4;
		region_features::ShapeFeatures<2, unsigned char> shapeFeatures(p);

		Features features;
		shapeFeatures.fill(square, features);

		BOOST_REQUIRE_EQUAL(features.values.size(), 7);

		// average angle
		BOOST_CHECK_CLOSE(features.values[0], M_PI/2.0, 1.5);
		// hist bins
		BOOST_CHECK_EQUAL(features.values[1], 0); // 0  ... ¼π
		BOOST_CHECK_EQUAL(features.values[2], 0); // ¼π ... ½π
		BOOST_CHECK_EQUAL(features.values[3], 4); // ½π ... ¾π
		BOOST_CHECK_EQUAL(features.values[4], 0); // ¾π ...  π
	}

	{
		// get pointiness features for it
		region_features::ShapeFeatures<2, unsigned char>::Parameters p;
		p.numAnglePoints = 8;
		p.contourVecAsArcSegmentRatio = 0.25;
		p.numAngleHistBins = 4;
		region_features::ShapeFeatures<2, unsigned char> shapeFeatures(p);

		Features features;
		shapeFeatures.fill(square, features);

		BOOST_REQUIRE_EQUAL(features.values.size(), 7);

		// average angle
		BOOST_CHECK_CLOSE(features.values[0], 3.0*M_PI/4.0, 1.5);
		// hist bins
		BOOST_CHECK_EQUAL(features.values[1], 0); // 0  ... ¼π
		BOOST_CHECK_EQUAL(features.values[2], 0); // ¼π ... ½π
		BOOST_CHECK_EQUAL(features.values[3], 4); // ½π ... ¾π
		BOOST_CHECK_EQUAL(features.values[4], 4); // ¾π ...  π
	}

	{
		// get pointiness features for it
		region_features::ShapeFeatures<2, unsigned char>::Parameters p;
		p.numAnglePoints = 100;
		p.contourVecAsArcSegmentRatio = 0.25;
		p.numAngleHistBins = 4;
		region_features::ShapeFeatures<2, unsigned char> shapeFeatures(p);

		Features features;
		shapeFeatures.fill(square, features);

		BOOST_REQUIRE_EQUAL(features.values.size(), 7);

		// hist bins
		BOOST_CHECK_EQUAL(features.values[1], 0);  // 0  ... ¼π
		BOOST_CHECK_EQUAL(features.values[2], 0);  // ¼π ... ½π
		BOOST_CHECK_EQUAL(features.values[3], 4);  // ½π ... ¾π
		BOOST_CHECK_EQUAL(features.values[4], 96); // ¾π ...  π
	}
}
