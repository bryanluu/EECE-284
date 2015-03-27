#ifndef standard_calc
#define standard_calc

namespace StandardCalc
{
	/*Takes in a value by reference and modifies it to be within the given lowerBound and upperBound.*/
	void boundValueBetween(double* value, const double& lowerBound, const double& upperBound);
	/*Returns a bounded value of the given value: it caps the value at either lower or upper bound and returns the result.*/
	double boundValueBetween(const double& value, const double& lowerBound, const double& upperBound);
}
#endif