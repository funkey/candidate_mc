#ifndef CANDIDATE_MC_IO_VECTORS_H__
#define CANDIDATE_MC_IO_VECTORS_H__

#include <vector>
#include <fstream>

template <typename T>
void storeVector(const std::vector<T>& v, std::string filename) {

	std::ofstream file(filename);
	for (double f : v)
		file << f << std::endl;
}

template <typename T>
std::vector<T> retrieveVector(std::string filename) {

	std::ifstream file(filename);
	std::vector<double> v;

	if (file.fail())
		return v;

	while (true) {
		double f;
		file >> f;
		if (!file.good())
			break;
		v.push_back(f);
	}

	return v;
}

#endif // CANDIDATE_MC_IO_VECTORS_H__

