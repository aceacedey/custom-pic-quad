//**********************************************************************
#include "QCM\QCMLocal.h"
//**********************************************************************

//**********************************************************************
void	QCMPerformStep(RCData* RC, DCMData* IMU, MCMData* MC)
	{

	//============================================================
	// Identify Integration/Differentiation time interval for the
	// current step; initialize LastRC values on start.
	//------------------------------------------------------------
	//{
	ulong	CurrentTS	= TMRGetTS();	// Capture timestamp
	if (0 == QSD.LastTS)
		{
		// The first step in the current flight
		QSD.dT = 0.0;
		}
	else
		{
		// Not the first step, we have saved values from last step
		QSD.dT = ((float)(CurrentTS - QSD.LastTS)) * TMRGetTSRate();
		}
	// Store current timestamp for next iteration
	QSD.LastTS = CurrentTS;
	//}
	//============================================================
			

	//************************************************************
	// Process Throttle
	//************************************************************
	//{
	QSD.Throttle = RC->Throttle;
	//------------------------------------------------------------
	// Calculate throttle "adjustment" based upon Inclination
	//------------------------------------------------------------
	// Adjust Throttle to compensate for the loss of lift due
	// to Inclination.
	//------------------------------------------------------------
	if (IMU->Incl > 0.0)
		{
		float	K = IMU->Incl;
		if (K < 0.8)	K = 0.8;	// Limit correction to 25%
		QSD.Throttle /= K;
		}
	//------------------------------------------------------------
	// Limit Throttle input to leave space for attitude control.
	//------------------------------------------------------------
	if (QSD.Throttle > 0.85)
		QSD.Throttle = 0.85;
	//}
	//************************************************************



	//************************************************************
	// Calculate Roll-Pitch-Yaw "adjustments" based upon IMU and
	// RC control input
	//************************************************************
	//{
	// First, calculate direct control error(s) AND bring them
	// into the proper range...
	// Makes sense to do it while the model is still on the ground
	// so if it is not level it will not swing to the side on
	// the lift-off.
	//------------------------------------------------------------
	QSD.RollError	= QCMRangeToPi(		RC->Roll  - IMU->Roll);
	QSD.PitchError	= QCMRangeToHalfPi(	RC->Pitch - IMU->Pitch);
	//------------------------------------------------------------
	// Yaw is kind of "special case" - no need to control Yaw
	// until we get into air...
	//------------------------------------------------------------
	if (QSD.Throttle > _PID_ThrottleNoFly)
		{
		// Yaw input from RC is treated as "rate", so first it has to
		// be integrated... We select weighting factor (0.5) so that
		// at max RC Yaw rate (1 rad/sec) the rate of turn control
		// set point change is limited to 0.5 rad/sec.
		QSD.RCYawInt   += RC->Yaw * 0.5 * QSD.dT;
		// Now we need to bring it into the range -Pi < RCYawInt <= Pi
		QSD.RCYawInt	= QCMRangeToPi(QSD.RCYawInt);
		//------------------------------------------------------------
		// Now we may calculate Yaw error...
		QSD.YawError	= QCMRangeToPi(QSD.RCYawInt - IMU->Yaw);
		}
	else
		{
		// Quad is still on the ground...
		QSD.RCYawInt	= 0.0;
		QSD.YawError	= 0.0;	// No Yaw control applied...
		}

	//------------------------------------------------------------
	// Calculate Proportional term
	//---------------------------------------------
	QSD.DeltaRollProp 	= _PID_Roll_Kp   * QSD.RollError;
	QSD.DeltaPitchProp 	= _PID_Pitch_Kp  * QSD.PitchError;
	QSD.DeltaYawProp 	= _PID_Yaw_Kp  	 * QSD.YawError;
	//}
	//************************************************************
		
	//************************************************************
	// Calculate Roll-Pitch-Yaw "adjustment" based upon State
	// Change derivatives
	//************************************************************
	//{
	//------------------------------------------------------------
	// Calculate state derivative term
	//------------------------------------------------------------
	QCMStepDifferentiator(); // First, calculate error derivatives
	//------------------------------------------------------------
	// Caculate Differential term of the control input from PID
	//------------------------------------------------------------
	QSD.DeltaRollDiff	= _PID_Roll_Kd  * QSD.RollErrDer;
	QSD.DeltaPitchDiff	= _PID_Pitch_Kd * QSD.PitchErrDer;
	QSD.DeltaYawDiff	= _PID_Yaw_Kd   * QSD.YawErrDer;
	//}
	//************************************************************
	

	//************************************************************
	// Calculate Roll-Pitch-Yaw "adjustment" based upon State
	// Change integral error
	//************************************************************
	//{
	//------------------------------------------------------------
	// Calculate Integral term
	//------------------------------------------------------------
	QCMStepIntegrator();	// First, update Integrator variables
	//------------------------------------------------------------
	// Now calculate adjustment as an additional contribution to
	// the proportional term 
	QSD.DeltaRollInt 	= _PID_Roll_Kp   * QSD.RollErrorSum;
	QSD.DeltaPitchInt 	= _PID_Pitch_Kp  * QSD.PitchErrorSum;
	QSD.DeltaYawInt 	= _PID_Yaw_Kp	 * QSD.YawErrorSum;
	//}
	//************************************************************

	//************************************************************
	// Calculate total 3-D adjustment
	//************************************************************
	QSD.DeltaRoll	= QSD.DeltaRollProp			// Proportional
					+ QSD.DeltaRollDiff			// Differential
					+ QSD.DeltaRollInt;			// Integral
	//-------------------
	QSD.DeltaPitch	= QSD.DeltaPitchProp		// Proportional
					+ QSD.DeltaPitchDiff		// Differential
					+ QSD.DeltaPitchInt;		// Integral

	//-------------------
	QSD.DeltaYaw	= QSD.DeltaYawProp			// Proportional
					+ QSD.DeltaYawDiff			// Differential
					+ QSD.DeltaYawInt;			// Integral
	//************************************************************

	//************************************************************
	// Implement CONTROL limits
	//************************************************************
	// Identify vurrent control limit based upon the Throttle
	// value and pre-set control limit
	float CM	= QSD.Throttle * _PID_ControlLimit;	

	// Roll ----------------------------------------
	if (QSD.DeltaRoll >  CM)	QSD.DeltaRoll 	=  CM;
	if (QSD.DeltaRoll < -CM)	QSD.DeltaRoll 	= -CM;
	// Pitch ---------------------------------------
	if (QSD.DeltaPitch >  CM)	QSD.DeltaPitch 	=  CM;
	if (QSD.DeltaPitch < -CM)	QSD.DeltaPitch 	= -CM;
	// Yaw ---------------------------------------
	if (QSD.DeltaYaw >    CM)	QSD.DeltaYaw 	=  CM;
	if (QSD.DeltaYaw <   -CM)	QSD.DeltaYaw 	= -CM;
	
	//************************************************************
	// Implement Motor Balance adjustment for Roll-Pitch
	//************************************************************
	// NOTE: this adjustment is not subject to control limit
	QSD.Voltage = ADCGetBatteryVoltage();
	float Power = QSD.Throttle * QSD.Voltage;
	// Using calculated constants....
	// These constants are dependent on motors, props, and
	// C.G. placement - should be adjusted if anything changes...
	QSD.DeltaRoll 	-= 0.0030 * Power;	
	QSD.DeltaPitch	+= 0.0000 * Power;
	QSD.DeltaYaw	-= 0.0050 * Power;
	
	
	//************************************************************
	// Implement motor power adjustments
	//************************************************************
	//{
	//************************************************************
	// Throttle adjustment for the transverse direction
	// REMEBER: Roll angle is positive when we bank right
	//------------------------------------------------------------
	MC->R	= QSD.Throttle - QSD.DeltaRoll;	
	MC->L	= QSD.Throttle + QSD.DeltaRoll;
	//------------------------------------------------------------
	// Throttle adjustment in the longitudinal direction
	// REMEMBER: Pitch angle is positive when the nose points UP
	//------------------------------------------------------------
	MC->B	= QSD.Throttle - QSD.DeltaPitch;
	MC->F	= QSD.Throttle + QSD.DeltaPitch;
	//------------------------------------------------------------
	// Throttle adjustment for Yaw compensation
	// REMEMBER: Yaw is positive for clockwise rotation
	// F-B props rotate quad counterclockwise, while
	// L-R - rotate quad clockwise
	MC->B -= QSD.DeltaYaw;
	MC->F -= QSD.DeltaYaw;
	//--------------
	MC->R += QSD.DeltaYaw;
	MC->L += QSD.DeltaYaw;
	//--------------
	//}
	//************************************************************

	//***********************************************************
	// Normalize all calculated Throttle values 
	//-----------------------------------------------------------
	QCMNormalize(MC);
	//***********************************************************
	}
//************************************************************




