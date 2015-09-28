#include <tests.h>
#include <vigra/multi_array.hxx>

void third_party_vigra() {

	typedef vigra::MultiArray<2, float> MA;

	MA a(vigra::Shape2(10, 10));
	for (float& f : a)
		f = 1;
	for (MA::iterator i = a.begin(); i != a.end(); i++)
		*i = 0;

	float sum = 0;
	for (MA::const_iterator i = static_cast<const MA>(a).begin(); i != static_cast<const MA>(a).end(); i++)
		sum += *i;

	// the following does not work:
	//for (MA::const_iterator i = a.begin(); i != a.end(); i++)
		//sum += *i;
}
