#include "TapeFollower.h"
#include "StandardCalc.h"

TapeFollower::TapeFollower(int* leftInputVar, int* midInputVar, int* rightInputVar, double* output)
{
	leftInput = leftInputVar;
	midInput = midInputVar;
	rightInput = rightInputVar;
	Output = output;

	fixedSampleRate = false;

	lastTime = millis();
	lastError = 0;
	error = 0;
	lastExtremeError = 0;

	outMax = 100;
	outMin = -100;

	kp = new double(0);
	ki = new double(0);
	kd = new double(0);

    THRESHOLD = 250;
    LeftOffset = 0;
    MidOffset = 0;
    RightOffset = 0;
}

double TapeFollower::Compute()
{
	timeChange = millis() - lastTime;

	if (fixedSampleRate == true && timeChange < SampleTime)
	{
		// Don't update the PID loop
		return *Output;
	} //Else, uses the time since last update to calculate PID


	updateOldData();

	error = calculateError();

	PTerm = *kp*(error);
	DTerm = *kd*(error - lastError) / timeChange;
	ITerm += *ki*(error)*timeChange;

	StandardCalc::boundValueBetween(&ITerm, outMin, outMax);

	*Output = PTerm + ITerm + DTerm;



	// Output is useful for testing
	return *Output;
}

void TapeFollower::updateOldData()
{
	lastError3 = lastError2;
	lastError2 = lastError;
	lastError = error;
	if (error == -3 || error == 3)
	{
		lastExtremeError = error;
	}
	lastTime = millis();
}

double TapeFollower::calculateError()
{
	if (goingStraight())
	{
		error = 0;
	}
	else if (slightlyRight())
	{
		error = 1;
	}
	else if (slightlyLeft())
	{
		error = -1;
	}
	else if (moreLeft())
	{
		error = -2;
	}
	else if (moreRight())
	{
		error = 2;
	}
	else if (tooMuchOnLeft())
	{
		error = -3;
	}
	else if (tooMuchOnRight())
	{
		error = 3;
	}
	// else if (missedState())
	// {
	// 	error = -lastExtremeError;
	// }
	return error;
}



void TapeFollower::attach_Kd_To(double* newKd) { delete kd; kd = newKd; }

void TapeFollower::attach_Kp_To(double* newKp) { delete kp;	kp = newKp; }

void TapeFollower::attach_Ki_To(double* newKi) { delete ki; ki = newKi; }

void TapeFollower::setKp(double newKp) { *kp = newKp; }

void TapeFollower::setKd(double newKd) { *kd = newKd; }

void TapeFollower::setKi(double newKi) { *ki = newKi; }

double TapeFollower::GetKp() { return *kp; }

double TapeFollower::GetKi() { return *ki; }

double TapeFollower::GetKd() { return *kd; }

double TapeFollower::GetError() { return error; }

void TapeFollower::tune(double newKp, double newKi, double newKd)
{
	*kp = newKp;
	*ki = newKi;
	*kd = newKd;
}

void TapeFollower::SetSampleTime(int sampleTimeInMilliseconds)
{
	SampleTime = sampleTimeInMilliseconds;
	fixedSampleRate = true;
}

inline void TapeFollower::AutoSample() { fixedSampleRate = false; }

void TapeFollower::setBounds(double newOutputMin, double newOutputMax)
{
	outMax = newOutputMax;
	outMin = newOutputMin;
}


void TapeFollower::SetThreshold(double newThreshold)
{
    THRESHOLD = newThreshold;
}

void TapeFollower::SetOffsets(double left, double mid, double right)
{
    LeftOffset = left;
    MidOffset = mid;
    RightOffset = right;
}

bool TapeFollower::goingStraight() { return  (*leftInput < THRESHOLD + LeftOffset) && (*midInput >= THRESHOLD + MidOffset) && (*rightInput < THRESHOLD + RightOffset); }

bool TapeFollower::slightlyLeft() { return (*leftInput < THRESHOLD + LeftOffset) && (*midInput >= THRESHOLD + MidOffset) && (*rightInput >= THRESHOLD + RightOffset); }

bool TapeFollower::slightlyRight() { return (*leftInput >= THRESHOLD + LeftOffset) && (*midInput >= THRESHOLD + MidOffset) && (*rightInput < THRESHOLD + RightOffset); }

bool TapeFollower::moreLeft() { return (*leftInput < THRESHOLD + LeftOffset) && (*midInput < THRESHOLD + MidOffset) && (*rightInput >= THRESHOLD + RightOffset); }

bool TapeFollower::moreRight() { return (*leftInput >= THRESHOLD + LeftOffset) && (*midInput < THRESHOLD + MidOffset) && (*rightInput < THRESHOLD + RightOffset); }

bool TapeFollower::offTape() { return (*leftInput < THRESHOLD + LeftOffset) && (*midInput < THRESHOLD + MidOffset) && (*rightInput < THRESHOLD + RightOffset); }

bool TapeFollower::tooMuchOnRight() { return offTape() && (lastError > 0) && (lastError2 > 0); }

bool TapeFollower::tooMuchOnLeft() { return offTape() && (lastError < 0) && (lastError2 < 0); }

bool TapeFollower::missedState() {return offTape() && (lastError == 0); }