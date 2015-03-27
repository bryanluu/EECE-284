#ifndef indy_pid
#define indy_pid
#include "WProgram.h"

/*
	PID, based on the PID_v1 library on the Arduino site. 
*/
class PID
{
	public:

	//commonly used functions **************************************************************************
	// Takes a pointer to the two sensor values, adjusts the Output pointer
	PID(double* input, double* setpoint, double* output);

	double Compute();                       // * performs the PID calculation.  it should be
	//   called every time loop() cycles. ON/OFF and
	//   calculation frequency can be set using SetMode
	//   SetSampleTime respectively

	void setKp(double);
	void setKd(double);
	void setKi(double);

	void tune(double newKp, double newKi, double newKd);
	void setBounds(double outMin, double outMax);

	/* Attachs a value to the Kp term*/
	void attach_Kp_To(double*);

	/* Attachs a value to the Kd term*/
	void attach_Kd_To(double*);

	/* Attachs a value to the Kd term*/
	void attach_Ki_To(double*);

	void SetSampleTime(int);              // * sets the frequency, in Milliseconds, with which 
	//   the PID calculation is performed.  default is 100

	void AutoSample();

	//Display functions ****************************************************************
	double GetKp();
	double GetKi();	
	double GetKd();
	double GetError();

#ifndef TESTING
protected:
#endif



	double* kp;                  // * (P)roportional Tuning Parameter
	double* ki;                  // * (I)ntegral Tuning Parameter
	double* kd;                  // * (D)erivative Tuning Parameter

	unsigned long lastTime;
	double ITerm, PTerm, DTerm, lastInput;

	double *Input, *Setpoint, *Output;

	double error, lastError;

	int timeChange;

	int sampleTime;
	bool fixedSampleRate;
	double outMin, outMax;



	double calculateError();

	void updateOldData();
};

#endif