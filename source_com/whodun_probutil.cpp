#include "whodun_probutil.h"

#include <math.h>

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
