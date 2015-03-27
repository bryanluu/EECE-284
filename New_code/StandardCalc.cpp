#include "StandardCalc.h"


void StandardCalc::boundValueBetween(double* value, const double &lowerBound, const double &higherBound)
{
	if (*value < lowerBound)
		*value = lowerBound;
	else if (*value > higherBound)
		*value = higherBound;
	return;
}

double StandardCalc::boundValueBetween(const double &value, const double &lowerBound, const double &higherBound)
{
	if (value < lowerBound)
		return lowerBound;
	else if (value > higherBound)
		return higherBound;
	return value;
}
