#include "whodun_opt.h"

#include <math.h>
#include <string.h>
#include <iostream>

ObjectiveFunction::~ObjectiveFunction(){}

double goldenRatioSearch(ObjectiveFunction* toSearch, double lowBound, double higBound, double tol){
	double goldRat = (sqrt(5) + 1) / 2;
	//thank you wikipedia
	double a = lowBound;
	double b = higBound;
	double c = b - (b-a)/goldRat;
	double d = a + (b-a)/goldRat;
	while(fabs(c-d) > tol){
		if(toSearch->getObjective(c) < toSearch->getObjective(d)){
			b = d;
		}
		else{
			a = c;
		}
		c = b - (b-a)/goldRat;
		d = a + (b-a)/goldRat;
	}
	return (b+a)/2;
}

MultivariateObjectiveFunction::MultivariateObjectiveFunction(){
	dimension = 0;
	defGradDX = 1.0e-4;
}

MultivariateObjectiveFunction::~MultivariateObjectiveFunction(){}

void MultivariateObjectiveFunction::getGradient(double* atValue, double* gradStore){
	double baseVal = getObjective(atValue);
	for(uintptr_t i = 0; i<dimension; i++){
		double origV = atValue[i];
		atValue[i] = origV + defGradDX;
		double curVal = getObjective(atValue);
		atValue[i] = origV;
		gradStore[i] = (curVal - baseVal) / defGradDX;
	}
}

MultivariateOptimizer::~MultivariateOptimizer(){}

/**
 * This will do a line search on [0,+inf).
 * @param toSearch The objective function to try and minimize.
 * @param startNudge THe starting value to feed to the objective function.
 * @param tol The absolute tolerance in the parameter to use for searching.
 * @param maxNudge The maximum distance.
 * @return The location of the minimum.
 */
double lineSearch(ObjectiveFunction* toSearch, double startNudge, double tol, double maxNudge){
	double startX = 0.0;
	double startV = toSearch->getObjective(startX);
	double newX = startNudge;
	double newV = toSearch->getObjective(newX);
	while(newV < startV){
		newX = newX * 2;
		if(newX > maxNudge){
			newX = maxNudge;
			break;
		}
		newV = toSearch->getObjective(newX);
	}
	return goldenRatioSearch(toSearch, startX, newX, tol);
}

/**
 * This will normalize a vector.
 * @param numP The number of things in the vector.
 * @param toNorm The thing to normalize.
 * @param toler THe minimum magnitude to try and normalize.
 */
void normalizeGradient(uintptr_t numP, double* toNorm){
	double gradSqr = 0.0;
	for(uintptr_t i = 0; i<numP; i++){
		gradSqr += (toNorm[i]*toNorm[i]);
	}
	gradSqr = 1.0 / sqrt(gradSqr);
	if(isinf(gradSqr) || isnan(gradSqr)){
		memset(toNorm, 0, numP*sizeof(double));
		return;
	}
	for(uintptr_t i = 0; i<numP; i++){
		toNorm[i] = -toNorm[i] * gradSqr;
	}
}

/**Wrap a multivariate function for line search.*/
class LineSearchObjectiveFunction : public ObjectiveFunction{
public:
	/**
	 * Wrap a multivariate function.
	 * @param baseFunction The function to wrap.
	 * @param baseLocation The starting point of the line.
	 * @param direction The search direction.
	 * @param storeLocation The place to store a location.
	 */
	LineSearchObjectiveFunction(MultivariateObjectiveFunction* baseFunction, double* baseLocation, double* direction, double* storeLocation){
		baseFun = baseFunction;
		baseLoc = baseLocation;
		dirLoc = direction;
		tmpLoc = storeLocation;
	}
	~LineSearchObjectiveFunction(){}
	double getObjective(double atValue){
		for(uintptr_t i = 0; i<baseFun->dimension; i++){
			tmpLoc[i] = baseLoc[i] + atValue*dirLoc[i];
		}
		return baseFun->getObjective(tmpLoc);
	}
	/**The function to wrap.*/
	MultivariateObjectiveFunction* baseFun;
	/**The starting point of the line.*/
	double* baseLoc;
	/**The search direction.*/
	double* dirLoc;
	/**The place to store a location.*/
	double* tmpLoc;
};

GradientDescentOptimizer::GradientDescentOptimizer(){}

GradientDescentOptimizer::~GradientDescentOptimizer(){}

void GradientDescentOptimizer::optimize(MultivariateObjectiveFunction* toSearch, double* fromLoc, double tol){
	gradStore.resize(toSearch->dimension);
	locStore.resize(toSearch->dimension);
	if(logFile){ (*logFile) << "Error\tMovement" << std::endl; }
	double lastChange = 5 * tol;
	while(lastChange > tol){
		if(logFile){ (*logFile) << toSearch->getObjective(fromLoc); }
		toSearch->getGradient(fromLoc, &(gradStore[0]));
		normalizeGradient(toSearch->dimension, &(gradStore[0]));
		LineSearchObjectiveFunction curLS(toSearch, fromLoc, &(gradStore[0]), &(locStore[0]));
		lastChange = lineSearch(&curLS, tol, tol/4, 1.0/0.0);
		for(uintptr_t i = 0; i<toSearch->dimension; i++){
			fromLoc[i] += lastChange*gradStore[i];
		}
		if(logFile){ (*logFile) << "\t" << lastChange << std::endl; }
	}
}

LBFGSOptimizer::LBFGSOptimizer(){
	maxHist = 5;
	maxDouble = 0;
	errorTolerance = 0.0;
}

LBFGSOptimizer::~LBFGSOptimizer(){
}

void LBFGSOptimizer::optimize(MultivariateObjectiveFunction* toSearch, double* fromLoc, double tol){
	uintptr_t numParams = toSearch->dimension;
	gradHistS.resize(numParams*maxHist);
	paramHistS.resize(numParams*maxHist);
	qstoreS.resize(numParams);
	astoreS.resize(maxHist);
	double* paramGHist = &(gradHistS[0]);
	double* paramHist = &(paramHistS[0]);
	double* qstore = &(qstoreS[0]);
	double* astore = &(astoreS[0]);
	//init the parameters
	double maxMove = 1.0 / 0.0;
	if(maxDouble){
		maxMove = tol;
		for(uintptr_t i = 0; i<maxDouble; i++){ maxMove = maxMove * 2; }
	}
	memcpy(paramHist, fromLoc, numParams*sizeof(double));
	uintptr_t lastParam = 0;
	uintptr_t liveParams = 1;
	//loop it
	double startVal = toSearch->getObjective(paramHist + numParams*lastParam);
	if(logFile){ (*logFile) << "Error\tMovement" << std::endl; }
	double lastChange = 5 * tol;
	while(lastChange > tol){
		if(logFile){ (*logFile) << startVal; }
		//get the current gradient
		toSearch->getGradient(paramHist + numParams*lastParam, paramGHist + numParams*lastParam);
		//pick a search direction
		memcpy(qstore, paramGHist + lastParam*numParams, numParams*sizeof(double));
		double* focAlp = astore;
		int noff = lastParam;
		for(uintptr_t i = 1; i<liveParams; i++){
			int poff = noff;
			noff--; if(noff < 0){ noff += maxHist; }
			double* curP = paramHist + numParams*noff;
			double* curPG = paramGHist + numParams*noff;
			double* prevP = paramHist + numParams*poff;
			double* prevPG = paramGHist + numParams*poff;
			double curN = 0.0;
			double curD = 0.0;
			for(uintptr_t j = 0; j<numParams; j++){
				curN += (prevP[j] - curP[j])*qstore[j];
				curD += (prevPG[j] - curPG[j])*(prevP[j] - curP[j]);
			}
			double curAlp = curN / curD;
			for(uintptr_t j = 0; j<numParams; j++){
				qstore[j] = qstore[j] - curAlp*(prevPG[j] - curPG[j]);
			}
			*focAlp = curAlp;
			focAlp++;
		}
		for(uintptr_t i = 1; i<liveParams; i++){
			focAlp--;
			double curAlp = *focAlp;
			int poff = noff+1; if(poff >= (intptr_t)maxHist){ poff -= maxHist; }
			double* curP = paramHist + numParams*noff;
			double* curPG = paramGHist + numParams*noff;
			double* prevP = paramHist + numParams*poff;
			double* prevPG = paramGHist + numParams*poff;
			double curN = 0.0;
			double curD = 0.0;
			for(uintptr_t j = 0; j<numParams; j++){
				curN += (prevPG[j] - curPG[j])*qstore[j];
				curD += (prevPG[j] - curPG[j])*(prevP[j] - curP[j]);
			}
			double curBet = curN / curD;
			for(uintptr_t j = 0; j<numParams; j++){
				qstore[j] += (prevP[j]-curP[j])*(curAlp - curBet);
			}
			noff = poff;
		}
		//if any are nan, go back to gradient
		for(uintptr_t j = 0; j<numParams; j++){
			if(!isfinite(qstore[j])){
				memcpy(qstore, paramGHist + lastParam*numParams, numParams*sizeof(double));
				break;
			}
		}
		//normalize the gradient
		normalizeGradient(numParams, qstore);
		//line search
		LineSearchObjectiveFunction curLS(toSearch, paramHist + numParams*lastParam, qstore, fromLoc);
		lastChange = lineSearch(&curLS, tol, tol/4, maxMove);
		maxMove = maxMove * 2;
		//make the next guess
		uintptr_t prevParam = lastParam;
		lastParam++; if(lastParam >= maxHist){ lastParam = 0; }
		double* setFP = paramHist + numParams*prevParam;
		double* setNP = paramHist + numParams*lastParam;
		for(uintptr_t i = 0; i<numParams; i++){
			setNP[i] = setFP[i] + lastChange*qstore[i];
		}
		liveParams++; if(liveParams > maxHist){ liveParams = maxHist; }
		if(logFile){ (*logFile) << "\t" << lastChange << std::endl; }
		//check error change
		double endVal = toSearch->getObjective(paramHist + numParams*lastParam);
		if(fabs(endVal - startVal) < errorTolerance){ break; }
		startVal = endVal;
	}
	//report the last guess
	memcpy(fromLoc, paramHist + numParams*lastParam, numParams*sizeof(double));
}


