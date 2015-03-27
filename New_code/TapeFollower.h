#ifndef tapefollower_h
#define tapefollower_h
#include "WProgram.h"

/*
	PID-like implementation of a tape follower, based on the PID_v1 library on the Arduino site. 
*/
class TapeFollower
{
	public:

	//commonly used functions **************************************************************************
	// Takes a pointer to the two sensor values, adjusts the Output pointer
	TapeFollower(int* leftInputVar, int* midInputVar, int* rightInputVar, double* output);

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
    
    void SetThreshold(double newThresh);
    
    void SetOffsets(double left, double mid, double right);

	//Display functions ****************************************************************
	double GetKp();
	double GetKi();	
	double GetKd();
	double GetError();

	bool goingStraight();
	bool slightlyLeft();
	bool slightlyRight();
	bool moreLeft();
	bool moreRight();
	bool tooMuchOnLeft();
	bool tooMuchOnRight();
	bool offTape();
	bool missedState();

	double THRESHOLD;
    double LeftOffset, MidOffset, RightOffset;

	double* kp;                  // * (P)roportional Tuning Parameter
	double* ki;                  // * (I)ntegral Tuning Parameter
	double* kd;                  // * (D)erivative Tuning Parameter

	unsigned long lastTime;
	double ITerm, PTerm, DTerm, lastInput;

	int *leftInput, *midInput, *rightInput;
	double *Output;

	double error, lastError, lastError2, lastError3, lastExtremeError;

	int timeChange;

	int SampleTime;
	bool fixedSampleRate;
	double outMin, outMax;



	double calculateError();

	void updateOldData();

	
};

#endif