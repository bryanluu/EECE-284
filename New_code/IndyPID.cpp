#include "IndyPID.h"
#include "StandardCalc.h"

PID::PID(double* newInput, double* newSetpoint, double* newOutput)
{
	Input = newInput;
	Setpoint = newSetpoint;
	Output = newOutput;

	fixedSampleRate = false;

	lastTime = millis();
	lastError = 0.0;
	error = 0.0;

	outMax = 100;
	outMin = -100;

	kp = new double(0);
	ki = new double(0);
	kd = new double(0);

}

double PID::Compute()
{
	timeChange = millis() - lastTime;

	if (fixedSampleRate == true && timeChange < sampleTime)
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

	*Output = -(PTerm + ITerm + DTerm);



	// Output is useful for testing
	return *Output;
}

void PID::updateOldData()
{
	lastError = error;
	lastTime = millis();
}

/*Returns a positive error if Input is Undershooting Setpoint, negative if overshooting, and 0 if on setpoint*/
double PID::calculateError()
{
	error = *Input - *Setpoint;
	return error;
}



void PID::attach_Kd_To(double* newKd) { delete kd; kd = newKd; }

void PID::attach_Kp_To(double* newKp) { delete kp;	kp = newKp; }

void PID::attach_Ki_To(double* newKi) { delete ki; ki = newKi; }

void PID::setKp(double newKp) { *kp = newKp; }

void PID::setKd(double newKd) { *kd = newKd; }

void PID::setKi(double newKi) { *ki = newKi; }

double PID::GetKp() { return *kp; }

double PID::GetKi() { return *ki; }

double PID::GetKd() { return *kd; }

double PID::GetError() { return error; }

void PID::tune(double newKp, double newKi, double newKd)
{
	*kp = newKp;
	*ki = newKi;
	*kd = newKd;
}

void PID::SetSampleTime(int sampleTimeInMilliseconds)
{
	sampleTime = sampleTimeInMilliseconds;
	fixedSampleRate = true;
}

inline void PID::AutoSample() { fixedSampleRate = false; }

void PID::setBounds(double newOutputMin, double newOutputMax)
{
	outMax = newOutputMax;
	outMin = newOutputMin;
}
