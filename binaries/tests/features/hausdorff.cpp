#include <map>
#include <tests.h>
#include <crag/Crag.h>
#include <crag/CragVolumes.h>
#include <features/HausdorffDistance.h>

void hausdorff() {

	Crag cragA;
	Crag cragB;
	CragVolumes volumesA(cragA);
	CragVolumes volumesB(cragB);

	// add two nodes each
	Crag::Node a1 = cragA.addNode();
	Crag::Node a2 = cragA.addNode();
	Crag::Node b1 = cragB.addNode();
	Crag::Node b2 = cragB.addNode();

	// add more nodes as parents of a and b
	Crag::Node p_a1 = cragA.addNode();
	Crag::Node p_b1 = cragB.addNode();
	cragA.addSubsetArc(a1, p_a1);
	cragB.addSubsetArc(b1, p_b1);

	// add more nodes merging p_a1 and a2 (same for b)
	Crag::Node root_a = cragA.addNode();
	Crag::Node root_b = cragB.addNode();
	cragA.addSubsetArc(p_a1, root_a);
	cragA.addSubsetArc(a2, root_a);
	cragB.addSubsetArc(p_b1, root_b);
	cragB.addSubsetArc(b2, root_b);

	// create volumes
	std::shared_ptr<CragVolume> volumeA1 = std::make_shared<CragVolume>(11, 11, 1);
	std::shared_ptr<CragVolume> volumeA2 = std::make_shared<CragVolume>(11, 11, 1);
	std::shared_ptr<CragVolume> volumeB1 = std::make_shared<CragVolume>(11, 11, 1);
	std::shared_ptr<CragVolume> volumeB2 = std::make_shared<CragVolume>(11, 11, 1);
	volumeA2->setOffset(util::point<float, 3>(5, 5, 0));
	volumeB2->setOffset(util::point<float, 3>(5, 5, 0));

	volumeA1->data() = 0;
	volumeA2->data() = 0;
	volumeB1->data() = 0;
	volumeB2->data() = 0;

	// add a stripe for volumeA{1,2} and a dot for volumeB{1,2}
	//
	// 00000000000 00000000000
	// 00000000000 00000000000
	// 00000000000 00000000000
	// 00000000x00 00000000*00   x=8,y=3
	// 00000000000 00000000000
	// *********** xxxxxxxxxxx   y=5
	// 00000000000 00000000000
	// 00000000000 00000000000
	// 00000000000 00000000000
	// 00000000000 00000000000
	// 00000000000 00000000000

	for (int x = 0; x < 11; x++) {
		(*volumeA1)(x, 5, 0) = 1;
		(*volumeA2)(x, 5, 0) = 1;
	}
	(*volumeB1)(8, 3, 0) = 1;
	(*volumeB2)(8, 3, 0) = 1;

	volumesA.setVolume(a1, volumeA1);
	volumesA.setVolume(a2, volumeA2);
	volumesB.setVolume(b1, volumeB1);
	volumesB.setVolume(b2, volumeB2);

	HausdorffDistance hausdorff(100);

	double a_b, b_a;
	hausdorff(*volumesA[a1], *volumesB[b1], a_b, b_a);

	// Hausdorff should be sqrt(2*2 + 8*8) = 8.25 for A->B and 2 for B->A
	BOOST_CHECK_CLOSE(a_b, 8.246, 0.01);
	BOOST_CHECK_CLOSE(b_a, 2.0,   0.01);

	// same for parents
	hausdorff(*volumesA[p_a1], *volumesB[p_b1], a_b, b_a);
	BOOST_CHECK_CLOSE(a_b, 8.246, 0.01);
	BOOST_CHECK_CLOSE(b_a, 2.0,   0.01);

	// between a1 and b2, the distances should be sqrt(3*3 + 13*13) = 13.342 
	// A->B and sqrt(3*3 + 3*3) = 4.243 for B->A
	hausdorff(*volumesA[a1], *volumesB[b2], a_b, b_a);
	BOOST_CHECK_CLOSE(a_b, 13.342, 0.01);
	BOOST_CHECK_CLOSE(b_a, 4.243,  0.01);

	// between root_a and root_b Hausdorff should be sqrt(2*2 + 8*8) = 8.25 for 
	// A->B and 2 for B->A
	hausdorff(*volumesA[root_a], *volumesB[root_b], a_b, b_a);
	BOOST_CHECK_CLOSE(a_b, 8.246, 0.01);
	BOOST_CHECK_CLOSE(b_a, 2.0,   0.01);
}

void hausdorff_anisotropic() {

	Crag cragA;
	Crag cragB;
	CragVolumes volumesA(cragA);
	CragVolumes volumesB(cragB);

	// add two nodes each
	Crag::Node a1 = cragA.addNode();
	Crag::Node a2 = cragA.addNode();
	Crag::Node b1 = cragB.addNode();
	Crag::Node b2 = cragB.addNode();

	// add more nodes as parents of a and b
	Crag::Node p_a1 = cragA.addNode();
	Crag::Node p_b1 = cragB.addNode();
	cragA.addSubsetArc(a1, p_a1);
	cragB.addSubsetArc(b1, p_b1);

	// add more nodes merging p_a1 and a2 (same for b)
	Crag::Node root_a = cragA.addNode();
	Crag::Node root_b = cragB.addNode();
	cragA.addSubsetArc(p_a1, root_a);
	cragA.addSubsetArc(a2, root_a);
	cragB.addSubsetArc(p_b1, root_b);
	cragB.addSubsetArc(b2, root_b);

	// create volumes
	std::shared_ptr<CragVolume> volumeA1 = std::make_shared<CragVolume>(11, 11, 1);
	std::shared_ptr<CragVolume> volumeA2 = std::make_shared<CragVolume>(11, 11, 1);
	std::shared_ptr<CragVolume> volumeB1 = std::make_shared<CragVolume>(11, 11, 1);
	std::shared_ptr<CragVolume> volumeB2 = std::make_shared<CragVolume>(11, 11, 1);
	volumeA2->setOffset(util::point<float, 3>(5, 6, 0));
	volumeB2->setOffset(util::point<float, 3>(5, 6, 0));
	volumeA1->setResolution(util::point<float, 3>(1, 2, 1));
	volumeA2->setResolution(util::point<float, 3>(1, 2, 1));
	volumeB1->setResolution(util::point<float, 3>(1, 2, 1));
	volumeB2->setResolution(util::point<float, 3>(1, 2, 1));

	volumeA1->data() = 0;
	volumeA2->data() = 0;
	volumeB1->data() = 0;
	volumeB2->data() = 0;

	// add a stripe for volumeA{1,2} and a dot for volumeB{1,2}
	//
	// 00000000000 00000000000
	// 00000000000 00000000000
	// 00000000000 00000000000
	// 00000000x00 00000000*00     x=8,y=3
	// 00000000000 00000000000
	// *********** xxxxxxxxxxx     y=5
	// 00000000000 00000000000  *  x=13,y=6
	// 00000000000 00000000000
	// 00000000000 00000xxxxxxxxxxx
	// 00000000000 00000000000
	// 00000000000 00000000000

	for (int x = 0; x < 11; x++) {
		(*volumeA1)(x, 5, 0) = 1;
		(*volumeA2)(x, 5, 0) = 1;
	}
	(*volumeB1)(8, 3, 0) = 1;
	(*volumeB2)(8, 3, 0) = 1;

	volumesA.setVolume(a1, volumeA1);
	volumesA.setVolume(a2, volumeA2);
	volumesB.setVolume(b1, volumeB1);
	volumesB.setVolume(b2, volumeB2);

	{
		HausdorffDistance hausdorff(100);

		double a_b, b_a;
		hausdorff(*volumesA[a1], *volumesB[b1], a_b, b_a);

		BOOST_CHECK_CLOSE(a_b, sqrt(4*4 + 8*8), 0.01);
		BOOST_CHECK_CLOSE(b_a, sqrt(4*4),       0.01);

		// same for parents
		hausdorff(*volumesA[p_a1], *volumesB[p_b1], a_b, b_a);
		BOOST_CHECK_CLOSE(a_b, sqrt(4*4 + 8*8), 0.01);
		BOOST_CHECK_CLOSE(b_a, sqrt(4*4),       0.01);

		// b2 is offset by (5,6), which corresponds (5,3) pixels
		//
		// between a1 and b2, the distances should be sqrt(13*13 + 10*10) A->B and 
		// sqrt(3*3 + 3*3) = 4.243 for B->A
		hausdorff(*volumesA[a1], *volumesB[b2], a_b, b_a);
		BOOST_CHECK_CLOSE(a_b, sqrt(13*13 + 2*2), 0.01);
		BOOST_CHECK_CLOSE(b_a, sqrt(3*3 + 2*2),   0.01);

		// between root_a and root_b Hausdorff should be sqrt(2*2 + 16*16) for A->B 
		// and 4 for B->A
		hausdorff(*volumesA[root_a], *volumesB[root_b], a_b, b_a);
		BOOST_CHECK_CLOSE(a_b, sqrt(4*4 + 8*8), 0.01);
		BOOST_CHECK_CLOSE(b_a, sqrt(4*4),       0.01);
	}

	{
		HausdorffDistance hausdorff(10);

		double a_b, b_a;
		hausdorff(*volumesA[a1], *volumesB[b1], a_b, b_a);

		BOOST_CHECK_CLOSE(a_b, std::min(10.0, sqrt(4*4 + 8*8)), 0.01);
		BOOST_CHECK_CLOSE(b_a, std::min(10.0, sqrt(4*4)),       0.01);

		// same for parents
		hausdorff(*volumesA[p_a1], *volumesB[p_b1], a_b, b_a);
		BOOST_CHECK_CLOSE(a_b, std::min(10.0, sqrt(4*4 + 8*8)), 0.01);
		BOOST_CHECK_CLOSE(b_a, std::min(10.0, sqrt(4*4)),       0.01);

		// b2 is offset by (5,6), which corresponds (5,3) pixels
		//
		// between a1 and b2, the distances should be sqrt(13*13 + 10*10) A->B and 
		// sqrt(3*3 + 3*3) = 4.243 for B->A
		hausdorff(*volumesA[a1], *volumesB[b2], a_b, b_a);
		BOOST_CHECK_CLOSE(a_b, std::min(10.0, sqrt(13*13 + 2*2)), 0.01);
		BOOST_CHECK_CLOSE(b_a, std::min(10.0, sqrt(3*3 + 2*2)),   0.01);

		// between root_a and root_b Hausdorff should be sqrt(2*2 + 16*16) for A->B 
		// and 4 for B->A
		hausdorff(*volumesA[root_a], *volumesB[root_b], a_b, b_a);
		BOOST_CHECK_CLOSE(a_b, std::min(10.0, sqrt(4*4 + 8*8)), 0.01);
		BOOST_CHECK_CLOSE(b_a, std::min(10.0, sqrt(4*4)),       0.01);
	}
}
