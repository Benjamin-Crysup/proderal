#include "whodun_probutil.h"

#include <math.h>
#include <float.h>
#include <stdint.h>

ProbabilitySummation::ProbabilitySummation(){
	runningSum = 0.0;
	maxLogProb = 0.0/0.0;
	nextQInd = 0;
}

void ProbabilitySummation::updateFromQueue(){
	//get the maximum currently in the queue
	double queueMax = queuedVals[0];
	for(int i = 1; i<nextQInd; i++){
		if(queuedVals[i] > queueMax){
			queueMax = queuedVals[i];
		}
	}
	//if it is larger than the current winner, shift
	if(isnan(maxLogProb) || (queueMax > maxLogProb)){
		if(!isnan(maxLogProb)){
			//rescale the current sum
			double rescaleFac = pow(10.0, maxLogProb - queueMax);
			runningSum = runningSum * rescaleFac;
		}
		maxLogProb = queueMax;
	}
	//add to the running sum
	double curSum = 0.0;
	for(int i = 0; i<nextQInd; i++){
		curSum = curSum + pow(10.0, queuedVals[i] - maxLogProb);
	}
	runningSum += curSum;
	//reset
	nextQInd = 0;
}

void ProbabilitySummation::addNextLogProb(double nextLogProb){
	queuedVals[nextQInd] = nextLogProb;
	nextQInd++;
	if(nextQInd >= SUMMATION_QUEUE){
		updateFromQueue();
	}
}

double ProbabilitySummation::getFinalLogSum(){
	if(nextQInd){
		updateFromQueue();
	}
	return maxLogProb + log10(runningSum);
}

#define EULERMASCHERONI 0.5772156649015328606065120900824024310421

double logGammaT(double forVal, double useEpsilon){
	#ifdef LGAMMA_FALLBACK
		if(forVal <= 0.0){
			return 1.0 / 0.0;
		}
		double fullTot = - EULERMASCHERONI * forVal;
		fullTot -= log(forVal);
		uintptr_t curK = 1;
		while(true){
			double curAdd = forVal/curK - log(1.0 + forVal/curK);
			if(fabs(fullTot) > useEpsilon){
				if(fabs(curAdd/fullTot) < useEpsilon){
					break;
				}
			}
			else if(fabs(curAdd) < useEpsilon){
				break;
			}
			fullTot += curAdd;
			curK++;
		}
		return fullTot;
	#else
		return lgamma(forVal);
	#endif
}

double logGamma(double forVal){
	#ifdef LGAMMA_FALLBACK
		return logGammaT(forVal, DBL_EPSILON);
	#else
		return lgamma(forVal);
	#endif
}

